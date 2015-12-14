/* mod_rivet.c -- The apache module itself, for Apache 2.4. */

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

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

/* Apache includes */
#include <httpd.h>
#include <http_config.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_log.h>
#include <http_main.h>
#include <ap_config.h>
#include <ap_mpm.h>
#include <apr_queue.h>
#include <apr_strings.h>
#include <apr_general.h>
#include <apr_time.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_env.h>

/* Tcl includes */
#include <tcl.h>

/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include "rivet_types.h"
#include "mod_rivet.h"
#include "rivetChannel.h"
#include "apache_config.h"
#include "TclWeb.h"
#include "rivet.h"
#include "mod_rivet_common.h"
#include "rivetParser.h"

rivet_interp_globals    interp_globals;

extern Tcl_ChannelType   RivetChan;
apr_threadkey_t*         rivet_thread_key    = NULL;
mod_rivet_globals*       module_globals      = NULL;

void        Rivet_PerInterpInit (rivet_thread_interp* interp, 
                                 rivet_thread_private* private, 
                                 server_rec *s, apr_pool_t *p );
static int  Rivet_ExecuteAndCheck (rivet_thread_private *private, Tcl_Obj *tcl_script_obj);
int         Rivet_InitCore (Tcl_Interp *interp,rivet_thread_private* p); 

#define ERRORBUF_SZ     256

/*
 * -- Rivet_Exit_Handler
 *
 * 
 *
 */

int Rivet_Exit_Handler(int code)
{
    //Tcl_Exit(code);
    /*NOTREACHED*/
    return TCL_OK;		/* Better not ever reach this! */
}

/* -- Rivet_PrintErrorMessage
 *
 * Utility function to print the error message stored in errorInfo
 * with a custom header. This procedure is called to print standard
 * errors when one of Tcl scripts fails
 * 
 * Arguments:
 *
 *   - Tcl_Interp* interp: the Tcl interpreter that was running the script
 *                         (and therefore the built-in variable errorInfo
 *                          keeps the message)
 *   - const char* error: Custom error header
 */

static void 
Rivet_PrintErrorMessage (Tcl_Interp* interp,const char* error_header)
{
    Tcl_Obj* errormsg  = Tcl_NewObj();

    Tcl_IncrRefCount(errormsg);
    Tcl_AppendStringsToObj(errormsg,"puts \"",error_header,"<br />\"\n",NULL);
    Tcl_AppendStringsToObj(errormsg,"puts \"<pre>$errorInfo</pre>\"\n",NULL);
    Tcl_EvalObjEx(interp,errormsg,0);
    Tcl_DecrRefCount(errormsg);
}


/*----------------------------------------------------------------------------
 * -- Rivet_RunningScripts
 *
 *
 *
 *-----------------------------------------------------------------------------
 */

static running_scripts*
Rivet_RunningScripts (  apr_pool_t*         pool, 
                        running_scripts*    scripts, 
                        rivet_server_conf*  rivet_conf)
{
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_before_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_after_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_error_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_abort_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,after_every_script);

    return scripts;
}

/*-----------------------------------------------------------------------------
 * -- Rivet_ReleaseScript
 *
 *
 * 
 *
 *-----------------------------------------------------------------------------
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
 *-----------------------------------------------------------------------------
 * Rivet_CreateCache --
 *
 * Arguments:
 *     rsc: pointer to a server_rec structure
 *
 * Results:
 *     None
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------------
 */

static void Rivet_CreateCache (apr_pool_t *p, rivet_thread_interp* interp_obj)
{
    interp_obj->objCacheList = apr_pcalloc(p, (signed)((interp_obj->cache_size)*sizeof(char *)));
    interp_obj->objCache = apr_pcalloc(p, sizeof(Tcl_HashTable));
    Tcl_InitHashTable(interp_obj->objCache, TCL_STRING_KEYS);
}

/*
 * Rivet_DuplicateVhostInterp
 *
 *
 */

static rivet_thread_interp* 
Rivet_DuplicateVHostInterp(apr_pool_t* pool, rivet_thread_interp* source_obj)
{
    rivet_thread_interp* interp_obj = apr_pcalloc(pool,sizeof(rivet_thread_interp));

    interp_obj->interp      = source_obj->interp;
    interp_obj->channel     = source_obj->channel;
    interp_obj->cache_free  = source_obj->cache_free;
    interp_obj->cache_size  = source_obj->cache_size;

    /* TODO: decouple cache by creating a new cache object */

    if (interp_obj->cache_size) {
        Rivet_CreateCache(pool,interp_obj); 
    }

    interp_obj->pool            = source_obj->pool;
    interp_obj->scripts         = (running_scripts *) apr_pcalloc(pool,sizeof(running_scripts));
    interp_obj->per_dir_scripts = apr_hash_make(pool); 
    interp_obj->flags           = source_obj->flags;
    return interp_obj;
}

 /* XXX: the pool passed to Rivet_NewVHostInterp must be the private pool of
  * a rivet_thread_private object 
  */

