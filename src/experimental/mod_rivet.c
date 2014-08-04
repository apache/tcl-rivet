
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include <ap_config.h>
#include <apr_queue.h>
#include <apr_strings.h>
#include <apr_general.h>
#include <apr_time.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include "TclWeb.h"
#include "mod_rivet.h"
#include "rivetChannel.h"

#define RIVET_TEMPLATE_CTYPE        "application/x-httpd-rivet"
#define RIVET_TCLFILE_CTYPE         "application/x-rivet-tcl"
#define MODNAME                     "mod_rivet"
#define MOD_RIVET_QUEUE_SIZE        100
#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024*1024)

mod_rivet_globals       module_globals;
rivet_server_conf       rsc;
rivet_interp_globals    interp_globals;

extern Tcl_ChannelType RivetChan;
apr_threadkey_t*        rivet_thread_key;
apr_threadkey_t*        handler_thread_key;

static int
RivetServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    ap_add_version_component(pPool, "Rivet/Experimental/"__DATE__"/"__TIME__"/");
    return OK;
}

static int RivetContent (rivet_thread_private* private)    
{
    Tcl_Interp*             interp = private->interp;
    request_rec*            r = private->r;    
    Tcl_Obj*                script;
    char                    script_buf[HUGE_STRING_LEN];
    apr_size_t              bytes_read;
    apr_finfo_t*            fileinfo = (apr_finfo_t*) apr_palloc(r->pool,sizeof(apr_finfo_t));
    apr_file_t*             filedesc;

    if (strcmp(r->handler, RIVET_TCLFILE_CTYPE)) {
        return DECLINED;
    }

    if (r->method_number != M_GET) { return DECLINED; }

    if (Rivet_chdir_file(r->filename) < 0)
    {
        /* something went wrong doing chdir into r->filename, we are not specific
         * at this. We simply emit an internal server error and print a log message
         */
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": Error accessing %s, could not chdir into directory", 
                     r->filename);

        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if (apr_stat(fileinfo, r->filename, APR_FINFO_SIZE | APR_FINFO_CSIZE ,r->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": couldn't get info on %s", 
                     r->filename);

        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if (apr_file_open(&filedesc,r->filename,APR_FOPEN_READ,APR_FPROT_UREAD,r->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": couldn't open %s", r->filename);

        return HTTP_INTERNAL_SERVER_ERROR;
    }
    apr_file_read_full(filedesc,script_buf,fileinfo->size,&bytes_read);
    apr_file_close(filedesc);

    r->content_type = "text/html";
    if (r->header_only) return OK;
    //if (r->args == 0) return OK;

    /* */
    Tcl_Preserve(interp);

    script = Tcl_NewStringObj(script_buf,bytes_read);
    Tcl_IncrRefCount(script);

    //apr_thread_mutex_lock(module_globals.tcl86_mutex);
    if (Tcl_EvalObjEx(interp, script, 0) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                     "Error evaluating script: %s", Tcl_GetVar(interp, "errorInfo", 0));
    }
    //apr_thread_mutex_unlock(module_globals.tcl86_mutex);

    TclWeb_PrintHeaders(private->req);
    Tcl_Flush(*(private->channel));
    Tcl_Release(interp);

    // Tcl_DecrRefCount(script);
    return OK;
}

