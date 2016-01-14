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
#include "mod_rivet_common.h"
#include "rivetParser.h"
#include "rivetCore.h"
#include "apache_config.h"
#include "TclWeb.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;

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
    Tcl_Interp* interp = RIVET_PEEK_INTERP(private,private->running_conf)->interp;

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

    interp_obj = RIVET_PEEK_INTERP(private,private->running_conf);
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

    rivet_interp = RIVET_PEEK_INTERP(private,private->running_conf);
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
Rivet_ParseExecString (rivet_thread_private* private, Tcl_Obj* inbuf)
{
    int res = 0;
    Tcl_Obj* outbuf = Tcl_NewObj();

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

    /* URL referenced script execution and exception handling */

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
        if (Rivet_ExecuteAndCheck(private,private->running->after_every_script) == TCL_ERROR)
        {
            Rivet_PrintErrorMessage(RIVET_PEEK_INTERP(private,private->running_conf)->interp,
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