rivet_thread_interp* Rivet_NewVHostInterp(apr_pool_t *pool)
{
    extern int              ap_max_requests_per_child;
    rivet_thread_interp*    interp_obj = apr_pcalloc(pool,sizeof(rivet_thread_interp));
    rivet_server_conf*      rsc;

    /* The cache size is global so we take it from here */
    
    rsc = RIVET_SERVER_CONF (module_globals->server->module_config);

    /* This calls needs the root server_rec just for logging purposes*/

    interp_obj->interp = Rivet_CreateTclInterp(module_globals->server); 

    /* We now read from the pointers to the cache_size and cache_free conf parameters
     * for compatibility with mod_rivet current version, but these values must become
     * integers not pointers
     */
    
    if (rsc->default_cache_size < 0) {
        if (ap_max_requests_per_child != 0) {
            interp_obj->cache_size = ap_max_requests_per_child / 5;
        } else {
            interp_obj->cache_size = 50;    // Arbitrary number
        }
    } else if (rsc->default_cache_size > 0) {
        interp_obj->cache_size = rsc->default_cache_size;
    }

    if (interp_obj->cache_size > 0) {
        interp_obj->cache_free = interp_obj->cache_size;
    }

    /* we now create memory from the cache pool as subpool of the thread private pool */
 
    if (apr_pool_create(&interp_obj->pool, pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                     MODNAME ": could not initialize cache private pool");
        return NULL;
    }

    // Initialize cache structures

    if (interp_obj->cache_size) {
        Rivet_CreateCache(pool,interp_obj); 
    }

    interp_obj->flags = 0;

    interp_obj->scripts         = (running_scripts *) apr_pcalloc(pool,sizeof(running_scripts));
    interp_obj->per_dir_scripts = apr_hash_make(pool); 

    return interp_obj;
}

/* -- Rivet_VirtualHostsInterps 
 *
 * The server_rec chain is walked through and server configurations is read to
 * set up the thread private configuration and interpreters database
 *
 *  Arguments:
 *
 *      rivet_thread_private* private;
 *
 *  Returned value:
 *
 *     a new rivet_thread_private object
 * 
 *  Side effects:
 *
 *     GlobalInitScript and ChildInitScript are run at this stage
 *     
 */

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private)
{
    server_rec*         s;
    server_rec*         root_server = module_globals->server;
    rivet_server_conf*  root_server_conf;
    rivet_server_conf*  myrsc; 
    rivet_thread_interp* root_interp;
    void*               parentfunction;     /* this is topmost initialization script */
    void*               function;

    root_server_conf = RIVET_SERVER_CONF (root_server->module_config);
    
    root_interp = (*RIVET_MPM_BRIDGE_FUNCTION(mpm_master_interp))();

    /* we must assume the module was able to create the root interprter */ 

    ap_assert (root_interp != NULL);

    /* Using the root interpreter we evaluate the global initialization script, if any */

    if (root_server_conf->rivet_global_init_script != NULL) {
        Tcl_Obj* global_tcl_script;

        global_tcl_script = Tcl_NewStringObj(root_server_conf->rivet_global_init_script,-1);
        Tcl_IncrRefCount(global_tcl_script);
        if (Tcl_EvalObjEx(root_interp->interp, global_tcl_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                         MODNAME ": Error running GlobalInitScript '%s': %s",
                         root_server_conf->rivet_global_init_script,
                         Tcl_GetVar(root_interp->interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server, 
                         MODNAME ": GlobalInitScript '%s' successful",
                         root_server_conf->rivet_global_init_script);
        }
        Tcl_DecrRefCount(global_tcl_script);
    }

    /* then we proceed assigning/creating the interpreters for the
     * virtual hosts known to the server
     */

    parentfunction = root_server_conf->rivet_child_init_script;

    for (s = root_server; s != NULL; s = s->next)
    {
        rivet_thread_interp*   rivet_interp;

        myrsc = RIVET_SERVER_CONF(s->module_config);

        /* by default we assign the root_interpreter as
         * interpreter of the virtual host. In case of separate
         * virtual interpreters we create new ones for each
         * virtual host 
         */

        rivet_interp = root_interp;

        if (s == root_server)
        {
            Tcl_RegisterChannel(rivet_interp->interp,*rivet_interp->channel);
        }
        else 
        {
            if (root_server_conf->separate_virtual_interps)
            {
                rivet_interp = Rivet_NewVHostInterp(private->pool);
                if (myrsc->separate_channels)
                {
                    rivet_interp->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
                    Tcl_RegisterChannel(rivet_interp->interp,*rivet_interp->channel);
                }
                else
                {
                    rivet_interp->channel = private->channel;
                }
            }
            else
            {
                rivet_interp = Rivet_DuplicateVHostInterp(private->pool,root_interp);           
            }
        }

        /* interpreter base running scripts definition and initialization */

        rivet_interp->scripts = Rivet_RunningScripts (private->pool,rivet_interp->scripts,myrsc);

        private->interps[myrsc->idx] = rivet_interp;

        /* Basic Rivet packages and libraries are loaded here */

        if ((rivet_interp->flags & RIVET_INTERP_INITIALIZED) == 0)
        {
            Rivet_PerInterpInit(rivet_interp, private, root_server, private->pool);
        }

        /*  TODO: check if it's absolutely necessary to lock the pool_mutex in order
         *  to allocate from the module global pool
         */

        /*  this stuff must be allocated from the module global pool which
         *  has the child process' same lifespan
         */

        apr_thread_mutex_lock(module_globals->pool_mutex);
        myrsc->server_name = (char*) apr_pstrdup (private->pool, s->server_hostname);
        apr_thread_mutex_unlock(module_globals->pool_mutex);

        /* when configured a child init script gets evaluated */

        function = myrsc->rivet_child_init_script;
        if (function && 
            (s == root_server || root_server_conf->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Interp* interp = rivet_interp->interp;
            Tcl_Obj*    tcl_child_init = Tcl_NewStringObj(function,-1);

            Tcl_IncrRefCount(tcl_child_init);
            Tcl_Preserve (interp);

            /* There is a lot of passing around of pointers among various record 
             * objects. We should understand if this is all that necessary.
             * Here we assign the server_rec pointer to the interpreter which
             * is wrong, because without separate interpreters it doens't make
             * any sense. TODO
             */

            if (Tcl_EvalObjEx(interp,tcl_child_init, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server,
                             errmsg, function);
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorCode: %s",
                                Tcl_GetVar(interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorInfo: %s",
                        Tcl_GetVar(interp, "errorInfo", 0));
            }
            Tcl_Release (interp);
            Tcl_DecrRefCount(tcl_child_init);
        }
    }
    return private;
}


/*
 * -- Rivet_SendContent
 *
 *   Set things up to execute a Tcl script or parse a rvt template, prepare
 *   the environment then execute it as a pure Tcl script
 *
 */

#define USE_APACHE_RSC

int
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

    interp_obj = private->interps[private->running_conf->idx];
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

                Rivet_CopyConfig( private->running_conf, newconfig );
                Rivet_MergeDirConfigVars( private->r->pool, newconfig, private->running_conf, rdc );
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

    if (Tcl_EvalObjEx(interp, private->request_init, 0) == TCL_ERROR)
    {
        request_rec* r = private->r;

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                            MODNAME ": Could not create request namespace (%s)\n" ,
                            Tcl_GetStringResult(interp));

        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }

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

/* 
 * if we are handling the request we also want to check if a charset 
 * parameter was set with the content type, e.g. rivet's configuration 
 * or .htaccess had lines like 
 *
 * AddType 'application/x-httpd-rivet; charset=utf-8;' rvt 
 */
 

    if (Rivet_ParseExecFile (private, private->r->filename, 1) != TCL_OK)
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

    /* We execute also the AfterEveryScript if one was set */

    if (private->running->after_every_script) 
    {
        //if (Tcl_EvalObjEx(interp_obj->interp,private->running->after_every_script,0) == TCL_ERROR)
        if (Rivet_ExecuteAndCheck(private,private->running->after_every_script) == TCL_ERROR)
        {
            Rivet_PrintErrorMessage(private->interps[private->running_conf->idx]->interp,
                                    "<b>Rivet AfterEveryScript failed</b>");
        }
    }

    /* and finally we run the request_cleanup procedure (always set) */

    if (Tcl_EvalObjEx(interp, private->request_cleanup, 0) == TCL_ERROR) {

        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, private->r, 
                     MODNAME ": Error evaluating cleanup request: %s",
                     Tcl_GetVar(interp, "errorInfo", 0));

    }

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
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, private->r, 
                                  "process terminating with code %d",private->exit_status);
        RIVET_MPM_BRIDGE_CALL(mpm_exit_handler,private->exit_status);
        //Tcl_Exit(private->exit_status);
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

