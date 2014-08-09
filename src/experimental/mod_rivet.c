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

/* $Id: mod_rivet.h 1609472 2014-07-10 15:08:52Z mxmanghi $ */

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

#include "TclWeb.h"
#include "rivet.h"
#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "rivetParser.h"
#include "rivetChannel.h"
#include "apache_config.h"

mod_rivet_globals*      module_globals;
rivet_server_conf       rsc;
rivet_interp_globals    interp_globals;

extern Tcl_ChannelType RivetChan;
apr_threadkey_t*        rivet_thread_key;
apr_threadkey_t*        handler_thread_key;

#define ERRORBUF_SZ     256

/* -- Rivet_ExecuteAndCheck
 * 
 * Tcl script execution central procedure. The script stored
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
 *      - invariably TCL_OK
 *
 *   Side effects:
 *
 *      The Tcl interpreter internal status is changed by the execution
 *      of the script
 *
 */

static int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *tcl_script_obj, request_rec *req)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    rivet_server_conf *conf = Rivet_GetConf(req);
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    Tcl_Preserve (interp);
    if ( Tcl_EvalObjEx(interp, tcl_script_obj, 0) == TCL_ERROR ) {
        Tcl_Obj *errscript;
        Tcl_Obj *errorCodeListObj;
        Tcl_Obj *errorCodeElementObj;
        char *errorCodeSubString;

        /* There was an error, see if it's from Rivet and it was caused
         * by abort_page.
         */

        errorCodeListObj = Tcl_GetVar2Ex (interp, "errorCode", (char *)NULL, TCL_GLOBAL_ONLY);

        /* errorCode is guaranteed to be set to NONE, but let's make sure
         * anyway rather than causing a SIGSEGV
         */
        ap_assert (errorCodeListObj != (Tcl_Obj *)NULL);

        /* dig the first element out of the errorCode list and see if it
         * says Rivet -- this shouldn't fail either, but let's assert
         * success so we don't get a SIGSEGV afterwards */
        ap_assert (Tcl_ListObjIndex (interp, errorCodeListObj, 0, &errorCodeElementObj) == TCL_OK);

        /* if the error was thrown by Rivet, see if it's abort_page and,
         * if so, don't treat it as an error, i.e. don't execute the
         * installed error handler or the default one, just check if
         * a rivet_abort_script is defined, otherwise the page emits 
         * as normal
         */
        if (strcmp (Tcl_GetString (errorCodeElementObj), "RIVET") == 0) {

            /* dig the second element out of the errorCode list, make sure
             * it succeeds -- it should always
             */
            ap_assert (Tcl_ListObjIndex (interp, errorCodeListObj, 1, &errorCodeElementObj) == TCL_OK);

            errorCodeSubString = Tcl_GetString (errorCodeElementObj);
            if (strcmp (errorCodeSubString, ABORTPAGE_CODE) == 0) 
            {
                if (conf->rivet_abort_script) 
                {
                    if (Tcl_EvalObjEx(interp,conf->rivet_abort_script,0) == TCL_ERROR)
                    {
                        CONST84 char *errorinfo = Tcl_GetVar( interp, "errorInfo", 0 );
                        TclWeb_PrintError("<b>Rivet AbortScript failed!</b>",1,globals->req);
                        TclWeb_PrintError( errorinfo, 0, globals->req );
                    }
                }
                goto good;
            }
        }

        Tcl_SetVar( interp, "errorOutbuf",Tcl_GetStringFromObj( tcl_script_obj, NULL ),TCL_GLOBAL_ONLY );

        /* If we don't have an error script, use the default error handler. */
        if (conf->rivet_error_script ) {
            errscript = conf->rivet_error_script;
        } else {
            errscript = conf->rivet_default_error_script;
        }

        Tcl_IncrRefCount(errscript);
        if (Tcl_EvalObjEx(interp, errscript, 0) == TCL_ERROR) {
            CONST84 char *errorinfo = Tcl_GetVar( interp, "errorInfo", 0 );
            TclWeb_PrintError("<b>Rivet ErrorScript failed!</b>",1,globals->req);
            TclWeb_PrintError( errorinfo, 0, globals->req );
        }

        /* This shouldn't make the default_error_script go away,
         * because it gets a Tcl_IncrRefCount when it is created. */
        Tcl_DecrRefCount(errscript);
    }

    /* Make sure to flush the output if buffer_add was the only output */
