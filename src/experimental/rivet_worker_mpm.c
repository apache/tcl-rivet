
#include "mod_rivet.h"
#include "httpd.h"
#include "rivetChannel.h"

extern mod_rivet_globals module_globals;
extern apr_threadkey_t*        rivet_thread_key;
extern apr_threadkey_t*        handler_thread_key;

static void processor_cleanup (void *data)
{
    rivet_thread_private*   private = (rivet_thread_private *) data;

    Tcl_UnregisterChannel(private->interp,*private->channel);
    Tcl_DeleteInterp(private->interp);

    ap_log_error(APLOG_MARK, APLOG_INFO, 0, module_globals.server, 
            "Thread exiting after %d requests served", private->req_cnt);
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

    apr_thread_exit(thd,APR_SUCCESS);
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

                        apr_strerror(rv,errorbuf,200);
                        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals.server, 
                            "Error starting request_processor thread (%d) rv=%d:%s",i,rv,errorbuf);
                        exit(1);

                    }
                    
                    break;
                }
            }       
        }   
        apr_thread_mutex_unlock(module_globals.job_mutex);
    }

    return NULL;
}

void Rivet_MPM_Init (apr_pool_t* pool, server_rec* server)
{
    apr_status_t rv;

    apr_thread_mutex_create(&module_globals.job_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_thread_cond_create(&module_globals.job_cond, pool);
    module_globals.exiting = apr_array_make(pool,100,sizeof(apr_thread_t*));

    apr_thread_mutex_create(&module_globals.pool_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_threadkey_private_create (&rivet_thread_key, processor_cleanup, pool);
    apr_threadkey_private_create (&handler_thread_key, NULL, pool);
    module_globals.server = server;

    apr_thread_mutex_lock(module_globals.pool_mutex);
    if (apr_pool_create(&module_globals.pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }
    apr_thread_mutex_unlock(module_globals.pool_mutex);

    if (apr_queue_create(&module_globals.queue, MOD_RIVET_QUEUE_SIZE, module_globals.pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet request queue");
        exit(1);
    }

    rv = apr_thread_create( &module_globals.supervisor, NULL, 
                            supervisor_thread, server, module_globals.pool);

    if (rv != APR_SUCCESS) {
        char    errorbuf[512];

        apr_strerror(rv, errorbuf,200);
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME "Error starting supervisor thread rv=%d:%s\n",rv,errorbuf);
        exit(1);
    }

}

int Rivet_MPM_Request (request_rec* r)
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

void Rivet_MPM_Finalize (void)
{

}