/* -- Rivet_ExecuteErrorHandler
 *
 * Invoking either the default error handler or the ErrorScript.
 * In case the error handler fails a standard error message is printed
 * (you're better off if you make your error handlers fail save)
 *
 * Arguments:
 *
 *  - Tcl_Interp* interp: the Tcl interpreter
 *  - Tcl_Obj*    tcl_script_obj: the script that failed (to retrieve error
 *                info from it
 *  - request_rec* req: current request obj pointer
 * 
 * Returned value:
 *
 *  - A Tcl status
 */

static int 
Rivet_ExecuteErrorHandler (Tcl_Interp* interp,Tcl_Obj* tcl_script_obj, rivet_thread_private* private)
{
    int                 result;
    Tcl_Obj*            errscript;
    rivet_server_conf*  conf = private->running_conf;

    /* We extract information from the errorOutbuf variable. Notice that tcl_script_obj
     * can either be the request processing script or conf->rivet_abort_script
     */

    Tcl_SetVar( interp, "errorOutbuf",Tcl_GetStringFromObj( tcl_script_obj, NULL ),TCL_GLOBAL_ONLY );

    /* If we don't have an error script, use the default error handler. */
    if (conf->rivet_error_script) {
        errscript = Tcl_NewStringObj(conf->rivet_error_script,-1);
    } else {
        errscript = Tcl_NewStringObj(conf->rivet_default_error_script,-1);
    }

    Tcl_IncrRefCount(errscript);
    result = Tcl_EvalObjEx(interp, errscript, 0);
    if (result == TCL_ERROR) {
        Rivet_PrintErrorMessage(interp,"<b>Rivet ErrorScript failed</b>");
    }

    /* This shouldn't make the default_error_script go away,
     * because it gets a Tcl_IncrRefCount when it is created.
     */

    Tcl_DecrRefCount(errscript);

    /* In case we are handling an error occurring after an abort_page call (for
     * example because the AbortString itself failed) we must reset this
     * flag or the logging will be inihibited
     */

    private->page_aborting = 0;

    return result;
}

/*
 * -- Rivet_RunAbortScript
 *
 * 
 */