good:
    
    if (conf->after_every_script) {
        if (Tcl_EvalObjEx(interp,conf->after_every_script,0) == TCL_ERROR)
        {
            CONST84 char *errorinfo = Tcl_GetVar( interp, "errorInfo", 0 );
            TclWeb_PrintError("<b>Rivet AfterEveryScript failed!</b>",1,globals->req);
            TclWeb_PrintError( errorinfo, 0, globals->req );
        }
    }

    if (!globals->req->headers_set && (globals->req->charset != NULL)) {
        TclWeb_SetHeaderType (apr_pstrcat(globals->req->req->pool,"text/html;",globals->req->charset,NULL),globals->req);
    }
    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(*(private->channel));
    Tcl_Release(interp);

    return TCL_OK;
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
 *   - TclWebRequest: pointer to the structure collecting Tcl and Apache data
 *   - filename:      pointer to a string storing the path to the template or
 *                    Tcl script
 *   - toplevel:      integer to be interpreted as a boolean meaning the
 *                    file is pointed by the request. When 0 that's a subtemplate
 *                    to be parsed and executed from another template
 */

int
Rivet_ParseExecFile(TclWebRequest *req, char *filename, int toplevel)
{
    char *hashKey = NULL;
    int isNew = 0;
    int result = 0;

    Tcl_Obj *outbuf = NULL;
    Tcl_HashEntry *entry = NULL;

    time_t ctime;
    time_t mtime;

    rivet_server_conf *rsc;
    Tcl_Interp *interp = req->interp;

    rsc = Rivet_GetConf( req->req );

    /* If the user configuration has indeed been updated, I guess that
       pretty much invalidates anything that might have been
       cached. */

    /* This is all horrendously slow, and means we should *also* be
       doing caching on the modification time of the .htaccess files
       that concern us. FIXME */

    if (rsc->user_scripts_updated && *(rsc->cache_size) != 0) {
        int ct;
        Tcl_HashEntry *delEntry;
        /* Clean out the list. */
        ct = *(rsc->cache_free);
        while (ct < *(rsc->cache_size)) {
            /* Free the corresponding hash entry. */
            delEntry = Tcl_FindHashEntry(
                    rsc->objCache,
                    rsc->objCacheList[ct]);
            if (delEntry != NULL) {
                Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
            }
            Tcl_DeleteHashEntry(delEntry);

            free(rsc->objCacheList[ct]);
            rsc->objCacheList[ct] = NULL;
            ct ++;
        }
        *(rsc->cache_free) = *(rsc->cache_size);
    }

    /* If toplevel is 0, we are being called from Parse, which means
       we need to get the information about the file ourselves. */
    if (toplevel == 0)
    {
        Tcl_Obj *fnobj;
        Tcl_StatBuf buf;

        fnobj = Tcl_NewStringObj(filename, -1);
        Tcl_IncrRefCount(fnobj);
        if( Tcl_FSStat(fnobj, &buf) < 0 )
            return TCL_ERROR;
        Tcl_DecrRefCount(fnobj);
        ctime = buf.st_ctime;
        mtime = buf.st_mtime;
    } else {
        ctime = req->req->finfo.ctime;
        mtime = req->req->finfo.mtime;
    }

    /* Look for the script's compiled version.  If it's not found,
     * create it.
     */
    if (*(rsc->cache_size))
    {
        hashKey = (char*) apr_psprintf(req->req->pool, "%s%lx%lx%d", filename,
                mtime, ctime, toplevel);
        entry = Tcl_CreateHashEntry(rsc->objCache, hashKey, &isNew);
    }

    /* We don't have a compiled version.  Let's create one. */
    if (isNew || *(rsc->cache_size) == 0)
    {
        outbuf = Tcl_NewObj();
        Tcl_IncrRefCount(outbuf);

        if (toplevel) {
            if (rsc->rivet_before_script) {
                Tcl_AppendObjToObj(outbuf,rsc->rivet_before_script);
            }
        }

/*
 * We check whether we are dealing with a pure Tcl script or a Rivet template.
 * Actually this check is done only if we are processing a toplevel file, every nested 
 * file (files included through the 'parse' command) is treated as a template.
 */

        if (!toplevel || (Rivet_CheckType(req->req) == RIVET_TEMPLATE))
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

        if (toplevel && rsc->rivet_after_script) {
            Tcl_AppendObjToObj(outbuf,rsc->rivet_after_script);
        }

        if (*(rsc->cache_size)) {
            /* We need to incr the reference count of outbuf because we want
             * it to outlive this function.  This allows it to stay alive
             * as long as it's in the object cache.
             */
            Tcl_IncrRefCount( outbuf );
            Tcl_SetHashValue(entry, (ClientData)outbuf);
        }

        if (*(rsc->cache_free)) {

            //hkCopy = (char*) malloc ((strlen(hashKey)+1) * sizeof(char));
            //strcpy(rsc->objCacheList[-- *(rsc->cache_free)], hashKey);
            rsc->objCacheList[--*(rsc->cache_free)] = 
                (char*) malloc ((strlen(hashKey)+1) * sizeof(char));
            strcpy(rsc->objCacheList[*(rsc->cache_free)], hashKey);
            //rsc->objCacheList[-- *(rsc->cache_free) ] = strdup(hashKey);
        } else if (*(rsc->cache_size)) { /* If it's zero, we just skip this. */
            Tcl_HashEntry *delEntry;
            delEntry = Tcl_FindHashEntry(
                    rsc->objCache,
                    rsc->objCacheList[*(rsc->cache_size) - 1]);
            Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
            Tcl_DeleteHashEntry(delEntry);
            free(rsc->objCacheList[*(rsc->cache_size) - 1]);
            memmove((rsc->objCacheList) + 1, rsc->objCacheList,
                    sizeof(char *) * (*(rsc->cache_size) - 1));

            //hkCopy = (char*) malloc ((strlen(hashKey)+1) * sizeof(char));
            //strcpy (rsc->objCacheList[0], hashKey);
            rsc->objCacheList[0] = (char*) malloc ((strlen(hashKey)+1) * sizeof(char));
            strcpy (rsc->objCacheList[0], hashKey);
            
            //rsc->objCacheList[0] = (char*) strdup(hashKey);
        }
    } else {
        /* We found a compiled version of this page. */
        outbuf = (Tcl_Obj *)Tcl_GetHashValue(entry);
        Tcl_IncrRefCount(outbuf);
    }

    rsc->user_scripts_updated = 0;
    {
        int res = 0;
        res = Rivet_ExecuteAndCheck(interp, outbuf, req->req);
        Tcl_DecrRefCount(outbuf);
        return res;
    }
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
    Tcl_Interp *interp = req->interp;

    Tcl_IncrRefCount(outbuf);
    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    /* If we are not inside a <? ?> section, add the closing ". */
    if (Rivet_Parser(outbuf, inbuf) == 0)
    {
        Tcl_AppendToObj(outbuf, "\"\n", 2);
    } 

    Tcl_AppendToObj(outbuf, "\n", -1);

    res = Rivet_ExecuteAndCheck(interp, outbuf, req->req);
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

static Tcl_Interp* 
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
void  Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p)
{
    rivet_interp_globals *globals = NULL;
    Tcl_Obj* auto_path = NULL;
    Tcl_Obj* rivet_tcl = NULL;

    ap_assert (interp != (Tcl_Interp *)NULL);
    Tcl_Preserve (interp);

    /* We register the Tcl channel to the interpreter */

    Tcl_RegisterChannel(interp, *(module_globals->outchannel));

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
    globals->page_aborting  = 0;
    globals->abort_code     = NULL;
    globals->req            = TclWeb_NewRequestObject (p); 
    globals->srec           = s;
    globals->r              = NULL;

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    /* We put in front the auto_path list the path to the directory where
     * init.tcl is located (provides package Rivet, previously RivetTcl)
     */

    auto_path = Tcl_GetVar2Ex(interp,"auto_path",NULL,TCL_GLOBAL_ONLY);

    rivet_tcl = Tcl_NewStringObj(RIVET_DIR,-1);
    Tcl_IncrRefCount(rivet_tcl);

    if (Tcl_ListObjReplace(interp,auto_path,0,0,1,&rivet_tcl) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": error setting auto_path: %s",
                     Tcl_GetStringFromObj(auto_path,NULL));
    } 
    Tcl_DecrRefCount(rivet_tcl);

    /* Initialize the interpreter with Rivet's Tcl commands. */
    Rivet_InitCore(interp);

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables(interp, p );
//  Rivet_PropagateServerConfArray( interp, rsc );

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

    if (Tcl_PkgRequire(interp, "Rivet", RIVET_TCL_PACKAGE_VERSION, 1) == NULL)
    {
        ap_log_error (APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                      MODNAME ": init.tcl must be installed correctly for Apache Rivet to function: %s (%s)",
                      Tcl_GetStringResult(interp), RIVET_DIR );
        exit(1);
    }

    Tcl_Release(interp);
}

