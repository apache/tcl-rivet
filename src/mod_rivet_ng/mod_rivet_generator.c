/* mod_rivet_generator.c -- Content generation functions */

/*
    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/

/* $Id$ */

#include <httpd.h>
#include <tcl.h>
#include <apr_strings.h>

#include "mod_rivet.h"
#include "rivetParser.h"
#include "rivetCore.h"
#include "apache_config.h"
#include "TclWeb.h"

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "mod_rivet_common.h"
#include "mod_rivet_cache.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;


/* 
 * -- Rivet_CheckType (request_rec *r)
 *
 * Utility function internally used to determine which type
 * of file (whether rvt template or plain Tcl script) we are
 * dealing with. In order to speed up multiple tests the
 * the test returns an integer (RIVET_TEMPLATE) for rvt templates
 * or RIVET_TCLFILE for Tcl scripts
 *
 * Argument: 
 *
 *    request_rec*: pointer to the current request record
 *
 * Returns:
 *
 *    integer number meaning the type of file we are dealing with
 *
 * Side effects:
 *
 *    none.
 *
 */

rivet_req_ctype
Rivet_CheckType (request_rec *req)
{
    rivet_req_ctype ctype = CTYPE_NOT_HANDLED;

    if ( req->handler != NULL ) {
        if (STRNEQU( req->handler, RIVET_TEMPLATE_CTYPE) ) {
            ctype  = RIVET_TEMPLATE;
        } else if ( STRNEQU( req->handler, RIVET_TCLFILE_CTYPE) ) {
            ctype = RIVET_TCLFILE;
        }
    }
    return ctype; 
}

/*
 * -- Rivet_ReleaseScript
 *
 *
 * 
 *
 *
 */

static void
Rivet_ReleaseScripts (running_scripts* scripts)
{
    if (scripts->rivet_before_script) Tcl_DecrRefCount(scripts->rivet_before_script);
    if (scripts->rivet_after_script) Tcl_DecrRefCount(scripts->rivet_after_script);
    if (scripts->rivet_error_script) Tcl_DecrRefCount(scripts->rivet_error_script);
    if (scripts->rivet_abort_script) Tcl_DecrRefCount(scripts->rivet_abort_script);
    if (scripts->after_every_script) Tcl_DecrRefCount(scripts->after_every_script);
}

/*
 * -- Rivet_SendContent
 *
 *   Set things up to execute a Tcl script or parse a rvt template, prepare
 *   the environment then execute it as a pure Tcl script
 *
 */

#define USE_APACHE_RSC