static int
Rivet_RunAbortScript (rivet_thread_private *private)
{
    int retcode = TCL_OK;
    Tcl_Interp* interp = private->interps[private->running_conf->idx]->interp;

    if (private->running->rivet_abort_script) 
    {

        /* Ideally an AbortScript should be fail safe, but in case
         * it fails we give a chance to the subsequent ErrorScript
         * to catch this error.
         */

        retcode = Tcl_EvalObjEx(interp,private->running->rivet_abort_script,0);

        if (retcode == TCL_ERROR)
        {
            /* This is not elegant, but we want to avoid to print
             * this error message if an ErrorScript will handle this error.
             * Thus we print the usual error message only if we are running the
             * default error handler
             */

            if (private->running->rivet_error_script == NULL) {

                Rivet_PrintErrorMessage(interp,"<b>Rivet AbortScript failed</b>");

            }
            Rivet_ExecuteErrorHandler(interp,private->running->rivet_abort_script,private);
        }

    }
    return retcode;
}


/* -- Rivet_ExecuteAndCheck
 * 
 * Tcl script execution central procedure. The script stored in
 * outbuf is evaluated and in case an error occurs in the execution
 * an error handler is executed. In case the error code returned
 * is RIVET then the error was caused by the invocation of a 
 * abort_page command and the script stored in conf->abort_script
 * is run istead. The default error script prints the error buffer
 *
 *   Arguments:
 * 
 *      - Tcl_Interp* interp:      the Tcl interpreter 
 *      - Tcl_Obj* tcl_script_obj: a pointer to the Tcl_Obj holding the script
 *      - request_rec* req:        the current request_rec object pointer
 *
 *   Returned value:
 *
 *      - One of the Tcl defined returned value of Tcl_EvelObjExe (TCL_OK, 
 *        TCL_ERROR, TCL_BREAK etc.)
 *
 *   Side effects:
 *
 *      The Tcl interpreter internal status is changed by the execution
 *      of the script
 *
 */

static int
Rivet_ExecuteAndCheck(rivet_thread_private *private, Tcl_Obj *tcl_script_obj)
{
    int           tcl_result;
    rivet_thread_interp* interp_obj;

    interp_obj = private->interps[private->running_conf->idx];
    //rivet_interp_globals *globals = Tcl_GetAssocData(interp_obj->interp, "rivet", NULL);

    Tcl_Preserve (interp_obj->interp);

    tcl_result = Tcl_EvalObjEx(interp_obj->interp, tcl_script_obj, 0);
    if (tcl_result == TCL_ERROR) {
        Tcl_Obj*    errorCodeListObj;
        Tcl_Obj*    errorCodeElementObj;

        /* There was an error, see if it's from Rivet and it was caused
         * by abort_page.
         */

        errorCodeListObj = Tcl_GetVar2Ex (interp_obj->interp, "errorCode", (char *)NULL, TCL_GLOBAL_ONLY);

        /* errorCode is guaranteed to be set to NONE, but let's make sure
         * anyway rather than causing a SIGSEGV
         */
        ap_assert (errorCodeListObj != (Tcl_Obj *)NULL);

        /* dig the first element out of the errorCode list and see if it
         * says Rivet -- this shouldn't fail either, but let's assert
         * success so we don't get a SIGSEGV afterwards */
        ap_assert (Tcl_ListObjIndex (interp_obj->interp, errorCodeListObj, 0, &errorCodeElementObj) == TCL_OK);

        /* if the error was thrown by Rivet, see if it's abort_page and,
         * if so, don't treat it as an error, i.e. don't execute the
         * installed error handler or the default one, just check if
         * a rivet_abort_script is defined, otherwise the page emits 
         * as normal
         */

        // if (strcmp (Tcl_GetString (errorCodeElementObj), "RIVET") == 0) 
        if (private->page_aborting)
        {
            //char*       errorCodeSubString;

            /* dig the second element out of the errorCode list, make sure
             * it succeeds -- it should always
             */
            //ap_assert (Tcl_ListObjIndex (interp_obj->interp, errorCodeListObj, 1, &errorCodeElementObj) == TCL_OK);

            //errorCodeSubString = Tcl_GetString (errorCodeElementObj);
            //if ((strcmp(errorCodeSubString, ABORTPAGE_CODE) == 0) || 
            //    (strcmp(errorCodeSubString, THREAD_EXIT_CODE) == 0))
            //
            //{
                Rivet_RunAbortScript(private);
            //}
 
        }
        else 
        {
            Rivet_ExecuteErrorHandler (interp_obj->interp,tcl_script_obj,private);
        }
    }
    
    Tcl_Release(interp_obj->interp);

    return tcl_result;
}

/*
 * -- Rivet_ParseExecFile
 *
 * given a filename if the file exists it's either parsed (when a rivet
 * template) and then executed as a Tcl_Obj instance or directly executed
 * if a Tcl script.
 *
 * This is a separate function so that it may be called from command 'parse'
 *
 * Arguments:
 *
 *   - rivet_thread_private:  pointer to the structure collecting thread private data
 *                            for Tcl and current request
 *   - filename:              pointer to a string storing the path to the template or
 *                            Tcl script
 *   - toplevel:              boolean value set when the argument 'filename' is the request 
 *                            toplevel script. The value is 0 when the function is called
 *                            by command ::rivet::parse
 *
 * Returned value:
 *
 *  this function must return a Tcl valid status code (TCL_OK, TCL_ERROR ....)
 *
 */