/*
 *
 */

static int Rivet_InitHandler(apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );

    apr_threadkey_private_create (&rivet_thread_key,NULL, pPool);

    module_globals->rivet_panic_pool       = pPool;
    module_globals->rivet_panic_server_rec = s;
    module_globals->server                 = s;

#if RIVET_DISPLAY_VERSION
    ap_add_version_component(pPool, "Rivet/Experimental/"__DATE__"/"__TIME__"/");
    //ap_add_version_component(pPool, RIVET_PACKAGE_NAME"/"RIVET_VERSION);
#else
    ap_add_version_component(pPool, RIVET_PACKAGE_NAME);
#endif

    FILEDEBUGINFO;

    /* we create and initialize a master (server) interpreter */

    rsc->server_interp = Rivet_CreateTclInterp(s) ; /* root interpreter */

    /* Create TCL channel and store a pointer in the rivet_server_conf object */

    module_globals->outchannel    = apr_pcalloc (pPool, sizeof(Tcl_Channel));
    *(module_globals->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

    /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*(module_globals->outchannel), TCL_STDOUT);

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*(module_globals->outchannel), TCL_MAX_CHANNEL_BUFFER_SIZE);

    /* We register the Tcl channel to the interpreter */

    Tcl_RegisterChannel(rsc->server_interp, *(module_globals->outchannel));

    Rivet_PerInterpInit(rsc->server_interp,s,pPool);

    /* we don't create the cache here: it would make sense for prefork MPM
     * but threaded MPM bridges have their pool of threads. Each of them
     * will by now have their own cache
     */

    // Rivet_CreateCache(s,pPool);

    if (rsc->rivet_server_init_script != NULL) {
        Tcl_Interp* interp = rsc->server_interp;

        if (Tcl_EvalObjEx(interp, rsc->rivet_server_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running ServerInitScript '%s': %s",
                         Tcl_GetString(rsc->rivet_server_init_script),
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": ServerInitScript '%s' successful", 
                         Tcl_GetString(rsc->rivet_server_init_script));
        }
    }
    Tcl_SetPanicProc(Rivet_Panic);
    
    return OK;
}