DLLEXPORT int
Rivet_SendContent(rivet_thread_private *private,request_rec* r)
{
    int                     errstatus;
    int                     retval;
    Tcl_Interp*             interp;
    rivet_thread_interp*    interp_obj;
    Tcl_Channel*            running_channel;

#ifdef USE_APACHE_RSC
    //rivet_server_conf    *rsc = NULL;
#else
    //rivet_server_conf    *rdc;
#endif

    private->r = r;

    /* Set the global request req to know what we are dealing with in
     * case we have to call the PanicProc. */

    /* TODO: we can't place a pointer to the request rec here, if Tcl_Panic 
       gets called in general it won't have this pointer which has to be 
       thread private */

    private->rivet_panic_request_rec = private->r;

    // rsc = Rivet_GetConf(r);

    private->running_conf = RIVET_SERVER_CONF (private->r->server->module_config);

    /* the interp index in the private data can not be changed by a config merge */

    interp_obj = RIVET_PEEK_INTERP(private,private->running_conf);
    private->running = interp_obj->scripts;
    running_channel = interp_obj->channel;

    if (private->r->per_dir_config)
    {
        rivet_server_conf* rdc = NULL;

        rdc = RIVET_SERVER_CONF(private->r->per_dir_config); 

        if ((rdc != NULL) && (rdc->path))
        {
            /* Let's check if a scripts object is already stored in the per-dir hash table */

            private->running = 
                (running_scripts *) apr_hash_get (interp_obj->per_dir_scripts,rdc->path,strlen(rdc->path));

            if (private->running == NULL)
            {
                rivet_server_conf*  newconfig   = NULL;
                running_scripts*    scripts     = 
                            (running_scripts *) apr_pcalloc (private->pool,sizeof(running_scripts));

                newconfig = RIVET_NEW_CONF(private->r->pool);

                Rivet_CopyConfig (private->running_conf,newconfig);
                Rivet_MergeDirConfigVars (private->r->pool,newconfig,private->running_conf,rdc);
                private->running_conf = newconfig;

                scripts = Rivet_RunningScripts (private->pool,scripts,newconfig);

                apr_hash_set (interp_obj->per_dir_scripts,rdc->path,strlen(rdc->path),scripts);
               
                private->running = scripts;
            }
        }

        if (USER_CONF_UPDATED(rdc))
        {
            rivet_server_conf*  newconfig   = NULL;
            private->running = (running_scripts *) apr_pcalloc (private->pool,sizeof(running_scripts));

            newconfig = RIVET_NEW_CONF(private->r->pool);

            Rivet_CopyConfig( private->running_conf, newconfig );
            Rivet_MergeDirConfigVars( private->r->pool, newconfig, private->running_conf, rdc );
            private->running_conf = newconfig;

            private->running = Rivet_RunningScripts(private->r->pool,private->running,newconfig);

        }
    }
    else
    {
        /* if no <Directory ...> rules applies we use the server configuration */

        private->running = interp_obj->scripts;
    }

    interp  = interp_obj->interp;

#ifndef USE_APACHE_RSC
    if (private->r->per_dir_config != NULL)
        rdc = RIVET_SERVER_CONF( private->r->per_dir_config );
    else
        rdc = rsc;
#endif

    private->r->allowed |= (1 << M_GET);
    private->r->allowed |= (1 << M_POST);
    private->r->allowed |= (1 << M_PUT);
    private->r->allowed |= (1 << M_DELETE);
    if (private->r->method_number != M_GET   && 
        private->r->method_number != M_POST  && 
        private->r->method_number != M_PUT   && 
        private->r->method_number != M_DELETE) {

        retval = DECLINED;
        goto sendcleanup;

    }

    if (private->r->finfo.filetype == 0)
    {
        request_rec* r = private->r;

        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, 
                     private->r->server,
                     MODNAME ": File does not exist: %s",
                     (r->path_info ? (char*)apr_pstrcat(r->pool, r->filename, r->path_info, NULL) : r->filename));
        retval = HTTP_NOT_FOUND;
        goto sendcleanup;
    }

    if ((errstatus = ap_meets_conditions(private->r)) != OK) {
        retval = errstatus;
        goto sendcleanup;
    }

    /* 
     * This one is the big catch when it comes to moving towards
     * Apache 2.0, or one of them, at least.
     */

    if (Rivet_chdir_file(private->r->filename) < 0)
    {
        request_rec* r = private->r;

        /* something went wrong doing chdir into r->filename, we are not specific
         * at this. We simply emit an internal server error and print a log message
         */
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": Error accessing %s, could not chdir into directory", 
                     r->filename);

        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }

    /*
    if (Tcl_EvalObjEx(interp, private->request_init, 0) == TCL_ERROR)
    {
        request_rec* r = private->r;

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                            MODNAME ": Could not create request namespace (%s)\n" ,
                            Tcl_GetStringResult(interp));

        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }
    */

    /* Apache Request stuff */

    TclWeb_InitRequest(private, interp);
    ApacheRequest_set_post_max(private->req->apachereq, private->running_conf->upload_max);
    ApacheRequest_set_temp_dir(private->req->apachereq, private->running_conf->upload_dir);

    /* Let's copy the request data into the thread private record */

    errstatus = ApacheRequest_parse(private->req->apachereq);
    if (errstatus != OK) {
        retval = errstatus;
        goto sendcleanup;
    }

    if (private->r->header_only && !private->running_conf->honor_header_only_reqs)
    {
        TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, private->req);
        TclWeb_PrintHeaders(private->req);
        retval = OK;
        goto sendcleanup;
    }

    /* If the user configuration has indeed been updated, I guess that
       pretty much invalidates anything that might have been cached. */

    /* This is all horrendously slow, and means we should *also* be
       doing caching on the modification time of the .htaccess files
       that concern us. FIXME */

    if (USER_CONF_UPDATED(private->running_conf) && (interp_obj->cache_size != 0) && 
                                                    (interp_obj->cache_free < interp_obj->cache_size)) 
    {
        RivetCache_Cleanup(private,interp_obj);
    }

    /* URL referenced script execution and exception handling */

    if (Tcl_EvalObjEx(interp, private->running->request_processing,0) == TCL_ERROR) 
    //if (Rivet_ParseExecFile (private, private->r->filename, 1) != TCL_OK)
    //if (Rivet_ExecuteAndCheck(private,private->request_processing) == TCL_ERROR)
    {
        /* we don't report errors coming from abort_page execution */

        if (!private->page_aborting) 
        {
            request_rec* r = private->r;

            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                         MODNAME ": Error parsing exec file '%s': %s",
                         r->filename, Tcl_GetVar(interp, "errorInfo", 0));
        }
    }

    /* We don't keep user script until we find a way to cache them consistently */

    if (IS_USER_CONF(private->running_conf))
    {
        Rivet_ReleaseScripts(private->running);
        private->running_conf->user_scripts_status &= ~(unsigned int)USER_SCRIPTS_UPDATED;
    }

    /* and finally we run the request_cleanup procedure (always set) */
    
    //if (Tcl_EvalObjEx(interp, private->request_cleanup, 0) == TCL_ERROR) {
    //
    //    ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, private->r, 
    //                 MODNAME ": Error evaluating cleanup request: %s",
    //                 Tcl_GetVar(interp, "errorInfo", 0));
    //
    //}

    /* We finalize the request processing by printing the headers and flushing
       the rivet channel internal buffer */

    TclWeb_PrintHeaders(private->req);
    Tcl_Flush(*(running_channel));

    /* Reset globals */
    Rivet_CleanupRequest(private->r);

    retval = OK;
sendcleanup:

    /* Request processing final stage */

    /* A new big catch is the handling of exit commands that are treated
     * as ::rivet::abort_page. After the AbortScript has been evaluated
     * the exit condition is checked and in case the exit handler
     * of the bridge module is called before terminating the whole process
     */
    
    if (private->thread_exit)
    {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, private->r, 
                                  "process terminating with code %d",private->exit_status);
        RIVET_MPM_BRIDGE_CALL(mpm_exit_handler,private->exit_status);
        //Tcl_Exit(private->exit_status);
        exit(private->exit_status);
    }

    /* We now reset the status to prepare the child process for another request */

    private->req->content_sent = 0;
    if (private->abort_code != NULL)
    {
        Tcl_DecrRefCount(private->abort_code);
        private->abort_code = NULL;
    }
    private->page_aborting = 0;

    /* We reset this pointer to signal we have terminated the request processing */

    private->r = NULL;
    return retval;
}