int
Rivet_ParseExecFile(rivet_thread_private* private, char *filename, int toplevel)
{
    char*           hashKey = NULL;
    int             isNew   = 0;
    int             result  = 0;
    rivet_thread_interp*   rivet_interp;
    Tcl_Obj*        outbuf  = NULL;
    Tcl_HashEntry*  entry   = NULL;
    Tcl_Interp*     interp;
    time_t          ctime;
    time_t          mtime;
    int             res = 0;

    /* We have to fetch the interpreter data from the thread private environment */

    rivet_interp = private->interps[private->running_conf->idx];
    interp = rivet_interp->interp;

    /* If the user configuration has indeed been updated, I guess that
       pretty much invalidates anything that might have been cached. */

    /* This is all horrendously slow, and means we should *also* be
       doing caching on the modification time of the .htaccess files
       that concern us. FIXME */

    if (USER_CONF_UPDATED(private->running_conf) && (rivet_interp->cache_size != 0) && 
                                                    (rivet_interp->cache_free < rivet_interp->cache_size)) 
    {
        int ct;
        Tcl_HashEntry *delEntry;

        /* Clean out the list. */
        ct = rivet_interp->cache_free;
        while (ct < rivet_interp->cache_size) {
            /* Free the corresponding hash entry. */
            delEntry = Tcl_FindHashEntry(
                    rivet_interp->objCache,
                    rivet_interp->objCacheList[ct]);

            if (delEntry != NULL) {
                Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
                Tcl_DeleteHashEntry(delEntry);
                rivet_interp->objCacheList[ct] = NULL;
            }

            ct++;
        }
        apr_pool_destroy(rivet_interp->pool);
        
        /* let's recreate the cache list */

        if (apr_pool_create(&rivet_interp->pool, private->pool) != APR_SUCCESS)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                         MODNAME ": could not recreate cache private pool. Cache disabled");
            rivet_interp->cache_free = rivet_interp->cache_size = 0;
        }
        else
        {
            rivet_interp->objCacheList = apr_pcalloc (rivet_interp->pool, 
                                                    (signed)(rivet_interp->cache_size*sizeof(char *)));
            rivet_interp->cache_free = rivet_interp->cache_size;
        }
    }

    /* If toplevel is 0, we are being called from Parse, which means
       we need to get the information about the file ourselves. */

    if (toplevel == 0)
    {
        Tcl_Obj *fnobj;
        Tcl_StatBuf buf;

        fnobj = Tcl_NewStringObj(filename, -1);
        Tcl_IncrRefCount(fnobj);
        if (Tcl_FSStat(fnobj, &buf) < 0)
            return TCL_ERROR;
        Tcl_DecrRefCount(fnobj);
        ctime = buf.st_ctime;
        mtime = buf.st_mtime;
    } else {
        ctime = private->r->finfo.ctime;
        mtime = private->r->finfo.mtime;
    }

    /* Look for the script's compiled version.  If it's not found,
     * create it.
     */

    if (rivet_interp->cache_size)
    {
        unsigned int user_conf = IS_USER_CONF(private->running_conf);

        hashKey = (char*) apr_psprintf(private->r->pool, "%s%lx%lx%d-%d", filename,
                mtime, ctime, toplevel,user_conf);
        entry = Tcl_CreateHashEntry(rivet_interp->objCache, hashKey, &isNew);
    }

    /* We don't have a compiled version.  Let's create one. */
    if (isNew || (rivet_interp->cache_size == 0))
    {
        outbuf = Tcl_NewObj();
        Tcl_IncrRefCount(outbuf);

        if (toplevel && private->running->rivet_before_script) 
        {
            Tcl_AppendObjToObj(outbuf,private->running->rivet_before_script);
        }

/*
 * We check whether we are dealing with a pure Tcl script or a Rivet template.
 * Actually this check is done only if we are processing a toplevel file, every nested 
 * file (files included through the 'parse' command) is treated as a template.
 */

        if (!toplevel || (Rivet_CheckType(private->r) == RIVET_TEMPLATE))
        {
            /* toplevel == 0 means we are being called from the parse
             * command, which only works on Rivet .rvt files. */

            result = Rivet_GetRivetFile(filename, toplevel, outbuf, interp);

        } else {
            /* It's a plain Tcl file */
            result = Rivet_GetTclFile(filename, outbuf, interp);
        }

        if (result != TCL_OK)
        {
            Tcl_DecrRefCount(outbuf);
            return result;
        }

        if (toplevel && private->running->rivet_after_script) {
            Tcl_AppendObjToObj(outbuf,private->running->rivet_after_script);
        }

        if (rivet_interp->cache_size) {

            /* We need to incr the reference count of outbuf because we want
             * it to outlive this function.  This allows it to stay alive
             * as long as it's in the object cache.
             */

            Tcl_IncrRefCount (outbuf);
            Tcl_SetHashValue (entry,(ClientData)outbuf);
        }

        if (rivet_interp->cache_free) {

            rivet_interp->objCacheList[--rivet_interp->cache_free] = 
                (char*) apr_pcalloc (rivet_interp->pool,(strlen(hashKey)+1)*sizeof(char));
            strcpy(rivet_interp->objCacheList[rivet_interp->cache_free], hashKey);

        } else if (rivet_interp->cache_size) { /* If it's zero, we just skip this. */

/*
        instead of removing the last entry in the cache (for what purpose after all??)
        we signal a 'cache full' condition
 */
            
            if ((rivet_interp->flags & RIVET_CACHE_FULL) == 0)
            {
                ap_log_error(APLOG_MARK, APLOG_NOTICE, APR_EGENERAL, private->r->server, 
                             MODNAME ": Cache full");
                rivet_interp->flags |= RIVET_CACHE_FULL;
            }
        }

    } else {

        /* We found a compiled version of this page. */
        outbuf = (Tcl_Obj *)Tcl_GetHashValue(entry);
        Tcl_IncrRefCount(outbuf);

    }

    res = Rivet_ExecuteAndCheck (private, outbuf);
    Tcl_DecrRefCount(outbuf);

    /* We don't keep user script until we find a way to cache them consistently */

    if (IS_USER_CONF(private->running_conf))
    {
        Rivet_ReleaseScripts(private->running);
        private->running_conf->user_scripts_status &= ~(unsigned int)USER_SCRIPTS_UPDATED;
    }

    return res;
}