static int
Rivet_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    apr_status_t aprrv;
    char errorbuf[ERRORBUF_SZ];
    char* mpm_model_handler = "rivet_prefork_mpm.so";
    char* mpm_model_path;

    /* Everything revolves around this structure */

    module_globals = apr_palloc(pPool,sizeof(mod_rivet_globals));

    /* Let's load the Rivet-MPM bridge */

    /* TODO: This kludge to load off-path MPM model share libraries has to be removed because is a security hole */

    if (apr_env_get (&mpm_model_path,"RIVET_MPM_BRIDGE",pTemp) != APR_SUCCESS)
    {
        mpm_model_path = apr_pstrcat(pTemp,RIVET_DIR,"/mpm/",mpm_model_handler,NULL);
    } 

    aprrv = apr_dso_load(&module_globals->dso_handle,mpm_model_path,pPool);
    if (aprrv == APR_SUCCESS)
    {
        apr_status_t                rv;
        apr_dso_handle_sym_t        func = NULL;

        rv = apr_dso_sym(&func,module_globals->dso_handle,"Rivet_MPM_Init");
        if (rv == APR_SUCCESS)
        {
            module_globals->mpm_init = (int (*)(apr_pool_t*,server_rec*)) func;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error loading symbol Rivet_MPM_Init: %s", 
                         apr_dso_error(module_globals->dso_handle,errorbuf,ERRORBUF_SZ));
            exit(1);   
        }

        rv = apr_dso_sym(&func,module_globals->dso_handle,"Rivet_MPM_Request");
        if (rv == APR_SUCCESS)
        {
            module_globals->mpm_request = (int (*)(request_rec*)) func;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error loading symbol Rivet_MPM_Request: %s", 
                         apr_dso_error(module_globals->dso_handle,errorbuf,ERRORBUF_SZ));
            exit(1);   
        }

        rv = apr_dso_sym(&func,module_globals->dso_handle,"Rivet_MPM_Finalize");
        if (rv == APR_SUCCESS)
        {
            module_globals->mpm_finalize = (apr_status_t (*)(void *)) func;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error loading symbol Rivet_MPM_Finalize: %s", 
                         apr_dso_error(module_globals->dso_handle,errorbuf,ERRORBUF_SZ));
            exit(1);   
        }

        /* active threads count */
        
        apr_atomic_init(pPool);
        module_globals->threads_count = (apr_uint32_t *) apr_palloc(pPool,sizeof(apr_uint32_t));
        apr_atomic_set32(module_globals->threads_count,0);

        module_globals->server_shutdown = 0;

        Rivet_InitHandler (pPool,pLog,pTemp,s);
    }
    else
    {

        /* If we don't find the mpm handler module we give up */

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME " Error loading MPM manager: %s", 
                     apr_dso_error(module_globals->dso_handle,errorbuf,1024));
        exit(1);   
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

static void Rivet_ChildInit (apr_pool_t *pChild, server_rec *s)
{
    (*module_globals->mpm_init)(pChild,s);
    apr_pool_cleanup_register (pChild, s, module_globals->mpm_finalize, module_globals->mpm_finalize);
}

static int Rivet_Handler (request_rec *r)    
{
    return (*module_globals->mpm_request)(r);
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