void tcl_exit_handler (ClientData data)
{
    apr_threadkey_t*        thread_key = (apr_threadkey_t *) data;  
    rivet_thread_private*   private;

    if (apr_threadkey_private_get ((void **)&private,thread_key) != APR_SUCCESS)
    {
        return;
    } 

    private->keep_going = 0;
    apr_threadkey_private_set (private,rivet_thread_key);

}

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    Tcl_Interp*             interp;
    rivet_thread_private*   private;
    Tcl_Channel             *outchannel;		    /* stuff for buffering output */
    apr_threadkey_t*        thread_key = (apr_threadkey_t *) data;  

    if (apr_threadkey_private_get ((void **)&private,thread_key) != APR_SUCCESS)
    {
        return NULL;
    } 
    else 
    {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals.pool_mutex);

            private             = apr_palloc (module_globals.pool,sizeof(*private));
            private->channel    = apr_pcalloc (module_globals.pool, sizeof(Tcl_Channel));
            private->req_cnt    = 0;
            private->keep_going = 2;
            apr_thread_mutex_unlock(module_globals.pool_mutex);
        }

    }

    interp = Tcl_CreateInterp();
    if (Tcl_Init(interp) == TCL_ERROR)
    {
        return NULL;
    }
    private->interp  = interp;

    outchannel  = private->channel;
    *outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

        /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*outchannel, TCL_STDOUT);

        /* Set the output buffer size to the largest allowed value, so that we 
         * won't send any result packets to the browser unless the Rivet
         * programmer does a "flush stdout" or the page is completed.
         */

    Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);
    private->status  = idle;
    apr_threadkey_private_set (private,rivet_thread_key);
    do
    {
        apr_status_t        rv;
        void*               v;
        apr_queue_t*        q = module_globals.queue;
        handler_private*    request_obj;

        do {

            rv = apr_queue_pop(q, &v);

        } while (rv == APR_EINTR);

        if (rv != APR_SUCCESS) {

            if (rv == APR_EOF) {
                fprintf(stderr, "request_processor: queue terminated APR_EOF\n");
                rv=APR_SUCCESS;
            }
            else 
            {
                fprintf(stderr, "consumer thread exit rv %d\n", rv);
            }
            apr_thread_exit(thd, rv);
            return NULL;

        }

        request_obj = (handler_private *) v;
        if (request_obj->job_type == orderly_exit)
        {
            ap_log_error(APLOG_MARK, APLOG_INFO, APR_EGENERAL, module_globals.server, "consumer thread exit");
            apr_thread_exit(thd, rv);
            return NULL;
        }

        TclWeb_InitRequest(request_obj->req, private->interp, request_obj->r);
        
        private->r   = request_obj->r;
        private->req = request_obj->req;

        request_obj->code = RivetContent (private);

        apr_thread_mutex_lock(request_obj->mutex);
        request_obj->status = done;
        apr_thread_cond_signal(request_obj->cond);
        apr_thread_mutex_unlock(request_obj->mutex);
    
        apr_threadkey_private_set (private,rivet_thread_key);
        private->req_cnt++;
    } while (private->keep_going-- > 0);

    apr_thread_mutex_lock(module_globals.job_mutex);
    *(apr_thread_t **) apr_array_push(module_globals.exiting) = thd;
    apr_thread_cond_signal(module_globals.job_cond);
    apr_thread_mutex_unlock(module_globals.job_mutex);

    //ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, s, MODNAME ": thread %lp exits", thd);
    return NULL;
}

static apr_status_t create_worker_thread (apr_thread_t** thd)
{
    return apr_thread_create(thd, NULL, request_processor, module_globals.queue, module_globals.pool);

}

static void start_thread_pool (int nthreads)
{
    int i;

    for (i = 0; i < nthreads; i++)
    {
        apr_status_t rv;

        rv = create_worker_thread( &module_globals.workers[i]);

        if (rv != APR_SUCCESS) {
            char    errorbuf[512];

            apr_strerror(rv, errorbuf,200);
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals.server, 
                "Error starting request_processor thread (%d) rv=%d:%s\n",i,rv,errorbuf);
            exit(1);

        }
    }
}

static void* APR_THREAD_FUNC supervisor_thread(apr_thread_t *thd, void *data)
{
    server_rec* s = (server_rec *)data;
    //handler_private* private;
    //apr_status_t     rv;

    start_thread_pool(TCL_INTERPS);
    while (1)
    {
        apr_thread_t*   p;
      
        apr_thread_mutex_lock(module_globals.job_mutex);
        while (apr_is_empty_array(module_globals.exiting))
        {
            apr_thread_cond_wait(module_globals.job_cond,module_globals.job_mutex);
        }

        while (!apr_is_empty_array(module_globals.exiting))
        {
            int i;
            p = *(apr_thread_t **)apr_array_pop(module_globals.exiting);
            
            for (i = 0; i < TCL_INTERPS; i++)
            {
                if (p == module_globals.workers[i])
                {
                    apr_status_t rv;

                    ap_log_error(APLOG_MARK,APLOG_INFO,0,s,"thread %d notifies orderly exit",i);

                    /* terminated thread restart */

                    rv = create_worker_thread (&module_globals.workers[i]);
                    if (rv != APR_SUCCESS) {
                        char    errorbuf[512];

                        apr_strerror(rv, errorbuf,200);
                        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals.server, 
                            "Error starting request_processor thread (%d) rv=%d:%s\n",i,rv,errorbuf);
                        exit(1);

                    }
                    
                    break;
                }
            }       
        }   
        apr_thread_mutex_unlock(module_globals.job_mutex);
    }

    /*
    apr_thread_mutex_lock(module_globals.pool_mutex);
    private = apr_palloc(module_globals.pool,sizeof(handler_private));
    private->job_type=orderly_exit;
    apr_thread_mutex_unlock(module_globals.pool_mutex);
    while (1)
    {

        int i;
        apr_sleep(10000000);

        ap_log_error(APLOG_MARK, APLOG_INFO, APR_EGENERAL, s, "restarting worker threads");

        for (i = 0; i < TCL_INTERPS; i++)
        {
            rv = apr_queue_push(module_globals.queue,private);
            if (rv != APR_SUCCESS)
            {
                apr_sleep(100000);
            }
        }

        for (i = 0; i < TCL_INTERPS; i++)
        {
            apr_status_t rv;

            rv = create_worker_thread (&module_globals.workers[i]);
            if (rv != APR_SUCCESS) {
                char    errorbuf[512];

                apr_strerror(rv, errorbuf,200);
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                    "Error starting request_processor thread (%d) rv=%d:%s\n",i,rv,errorbuf);
                exit(1);

            }
        }

    }

    */
    return NULL;
}