/*
 * -- Rivet_ParseExecString
 *
 * This function accepts a Tcl_Obj carrying a string to be interpreted as
 * a Rivet template. This function is the core for command 'parsestr'
 * 
 * Arguments:
 *
 *   - TclWebRequest* req: pointer to the structure collecting Tcl and
 *   Apache data
 *   - Tcl_Obj* inbuf: Tcl object storing the template to be parsed.
 */

int
Rivet_ParseExecString (TclWebRequest* req, Tcl_Obj* inbuf)
{
    int res = 0;
    Tcl_Obj* outbuf = Tcl_NewObj();
    rivet_thread_private* private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);
    Tcl_IncrRefCount(outbuf);
    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    /* If we are not inside a <? ?> section, add the closing ". */
    if (Rivet_Parser(outbuf, inbuf) == 0)
    {
        Tcl_AppendToObj(outbuf, "\"\n", 2);
    } 

    Tcl_AppendToObj(outbuf, "\n", -1);

    res = Rivet_ExecuteAndCheck(private, outbuf);
    Tcl_DecrRefCount(outbuf);

    return res;
}

/*
 *-----------------------------------------------------------------------------
 * Rivet_CreateTclInterp --
 *
 * Arguments:
 *  server_rec* s: pointer to a server_rec structure
 *
 * Results:
 *  pointer to a Tcl_Interp structure
 * 
 * Side Effects:
 *
 *-----------------------------------------------------------------------------
 */

Tcl_Interp* 
Rivet_CreateTclInterp (server_rec* s)
{
    Tcl_Interp* interp;

    /* Initialize TCL stuff  */
    Tcl_FindExecutable(RIVET_NAMEOFEXECUTABLE);
    interp = Tcl_CreateInterp();

    if (interp == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error in Tcl_CreateInterp, aborting\n");
        exit(1);
    }

    if (Tcl_Init(interp) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error in Tcl_Init: %s, aborting\n",
                     Tcl_GetStringResult(interp));
        exit(1);
    }

    return interp;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PerInterpInit --
 *
 *  Do the initialization that needs to happen for every
 *  interpreter.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */
void Rivet_PerInterpInit(rivet_thread_interp* interp_obj,rivet_thread_private* private, server_rec *s, apr_pool_t *p)
{
    rivet_interp_globals*   globals     = NULL;
    Tcl_Obj*                auto_path   = NULL;
    Tcl_Obj*                rivet_tcl   = NULL;
    Tcl_Interp*             interp      = interp_obj->interp;

    ap_assert (interp != (Tcl_Interp *)NULL);
    Tcl_Preserve (interp);

    /* Set up interpreter associated data */

    globals = apr_pcalloc (p, sizeof(rivet_interp_globals));
    Tcl_SetAssocData (interp,"rivet",NULL,globals);
    
    /* 
     * abort_page status variables in globals are set here and then 
     * reset in Rivet_SendContent just before the request processing is 
     * completed 
     */

    /* Rivet commands namespace is created */

    globals->rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,
                                            (Tcl_NamespaceDeleteProc *)NULL);
    //globals->req    = TclWeb_NewRequestObject (p); 
    //globals->r      = NULL;
    //globals->srec   = s;

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    /* We put in front the auto_path list the path to the directory where
     * init.tcl is located (provides package Rivet, previously RivetTcl)
     */

    auto_path = Tcl_GetVar2Ex(interp,"auto_path",NULL,TCL_GLOBAL_ONLY);

    rivet_tcl = Tcl_NewStringObj(RIVET_DIR,-1);
    Tcl_IncrRefCount(rivet_tcl);

    if (Tcl_IsShared(auto_path)) {
        auto_path = Tcl_DuplicateObj(auto_path);
    }

    if (Tcl_ListObjReplace(interp,auto_path,0,0,1,&rivet_tcl) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": error setting auto_path: %s",
                     Tcl_GetStringFromObj(auto_path,NULL));
    } else {
        Tcl_SetVar2Ex(interp,"auto_path",NULL,auto_path,TCL_GLOBAL_ONLY);
    }

    Tcl_DecrRefCount(rivet_tcl);

    /* Initialize the interpreter with Rivet's Tcl commands. */

    if (private == NULL)
    {
        /* reduced core for the server init interpreter */

        // RIVET_OBJ_CMD ("inspect",Rivet_InspectCmd,private);
        
    } 
    else
    {
        /* full fledged rivet core */

        Rivet_InitCore(interp,private);

    }

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables(interp, p );

    /* Loading into the interpreter commands in librivet.so */
    /* Tcl Bug #3216070 has been solved with 8.5.10 and commands shipped with
     * Rivetlib can be mapped at this stage
     */

    if (Tcl_PkgRequire(interp, RIVETLIB_TCL_PACKAGE, RIVET_VERSION, 1) == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error loading rivetlib package: %s",
                     Tcl_GetStringResult(interp) );
        exit(1);
    }

    /*  If rivet is configured to export the ::rivet namespace commands we set the
     *  array variable ::rivet::module_conf(export_namespace_commands) before calling init.tcl
     *  This array will be unset after commands are exported.
     */

    Tcl_SetVar2Ex(interp,"module_conf","export_namespace_commands",Tcl_NewIntObj(RIVET_NAMESPACE_EXPORT),0);
    Tcl_SetVar2Ex(interp,"module_conf","import_rivet_commands",Tcl_NewIntObj(RIVET_NAMESPACE_IMPORT),0);

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    /* Watch out! Calling Tcl_PkgRequire with a version number binds this module to
     * the Rivet package revision number in rivet/init.tcl
     *
     * RIVET_TCL_PACKAGE_VERSION is defined by configure.ac as the combination
     * "MAJOR_VERSION.MINOR_VERSION". We don't expect to change rivet/init.tcl
     * across patchlevel releases
     */

    if (Tcl_PkgRequire(interp, "Rivet", RIVET_INIT_VERSION, 1) == NULL)
    {
        ap_log_error (APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                      MODNAME ": init.tcl must be installed correctly for Apache Rivet to function: %s (%s)",
                      Tcl_GetStringResult(interp), RIVET_DIR );
        exit(1);
    }

    Tcl_Release(interp);
    interp_obj->flags |= RIVET_INTERP_INITIALIZED;
}

