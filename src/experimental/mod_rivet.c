/* mod_rivet.c -- The apache module itself, for Apache 2.4. */

/* Copyright 2000-2005 The Apache Software Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   	http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/* $Id: mod_rivet.h 1609472 2014-07-10 15:08:52Z mxmanghi $ */

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

mod_rivet_globals       module_globals;
rivet_server_conf       rsc;
rivet_interp_globals    interp_globals;

extern Tcl_ChannelType RivetChan;
apr_threadkey_t*        rivet_thread_key;
apr_threadkey_t*        handler_thread_key;

#define ERRORBUF_SZ     256

static int
RivetServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    apr_status_t aprrv;
    char errorbuf[ERRORBUF_SZ];

    ap_add_version_component(pPool, "Rivet/Experimental/"__DATE__"/"__TIME__"/");
    
    aprrv = apr_dso_load(&module_globals.dso_handle,"/home/manghi/Projects/rivet/src/experimental/.libs/rivet_worker_mpm.so",pPool);
    if (aprrv == APR_SUCCESS)
    {
        apr_status_t                rv;
        apr_dso_handle_sym_t        func = NULL;

        rv = apr_dso_sym(&func,module_globals.dso_handle,"Rivet_MPM_Init");
        if (rv == APR_SUCCESS)
        {
            module_globals.mpm_init = (int (*)(apr_pool_t*,server_rec*)) func;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME " Error loading symbol Rivet_MPM_Init: %s", 
                         apr_dso_error(module_globals.dso_handle,errorbuf,ERRORBUF_SZ));
        }

        rv = apr_dso_sym(&func,module_globals.dso_handle,"Rivet_MPM_Request");
        if (rv == APR_SUCCESS)
        {
            module_globals.mpm_request = (int (*)(request_rec*)) func;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME " Error loading symbol Rivet_MPM_Request: %s", 
                         apr_dso_error(module_globals.dso_handle,errorbuf,ERRORBUF_SZ));
        }


    }
    else
    {

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME " Error loading MPM manager: %s", 
                     apr_dso_error(module_globals.dso_handle,errorbuf,1024));
        
        return 1;
    }

    return OK;
}

int RivetContent (rivet_thread_private* private)    
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
                     MODNAME ": couldn't get info on %s", r->filename);

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

    /* 
     *  I take this call from the current mod_rivet code, but I'm not sure if it's
     *  necessary in this context
     */

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
    Tcl_DecrRefCount(script);
    return OK;
}

static void RivetChildInit (apr_pool_t *pChild, server_rec *s)
{
    (*module_globals.mpm_init)(pChild,s);
//  Tcl_CreateExitHandler(tcl_exit_handler,(ClientData)rivet_thread_key);
}

static int RivetHandler(request_rec *r)    
{
    return (*module_globals.mpm_request)(r);
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