static void processor_cleanup (void *data)
{
    rivet_thread_private*   private = (rivet_thread_private *) data;

    Tcl_UnregisterChannel(private->interp,*private->channel);
    Tcl_DeleteInterp(private->interp);

    ap_log_error(APLOG_MARK, APLOG_INFO, 0, module_globals.server, 
                    "Thread exiting after %d requests served", private->req_cnt);
}

static void
RivetChildInit (apr_pool_t *pChild, server_rec *s)
{
    apr_status_t rv;

    apr_thread_mutex_create(&module_globals.job_mutex, APR_THREAD_MUTEX_UNNESTED, pChild);
    apr_thread_cond_create(&module_globals.job_cond, pChild);
    module_globals.exiting = apr_array_make(pChild,100,sizeof(apr_thread_t*));

    apr_thread_mutex_create(&module_globals.pool_mutex, APR_THREAD_MUTEX_UNNESTED, pChild);
    apr_threadkey_private_create (&rivet_thread_key, processor_cleanup, pChild);
    apr_threadkey_private_create (&handler_thread_key, NULL, pChild);
    module_globals.server = s;

    apr_thread_mutex_lock(module_globals.pool_mutex);
    if (apr_pool_create(&module_globals.pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }
    apr_thread_mutex_unlock(module_globals.pool_mutex);

    if (apr_queue_create(&module_globals.queue, MOD_RIVET_QUEUE_SIZE, module_globals.pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": could not initialize mod_rivet request queue");
        exit(1);
    }

    rv = apr_thread_create( &module_globals.supervisor, NULL, 
                            supervisor_thread, s, module_globals.pool);

    if (rv != APR_SUCCESS) {
        char    errorbuf[512];

        apr_strerror(rv, errorbuf,200);
        fprintf(stderr, "Error starting supervisor thread rv=%d:%s\n",rv,errorbuf);
        exit(1);

    }

    Tcl_CreateExitHandler(tcl_exit_handler,(ClientData)rivet_thread_key);
}

static int RivetHandler(request_rec *r)    
{
    handler_private*    private;
    apr_status_t        rv;

    if (apr_threadkey_private_get ((void **)&private,handler_thread_key) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                     MODNAME ": cannot get private data for processor thread");
        exit(1);

    } else {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals.pool_mutex);
            private         = apr_palloc(module_globals.pool,sizeof(handler_private));
            private->req    = TclWeb_NewRequestObject (module_globals.pool);
            apr_thread_cond_create(&(private->cond), module_globals.pool);
            apr_thread_mutex_create(&(private->mutex), APR_THREAD_MUTEX_UNNESTED, module_globals.pool);
            apr_thread_mutex_unlock(module_globals.pool_mutex);
            private->job_type = request;
        }

    }

    private->r      = r;
    private->code   = OK;
    private->status = init;
    apr_threadkey_private_set (private,handler_thread_key);

    do
    {

        rv = apr_queue_push(module_globals.queue,private);
        if (rv != APR_SUCCESS)
        {
            apr_sleep(100000);
        }

    } while (rv != APR_SUCCESS);

    apr_thread_mutex_lock(private->mutex);
    while (private->status != done)
    {
        apr_thread_cond_wait(private->cond,private->mutex);
    }
    apr_thread_mutex_unlock(private->mutex);

    return private->code;
}

static void rivet_register_hooks(apr_pool_t *p)
{
    ap_hook_handler     (RivetHandler,   NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init  (RivetChildInit, NULL, NULL, APR_HOOK_LAST);
    ap_hook_post_config (RivetServerInit, NULL, NULL, APR_HOOK_LAST);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA rivet_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                /* create per-dir    config structures */
    NULL,                /* merge  per-dir    config structures */
    NULL,                /* create per-server config structures */
    NULL,                /* merge  per-server config structures */
    NULL,                /* table of config file commands       */
    rivet_register_hooks /* register hooks                      */
};