/* 
 * -- Rivet_RunServerInit
 *
 */

static int
Rivet_RunServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf* rsc = RIVET_SERVER_CONF( s->module_config );

    FILEDEBUGINFO;

    /* we create and initialize a master (server) interpreter */

    module_globals->server_interp = Rivet_NewVHostInterp(pPool); /* root interpreter */

    /* We initialize the interpreter and we won't register a channel with it because
     * we couldn't send data to the stdout anyway */

    Rivet_PerInterpInit(module_globals->server_interp,NULL,s,pPool);

    /* We don't create the cache here: it would make sense for prefork MPM
     * but threaded MPM bridges have their pool of threads. Each of them
     * will by now have their own cache
     */

    if (rsc->rivet_server_init_script != NULL) {
        Tcl_Interp* interp = module_globals->server_interp->interp;
        Tcl_Obj*    server_init = Tcl_NewStringObj(rsc->rivet_server_init_script,-1);

        Tcl_IncrRefCount(server_init);

        if (Tcl_EvalObjEx(interp, server_init, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running ServerInitScript '%s': %s",
                         rsc->rivet_server_init_script,
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": ServerInitScript '%s' successful", 
                         rsc->rivet_server_init_script);
        }

        Tcl_DecrRefCount(server_init);
    }

    return OK;

}

/* -- Rivet_ServerInit
 *
 */

static int
Rivet_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *server)
{
    char*               mpm_prefork_bridge = "rivet_prefork_mpm.so";
    char*               mpm_worker_bridge  = "rivet_worker_mpm.so";
    char*               mpm_model_path;
    int                 ap_mpm_result;
    apr_dso_handle_t*   dso_handle;

#if RIVET_DISPLAY_VERSION
    ap_add_version_component(pPool,RIVET_PACKAGE_NAME"/"RIVET_VERSION);
#else
    ap_add_version_component(pPool,RIVET_PACKAGE_NAME);
#endif

    /* Everything revolves around this structure */

    module_globals = apr_palloc(pPool,sizeof(mod_rivet_globals));

    /* Creating the module global pool */

    if (apr_pool_create(&module_globals->pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }

    /* Let's load the Rivet-MPM bridge */

    module_globals->rivet_mpm_bridge = mpm_worker_bridge;
    if (ap_mpm_query(AP_MPMQ_IS_THREADED,&ap_mpm_result) == APR_SUCCESS)
    {

        if (ap_mpm_result == AP_MPMQ_NOT_SUPPORTED)
        {
            /* we are forced to load the prefork MPM bridge */

            module_globals->rivet_mpm_bridge = apr_pstrdup(pPool,mpm_prefork_bridge);
        }
        else
        {
            module_globals->rivet_mpm_bridge = apr_pstrdup(pPool,mpm_worker_bridge);
        }
    }
    else
    {

        /* Execution shouldn't get here as a failure querying about MPM is supposed
         * to return APR_SUCCESS in every normal operative conditions. We
         * give a default to the MPM bridge anyway
         */

        module_globals->rivet_mpm_bridge = apr_pstrdup(pPool,mpm_worker_bridge);
    }

    /* With the env variable RIVET_MPM_BRIDGE we have the chance to tell mod_rivet 
       what bridge custom implementation we want to be loaded */

    if (apr_env_get (&mpm_model_path,"RIVET_MPM_BRIDGE",pTemp) != APR_SUCCESS)
    {
        mpm_model_path = apr_pstrcat(pTemp,RIVET_DIR,"/mpm/",module_globals->rivet_mpm_bridge,NULL);
    } 

    /* Finally the bridge is loaded and the jump table sought */

    if (apr_dso_load(&dso_handle,mpm_model_path,pPool) == APR_SUCCESS)
    {
        apr_status_t                rv;
        apr_dso_handle_sym_t        func = NULL;

        ap_log_error(APLOG_MARK,APLOG_DEBUG,0,server,"MPM bridge loaded: %s",mpm_model_path);

        rv = apr_dso_sym(&func,dso_handle,"bridge_jump_table");
        if (rv == APR_SUCCESS)
        {
            module_globals->bridge_jump_table = (rivet_bridge_table*) func;
        }
        else
        {
            char errorbuf[ERRORBUF_SZ];

            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                         MODNAME ": Error loading symbol bridge_jump_table: %s", 
                         apr_dso_error(dso_handle,errorbuf,ERRORBUF_SZ));
            exit(1);   
        }

        /* we require only mpm_request and mpm_master_interp to be defined */

        ap_assert(RIVET_MPM_BRIDGE_FUNCTION(mpm_request) != NULL);
        ap_assert(RIVET_MPM_BRIDGE_FUNCTION(mpm_master_interp) != NULL);

        apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pPool);

    /* Another crucial point: we are storing in the globals a reference to the
     * root server_rec object from which threads are supposed to derive 
     * all the other chain of virtual hosts server records
     */

        module_globals->server = server;

        //ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pPool) == APR_SUCCESS);
        //private = Rivet_CreatePrivateData();
        //ap_assert (private != NULL);
        //Rivet_SetupTclPanicProc ();

    }
    else
    {
        char errorbuf[ERRORBUF_SZ];

        /* If we don't find the mpm handler module we give up and exit */

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME " Error loading MPM manager: %s", 
                     apr_dso_error(dso_handle,errorbuf,ERRORBUF_SZ));
        exit(1);   
    }

    Rivet_RunServerInit(pPool,pLog,pTemp,server);

    RIVET_MPM_BRIDGE_CALL(mpm_server_init,pPool,pLog,pTemp,server);

    return OK;
}

