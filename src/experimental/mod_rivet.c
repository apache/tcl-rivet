
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include <ap_config.h>
#include <apr_queue.h>
#include <apr_strings.h>
#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>

#include "TclWeb.h"
#include "mod_rivet.h"
#include "rivetChannel.h"

#define RIVET_TEMPLATE_CTYPE    "application/x-httpd-rivet"
#define RIVET_TCLFILE_CTYPE     "application/x-rivet-tcl"
#define MODNAME                 "mod_rivet"
#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024*1024)

mod_rivet_globals       module_globals;
rivet_server_conf       rsc;
rivet_interp_globals    interp_globals;

extern Tcl_ChannelType RivetChan;

int
Rivet_ParseExecString (TclWebRequest* req, Tcl_Obj* inbuf)
{
    return TCL_OK;
}

int
Rivet_ParseExecFile(TclWebRequest *req, char *filename, int toplevel)
{
    return TCL_OK;
}

static void RivetReleaseInterp(int interp_id)
{
    apr_thread_mutex_lock(module_globals.mutex);
    module_globals.status[interp_id] = idle;
    module_globals.busy_cnt--;
    apr_thread_cond_signal(module_globals.cond);
    apr_thread_mutex_unlock(module_globals.mutex);
}

static int RivetFetchInterp(Tcl_Interp** interp_p, request_rec* req)
{
    int c;
    int interp_id;
    int interp_found = 0;

    apr_thread_mutex_lock(module_globals.mutex);
    do     
    {
        while (module_globals.busy_cnt >= TCL_INTERPS)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, req->server, 
                     MODNAME ": no available interpreter from the pool.");
            apr_thread_cond_wait(module_globals.cond, module_globals.mutex);
        }

        for (c = 0; c < TCL_INTERPS; c++)
        {
            interp_id = module_globals.interp_idx + c + 1;
            interp_id %= TCL_INTERPS;
            if (module_globals.status[interp_id] == idle)
            {
                interp_found = 1;
                module_globals.interp_idx = interp_id;
                module_globals.status[interp_id] = request_processing;
                module_globals.busy_cnt++;
                *interp_p = module_globals.interp_a[interp_id];
                break;
            }
        }

    // we should assert here an idle interpreter has been found, otherwise
    // the busy_cnt was inconsistent.

    } while (!interp_found);
    apr_thread_mutex_unlock(module_globals.mutex);
 
    return interp_id;
}

static int
RivetServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    ap_add_version_component(pPool, "Rivet/Experimental");
    return OK;
}

static void
RivetChildInit (apr_pool_t *pChild, server_rec *s)
{
    int i;
    Tcl_Channel *outchannel;		    /* stuff for buffering output */

    if (apr_pool_create(&module_globals.pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }

    apr_thread_mutex_create(&module_globals.mutex, APR_THREAD_MUTEX_UNNESTED, pChild);
    apr_thread_mutex_create(&module_globals.channel_mutex, APR_THREAD_MUTEX_UNNESTED, pChild);
    apr_thread_cond_create(&module_globals.cond, pChild);

    for (i = 0; i < TCL_INTERPS; i++)
    {
        Tcl_Interp* interp;
        
        interp = Tcl_CreateInterp();
        if (Tcl_Init(interp) == TCL_ERROR)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                         MODNAME ": Error in Tcl_Init: %s, aborting\n",
                         Tcl_GetStringResult(interp));
            exit(1);
        }
        module_globals.interp_a[i] = interp;
        module_globals.status[i]   = idle;
    }

    outchannel  = apr_pcalloc (pChild, sizeof(Tcl_Channel));
    *outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", &module_globals, TCL_WRITABLE);

    /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*outchannel, TCL_STDOUT);

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);

    module_globals.outchannel = outchannel;

    for (i = 0; i < TCL_INTERPS; i++)
    {
        Tcl_Preserve (module_globals.interp_a[i]);

        /* We register the Tcl channel to the interpreter */

        Tcl_RegisterChannel(module_globals.interp_a[i], *outchannel);
        Tcl_Release (module_globals.interp_a[i]);
    }
    module_globals.interp_idx = 0;
    module_globals.req = TclWeb_NewRequestObject (pChild);
}

static int RivetHandler(request_rec *r)
{
    Tcl_Interp* interp;
    Tcl_Obj*    script = NULL;
    int         interp_id;

    if (strcmp(r->handler, "application/x-httpd-rivet")) {
        return DECLINED;
    }

    if (r->method_number != M_GET) { return DECLINED; }

    r->content_type = "text/html";
    if (r->header_only) return OK;
    //if (r->args == 0) return OK;

    interp_id = RivetFetchInterp(&interp,r);
    Tcl_Preserve(module_globals.interp_a[interp_id]);

    /*
    script = Tcl_NewStringObj("puts \"<html><head><title>experimental</title></head><body>OK</body></html>\"\n",-1);
    */

    script = Tcl_NewStringObj("puts \"<html><head><title>experimental module</title></head><body><h2>\"\n",-1);
    Tcl_AppendStringsToObj(script,"puts -nonewline \"current interp index: \"\nputs ",NULL);
    Tcl_AppendObjToObj(script,Tcl_NewIntObj(interp_id));
    Tcl_AppendStringsToObj(script,"\nputs \"</h2></body></html>\"",NULL);

    apr_thread_mutex_lock(module_globals.channel_mutex);

    Tcl_SetStdChannel (*(module_globals.outchannel), TCL_STDOUT);

    TclWeb_InitRequest(module_globals.req,interp,r);
    module_globals.r = r;

    if (Tcl_EvalObjEx(interp, script, 0) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                     "Error evaluating script: %s", Tcl_GetVar(interp, "errorInfo", 0));
    }

    TclWeb_PrintHeaders(module_globals.req);
    Tcl_Flush(*(module_globals.outchannel));
    Tcl_Release(module_globals.interp_a[interp_id]);

    apr_thread_mutex_unlock(module_globals.channel_mutex);
    RivetReleaseInterp(interp_id);

    return OK;
}

static void rivet_register_hooks(apr_pool_t *p)
{
    ap_hook_handler(RivetHandler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(RivetChildInit, NULL, NULL, APR_HOOK_LAST);
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