/*
 * -- Rivet_Finalize
 *
 */

apr_status_t Rivet_Finalize(void* data)
{

    RIVET_MPM_BRIDGE_CALL(mpm_finalize,data);
    apr_threadkey_private_delete (rivet_thread_key);

    return OK;
}

static void Rivet_ChildInit (apr_pool_t *pChild, server_rec *server)
{
    int                 idx;
    rivet_server_conf*  root_server_conf;
    server_rec*         s;

    /* the thread key used to access to Tcl threads private data */

    ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pChild) == APR_SUCCESS);

/* This code is run once per child process. In a threaded Tcl builds the forking 
 * of a child process most likely has not preserved the thread where the Tcl 
 * notifier runs. The Notifier should have been restarted by one the 
 * pthread_atfork callbacks (setup in Tcl >= 8.5.14 and Tcl >= 8.6.1). In
 * case pthread_atfork is not supported we unconditionally call Tcl_InitNotifier
 * hoping for the best (Bug #55153)      
 */

#if !defined(HAVE_PTHREAD_ATFORK)
    Tcl_InitNotifier();
#endif

    apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pChild);

    /* Once we have established a pool with the same lifetime of the child process we
     * process all the configured server records assigning an integer as unique key 
     * to each of them 
     */

    root_server_conf = RIVET_SERVER_CONF( server->module_config );
    idx = 0;
    for (s = server; s != NULL; s = s->next)
    {
        rivet_server_conf*  myrsc;

        myrsc = RIVET_SERVER_CONF( s->module_config );

        /* We only have a different rivet_server_conf if MergeConfig
         * was called. We really need a separate one for each server,
         * so we go ahead and create one here, if necessary. */

        if (s != server && myrsc == root_server_conf) {
            myrsc = RIVET_NEW_CONF(pChild);
            ap_set_module_config(s->module_config, &rivet_module, myrsc);
            Rivet_CopyConfig( root_server_conf, myrsc );
        }

        myrsc->idx = idx++;
    }
    module_globals->vhosts_count = idx;

    /* Calling the brigde child process initialization */

    RIVET_MPM_BRIDGE_CALL(mpm_child_init,pChild,server);

    apr_pool_cleanup_register (pChild,server,Rivet_Finalize,Rivet_Finalize);
}


static int Rivet_Handler (request_rec *r)    
{
    rivet_req_ctype ctype = Rivet_CheckType(r);  
    if (ctype == CTYPE_NOT_HANDLED) {
        return DECLINED;
    }

    return (*RIVET_MPM_BRIDGE_FUNCTION(mpm_request))(r,ctype);
}

/*
 * -- rivet_register_hooks: mod_rivet basic setup.
 *
 * 
 */

static void rivet_register_hooks(apr_pool_t *p)
{
    ap_hook_post_config (Rivet_ServerInit, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler     (Rivet_Handler,    NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init  (Rivet_ChildInit,  NULL, NULL, APR_HOOK_LAST);
}

/* mod_rivet basic structures */

/* configuration commands and directives */

const command_rec rivet_cmds[] =
{
    AP_INIT_TAKE2 ("RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, NULL),
    AP_INIT_TAKE2 ("RivetDirConf", Rivet_DirConf, NULL, ACCESS_CONF, NULL),
    AP_INIT_TAKE2 ("RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO,
                   "RivetUserConf key value: sets RivetUserConf(key) = value"),
    {NULL}
};

/* Dispatch list for API hooks */

module AP_MODULE_DECLARE_DATA rivet_module = 
{
    STANDARD20_MODULE_STUFF, 
    Rivet_CreateDirConfig,  /* create per-dir config structures    */
    Rivet_MergeDirConfig,   /* merge  per-dir config structures    */
    Rivet_CreateConfig,     /* create per-server config structures */
    Rivet_MergeConfig,      /* merge  per-server config structures */
    rivet_cmds,             /* table of config file commands       */
    rivet_register_hooks    /* register hooks                      */
};

