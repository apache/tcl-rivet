/* mod_rivet.c -- The apache module itself, for Apache 2.x. */

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

/* $Id$ */

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <sys/stat.h>
#include <string.h>

/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include <dlfcn.h>

/* Apache includes */
#include <httpd.h>
#include <http_config.h>
#include <http_request.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_log.h>
#include <http_main.h>
#include <util_script.h>
//#include "http_conf_globals.h"
#include <http_config.h>

#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_tables.h>

#include <ap_mpm.h>

/* Tcl includes */
#include <tcl.h>
/* There is code ifdef'ed out below which uses internal
 * declerations. */
/* #include <tclInt.h> */

/* Rivet Includes */
#include "mod_rivet.h"
#include "rivet.h"
#include "rivetParser.h"
#include "rivetChannel.h"
#include "apache_config.h"

#define MODNAME "mod_rivet"

//module AP_MODULE_DECLARE_DATA rivet_module;

/* This is used *only* in the PanicProc.  Otherwise, don't touch it! */
static request_rec  *rivet_panic_request_rec = NULL;
static apr_pool_t   *rivet_panic_pool        = NULL;
static server_rec   *rivet_panic_server_rec  = NULL;

/* Need some arbitrary non-NULL pointer which can't also be a request_rec */
#define NESTED_INCLUDE_MAGIC    (&rivet_module)
#define DEBUG(s) fprintf(stderr, s), fflush(stderr)

/* rivet or tcl file */
#define CTYPE_NOT_HANDLED   0
#define RIVET_TEMPLATE      1
#define RIVET_TCLFILE       2

/* rivet return codes */
#define RIVET_OK            0
#define RIVET_ERROR         1
 
TCL_DECLARE_MUTEX(sendMutex);

#define RIVET_TEMPLATE_CTYPE    "application/x-httpd-rivet"
#define RIVET_TCLFILE_CTYPE     "application/x-rivet-tcl"

/* 
 * for some reason the max buffer size definition is not exported by Tcl 
 * we steal and reproduce it here prepending the name with TCL
 */

#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024*1024)

static Tcl_Interp* Rivet_CreateTclInterp (server_rec* s);
static void Rivet_CreateCache (server_rec *s, apr_pool_t *p);
static apr_status_t Rivet_ChildExit(void *data);

mod_rivet_globals* rivet_module_globals = NULL;

/*
 * -- Rivet_chdir_file (const char* filename)
 * 
 * Determines the directory name from the filename argument
 * and sets it as current working directory
 *
 * Argument:
 * 
 *   const char* filename:  file name to be used for determining
 *                          the current directory (URI style path)
 *                          the directory name is everything comes
 *                          before the last '/' (slash) character
 *
 * This snippet of code came from the mod_ruby project, which is under a BSD license.
 */
 
static int Rivet_chdir_file (const char *file)
{
    const char  *x;
    int         chdir_retval = 0;
    char        chdir_buf[HUGE_STRING_LEN];

    x = strrchr(file, '/');
    if (x == NULL) {
        chdir_retval = chdir(file);
    } else if (x - file < sizeof(chdir_buf) - 1) {
        memcpy(chdir_buf, file, x - file);
        chdir_buf[x - file] = '\0';
        chdir_retval = chdir(chdir_buf);
    }
        
    return chdir_retval;
}


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

static int
Rivet_CheckType (request_rec *req)
{
    int ctype = CTYPE_NOT_HANDLED;

    if ( req->content_type != NULL ) {
        if( STRNEQU( req->content_type, RIVET_TEMPLATE_CTYPE) ) {
            ctype  = RIVET_TEMPLATE;
        } else if( STRNEQU( req->content_type, RIVET_TCLFILE_CTYPE) ) {
            ctype = RIVET_TCLFILE;
        } 
    }
    return ctype; 
}


/*
 * -- Rivet_InitServerVariables
 *
 * Setup an array in each interpreter to tell us things about Apache.
 * This saves us from having to do any real call to load an entire
 * environment.  This routine only gets called once, when the child process
 * is created.
 *
 *  Arguments:
 *
 *      Tcl_Interp* interp: pointer to the Tcl interpreter
 *      apr_pool_t* pool: pool used for calling Apache framework functions
 *
 * Returned value:
 *      none
 *
 * Side effects:
 *
 *      within the global scope of the interpreter passed as first
 *      argument a 'server' array is created and the variable associated
 *      to the following keys are defined
 *
 *          SERVER_ROOT - Apache's root location
 *          SERVER_CONF - Apache's configuration file
 *          RIVET_DIR   - Rivet's Tcl source directory
 *          RIVET_INIT  - Rivet's init.tcl file
 *          RIVET_VERSION - Rivet version (only when RIVET_DISPLAY_VERSION is 1)
 *          MPM_THREADED - It should contain the string 'unsupported' for a prefork MPM
 *          MPM_FORKED - String describing the forking model of the MPM 
 *
 */

static void
Rivet_InitServerVariables( Tcl_Interp *interp, apr_pool_t *pool )
{
    int     ap_mpm_result;
    Tcl_Obj *obj;

    obj = Tcl_NewStringObj(ap_server_root, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_ROOT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool,SERVER_CONFIG_FILE), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_CONF",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool, RIVET_DIR), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_DIR",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool, RIVET_INIT), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_INIT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

#if RIVET_DISPLAY_VERSION
    obj = Tcl_NewStringObj(RIVET_VERSION, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_VERSION",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
#endif

    if (ap_mpm_query(AP_MPMQ_IS_THREADED,&ap_mpm_result) == APR_SUCCESS)
    {
        switch (ap_mpm_result) 
        {
            case AP_MPMQ_STATIC:
                obj = Tcl_NewStringObj("static", -1);
                break;
            case AP_MPMQ_NOT_SUPPORTED:
                obj = Tcl_NewStringObj("unsupported", -1);
                break;
            default: 
                obj = Tcl_NewStringObj("undefined", -1);
                break;
        }
        Tcl_IncrRefCount(obj);
        Tcl_SetVar2Ex(interp,"server","MPM_THREADED",obj,TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(obj);
    }

    if (ap_mpm_query(AP_MPMQ_IS_FORKED,&ap_mpm_result) == APR_SUCCESS)
    {
        switch (ap_mpm_result) 
        {
            case AP_MPMQ_STATIC:
                obj = Tcl_NewStringObj("static", -1);
                break;
            case AP_MPMQ_DYNAMIC:
                obj = Tcl_NewStringObj("dynamic", -1);
                break;
            default: 
                obj = Tcl_NewStringObj("undefined", -1);
                break;
        }
        Tcl_IncrRefCount(obj);
        Tcl_SetVar2Ex(interp,"server","MPM_FORKED",obj,TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(obj);
    }
}

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
    int                     result;
    rivet_server_conf*      conf = Rivet_GetConf(req);
    rivet_interp_globals*   globals = Tcl_GetAssocData(interp, "rivet", NULL);

    Tcl_Preserve (interp);
    result = Tcl_EvalObjEx(interp, tcl_script_obj, 0);

    if (result == TCL_ERROR) {

        Tcl_Obj*    errscript;
        Tcl_Obj*    errorCodeListObj;
        Tcl_Obj*    errorCodeElementObj;
        char*       errorCodeSubString;

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
                        TclWeb_PrintError(errorinfo,0,globals->req);
                    }
                }
                goto good;
            }
        }

        Tcl_SetVar( interp, "errorOutbuf",Tcl_GetStringFromObj( tcl_script_obj, NULL ),TCL_GLOBAL_ONLY );

        /* If we don't have an error script, use the default error handler. */
        if (conf->rivet_error_script) {
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

good:
    
    /* Tcl_Flush moved to the end of Rivet_SendContent */

    Tcl_Release(interp);

    return result;
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
    result = Rivet_ExecuteAndCheck(interp, outbuf, req->req);
    Tcl_DecrRefCount(outbuf);

    return result;
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
    int result;
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

    result = Rivet_ExecuteAndCheck(interp, outbuf, req->req);
    Tcl_DecrRefCount(outbuf);

    return result;
}

static void
Rivet_CleanupRequest( request_rec *r )
{
#if 0
    apr_table_t *t;
    apr_array_header_t *arr;
    apr_table_entry_t  *elts;
    int i, nelts;
    Tcl_Obj *arrayName;
    Tcl_Interp *interp;

    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->per_dir_config );

    t = rsc->rivet_user_vars;
    arr   = (apr_array_header_t*) apr_table_elts( t );
    elts  = (apr_table_entry_t *) arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetUserConf", -1 );
    interp = rsc->server_interp;

    for( i = 0; i < nelts; ++i )
    {
        Tcl_UnsetVar2(interp,
                "RivetUserConf",
                elts[i].key,
                TCL_GLOBAL_ONLY);
    }
    Tcl_DecrRefCount(arrayName);

    rivet_server_conf *rdc = RIVET_SERVER_CONF( r->per_dir_config );

    if( rdc->rivet_before_script ) {
        Tcl_DecrRefCount( rdc->rivet_before_script );
    }
    if( rdc->rivet_after_script ) {
        Tcl_DecrRefCount( rdc->rivet_after_script );
    }
    if( rdc->rivet_error_script ) {
        Tcl_DecrRefCount( rdc->rivet_error_script );
    }
#endif
}

/*
 *-----------------------------------------------------------------------------
 *
 * -- Rivet_CreateRivetChannel
 *
 * Creates a channel and registers with to the interpreter
 *
 *  Arguments:
 *
 *     - Tcl_Interp*        interp: a Tcl interpreter object pointer
 *     - rivet_server_conf* rsc: the rivet_server_conf data structure
 *     - apr_pool_t*        pPool: a pointer to an APR memory pool
 *
 *  Returned value:
 *
 *     none
 *
 *  Side Effects:
 *
 *     a Tcl channel is created allocating memory from the pool and
 *     a reference to it is stored in the rivet_server_conf record
 *-----------------------------------------------------------------------------
 */
static void
Rivet_CreateRivetChannel(Tcl_Interp* interp,rivet_server_conf* rsc, apr_pool_t* pPool)
{
    rsc->outchannel    = apr_pcalloc (pPool, sizeof(Tcl_Channel));
    *(rsc->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_module_globals, TCL_WRITABLE);

    /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*(rsc->outchannel), TCL_STDOUT);

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*(rsc->outchannel), TCL_MAX_CHANNEL_BUFFER_SIZE);

    /* We register the Tcl channel to the interpreter */

    Tcl_RegisterChannel(rsc->server_interp, *(rsc->outchannel));
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
static void
Rivet_PerInterpInit(server_rec *s, rivet_server_conf *rsc, apr_pool_t *p, int new_channel)
{
    Tcl_Interp *interp = rsc->server_interp;
    rivet_interp_globals *globals = NULL;
    Tcl_Obj* auto_path = NULL;
    Tcl_Obj* rivet_tcl = NULL;

    ap_assert (interp != (Tcl_Interp *)NULL);
    Tcl_Preserve (interp);

    /* We don't generally need more than one channel as the child process
     * can serve a request at a time, but in case Rivet will ever be deployed
     * in large systems with many unrelated applications and separate developers
     * a private channel per interpreter can be created using SeparateChannels
     * in the conf. 
     */

    if (new_channel) 
    {
        Rivet_CreateRivetChannel(interp,rsc,p);
    }
    else
    {
        /* We register the Tcl channel to the interpreter */
        Tcl_RegisterChannel(interp, *(rsc->outchannel));
    }

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

static int
Rivet_InitHandler(apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    rivet_panic_pool       = pPool;
    rivet_panic_server_rec = s;

    rivet_module_globals = apr_palloc(pPool,sizeof(mod_rivet_globals));
    rivet_module_globals->rsc_p = rsc;
#if RIVET_DISPLAY_VERSION
    ap_add_version_component(pPool, RIVET_PACKAGE_NAME"/"RIVET_VERSION);
#else
    ap_add_version_component(pPool, RIVET_PACKAGE_NAME);
#endif

    FILEDEBUGINFO;

    /* we create and initialize a master (server) interpreter */

    rsc->server_interp = Rivet_CreateTclInterp(s) ; /* root interpreter */

    Rivet_PerInterpInit(s,rsc,pPool,1);

    /* we create also the cache */

    Rivet_CreateCache(s,pPool);

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
    
    return OK;
}



/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PanicProc --
 *
 *  Called when Tcl panics, usually because of memory problems.
 *  We log the request, in order to be able to determine what went
 *  wrong later.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Calls abort(), which does not return - the child exits.
 *
 *-----------------------------------------------------------------------------
 */
static void
Rivet_Panic TCL_VARARGS_DEF(CONST char *, arg1)
{
    va_list argList;
    char *buf;
    char *format;

    format = (char *) TCL_VARARGS_START(char *,arg1,argList);
    buf    = (char *) apr_pvsprintf(rivet_panic_pool, format, argList);

    if (rivet_panic_request_rec != NULL) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
                     rivet_panic_server_rec,
                     MODNAME ": Critical error in request: %s", 
                     rivet_panic_request_rec->unparsed_uri);
    }

    ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
                 rivet_panic_server_rec, "%s", buf);

    abort();
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
    Tcl_SetPanicProc(Rivet_Panic);

    return interp;
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

static void
Rivet_CreateCache (server_rec *s, apr_pool_t *p)
{
    extern int ap_max_requests_per_child;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );

    /* If the user didn't set a cache size in their configuration, we
     * will assume an arbitrary size for them.
     *
     * If the cache size is 0, the user has requested not to cache
     * documents.
     */

    if(*(rsc->cache_size) < 0) {
        if (ap_max_requests_per_child != 0) {
            *(rsc->cache_size) = ap_max_requests_per_child / 5;
        } else {
            *(rsc->cache_size) = 50;    // Arbitrary number
        }
    }

    if (*(rsc->cache_size) != 0) {
        *(rsc->cache_free) = *(rsc->cache_size);
    }

    // Initialize cache structures

    if (*(rsc->cache_size)) {
        rsc->objCacheList = apr_pcalloc(
                p, (signed)(*(rsc->cache_size) * sizeof(char *)));
        rsc->objCache = apr_pcalloc(p, sizeof(Tcl_HashTable));
        Tcl_InitHashTable(rsc->objCache, TCL_STRING_KEYS);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitTclStuff --
 *
 *  Initialize the Tcl system - create interpreters, load commands
 *  and so forth.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_InitTclStuff(server_rec *s, apr_pool_t *p)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    Tcl_Interp *interp = rsc->server_interp;
    rivet_server_conf *myrsc;
    server_rec *sr;
    extern int ap_max_requests_per_child;
    int interpCount = 0;

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

    if (rsc->rivet_global_init_script != NULL) {
        if (Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running GlobalInitScript '%s': %s",
                         Tcl_GetString(rsc->rivet_global_init_script),
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": GlobalInitScript '%s' successful",
                         Tcl_GetString(rsc->rivet_global_init_script));
        }
    }

    for (sr = s; sr; sr = sr->next)
    {
        myrsc = RIVET_SERVER_CONF(sr->module_config);

        /* We only have a different rivet_server_conf if MergeConfig
         * was called. We really need a separate one for each server,
         * so we go ahead and create one here, if necessary. */

        if (sr != s && myrsc == rsc) {
            myrsc = RIVET_NEW_CONF(p);
            ap_set_module_config(sr->module_config, &rivet_module, myrsc);
            Rivet_CopyConfig( rsc, myrsc );
        }

        myrsc->outchannel = rsc->outchannel;

        /* This sets up slave interpreters for other virtual hosts. */
        if (sr != s) /* not the first one  */
        {
            if (rsc->separate_virtual_interps != 0) {
                char *slavename = (char*) apr_psprintf (p, "%s_%d_%d", 
                        sr->server_hostname, 
                        sr->port,
                        interpCount++);

                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                            MODNAME 
": Rivet_InitTclStuff: creating slave interpreter '%s', hostname '%s', port '%d', separate interpreters %d",
                            slavename, sr->server_hostname, sr->port, 
                            rsc->separate_virtual_interps);

                /* Separate virtual interps. */
                myrsc->server_interp = Tcl_CreateSlave(interp, slavename, 0);
                if (myrsc->server_interp == NULL) {
                    ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                                 MODNAME ": slave interp create failed: %s",
                                 Tcl_GetStringResult(interp) );
                    exit(1);
                }
                Rivet_PerInterpInit(s, myrsc, p, rsc->separate_channels);
            } else {
                myrsc->server_interp = rsc->server_interp;
            }

            /* Since these things are global, we copy them into the
             * rivet_server_conf struct. */
            myrsc->cache_size = rsc->cache_size;
            myrsc->cache_free = rsc->cache_free;
            myrsc->objCache = rsc->objCache;
            myrsc->objCacheList = rsc->objCacheList;
        }
        myrsc->server_name = (char*)apr_pstrdup(p, sr->server_hostname);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildHandlers --
 *
 *  Handles, depending on the situation, the scripts for the init
 *  and exit handlers.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Runs the rivet_child_init/exit_script scripts.
 *
 *-----------------------------------------------------------------------------
 */
static void
Rivet_ChildHandlers(server_rec *s, int init)
{
    server_rec *sr;
    rivet_server_conf *rsc;
    rivet_server_conf *top;
    void *function;
    void *parentfunction;
    char *errmsg;

    top = RIVET_SERVER_CONF(s->module_config);
    if (init == 1) {
        parentfunction = top->rivet_child_init_script;
        errmsg = MODNAME ": Error in Child init script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child init script: %s");
    } else {
        parentfunction = top->rivet_child_exit_script;
        errmsg = MODNAME ": Error in Child exit script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child exit script: %s");
    }

    for (sr = s; sr; sr = sr->next)
    {
        rsc = RIVET_SERVER_CONF(sr->module_config);
        function = init ? rsc->rivet_child_init_script : rsc->rivet_child_exit_script;

        if (!init && sr == s) {
            Tcl_Preserve(rsc->server_interp);
        }

        /* Execute it if it exists and it's the top level, separate
         * virtual interps are turned on, or it's different than the
         * main script. 
         */

        if  (function &&
             ( sr == s || rsc->separate_virtual_interps || function != parentfunction))
        {
            rivet_interp_globals* globals = Tcl_GetAssocData( rsc->server_interp, "rivet", NULL );
            Tcl_Preserve (rsc->server_interp);

            globals->srec = sr;
            if (Tcl_EvalObjEx(rsc->server_interp,function, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                             errmsg, Tcl_GetString(function));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                             "errorCode: %s",
                        Tcl_GetVar(rsc->server_interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                             "errorInfo: %s",
                        Tcl_GetVar(rsc->server_interp, "errorInfo", 0));
            }
            Tcl_Release (rsc->server_interp);
        }

        if (!init) 
        {
            Tcl_UnregisterChannel(rsc->server_interp,*(rsc->outchannel));
        }

    }

    if (!init) {

    /*
     * Upon child exit we delete the master interpreter before the 
     * caller invokes Tcl_Finalize.  Even if we're running separate
     * virtual interpreters, we don't delete the slaves
     * as deleting the master implicitly deletes its slave interpreters.
     */

        rsc = RIVET_SERVER_CONF(s->module_config);
        if (!Tcl_InterpDeleted (rsc->server_interp)) {
            Tcl_DeleteInterp(rsc->server_interp);
        }
        Tcl_Release (rsc->server_interp);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildInit --
 *
 *  This function is run when each individual Apache child process
 *  is created.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Calls Tcl initialization function.
 *
 *-----------------------------------------------------------------------------
 */


static void
Rivet_ChildInit(apr_pool_t *pChild, server_rec *s)
{
    ap_assert (s != (server_rec *)NULL);

    Rivet_InitTclStuff(s, pChild);
    Rivet_ChildHandlers(s, 1);

    //cleanup
    apr_pool_cleanup_register (pChild, s, Rivet_ChildExit, Rivet_ChildExit);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildExit --
 *
 *  Run when each Apache child process is about to exit.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------------
 */

static apr_status_t
Rivet_ChildExit (void *data)
{
    server_rec *s = (server_rec*) data;
    ap_assert (s != (server_rec *)NULL);

    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, s, MODNAME ": Running ChildExit handler");
    Rivet_ChildHandlers(s, 0);

    return OK;
}

//TODO: clarify whether rsc or rdc

#define USE_APACHE_RSC

/* Set things up to execute a file, then execute */
static int
Rivet_SendContent(request_rec *r)
{
    int errstatus;
    int retval;
    int ctype;

    Tcl_Interp      *interp;
    static Tcl_Obj  *request_init = NULL;
    static Tcl_Obj  *request_cleanup = NULL;
    rivet_interp_globals *globals = NULL;
#ifdef USE_APACHE_RSC
    rivet_server_conf    *rsc = NULL;
#else
    rivet_server_conf    *rdc;
#endif

    ctype = Rivet_CheckType(r);  
    if (ctype == CTYPE_NOT_HANDLED) {
        return DECLINED;
    }

    Tcl_MutexLock(&sendMutex);

    /* Set the global request req to know what we are dealing with in
     * case we have to call the PanicProc. */
    rivet_panic_request_rec = r;

    rsc = Rivet_GetConf(r);
    rivet_module_globals->rsc_p = rsc;
    interp = rsc->server_interp;
    globals = Tcl_GetAssocData(interp, "rivet", NULL);

    /* Setting this pointer in globals is crucial as by assigning it
     * we signal to Rivet commands we are processing an HTTP request.
     * This pointer gets set to NULL just before we leave this function
     * making possible to invalidate command execution that could depend
     * on a valid request_rec
     */

    globals->r = r;
    globals->srec = r->server;

#ifndef USE_APACHE_RSC
    if (r->per_dir_config != NULL)
        rdc = RIVET_SERVER_CONF( r->per_dir_config );
    else
        rdc = rsc;
#endif

    r->allowed |= (1 << M_GET);
    r->allowed |= (1 << M_POST);
    r->allowed |= (1 << M_PUT);
    r->allowed |= (1 << M_DELETE);
    if (r->method_number != M_GET && r->method_number != M_POST && r->method_number != M_PUT && r->method_number != M_DELETE) {
        retval = DECLINED;
        goto sendcleanup;
    }

    if (r->finfo.filetype == 0)
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, 
                     r->server,
                     MODNAME ": File does not exist: %s",
                     (r->path_info ? (char*)apr_pstrcat(r->pool, r->filename, r->path_info, NULL) : r->filename));
        retval = HTTP_NOT_FOUND;
        goto sendcleanup;
    }

    if ((errstatus = ap_meets_conditions(r)) != OK) {
        retval = errstatus;
        goto sendcleanup;
    }

    /* 
     * This one is the big catch when it comes to moving towards
     * Apache 2.0, or one of them, at least.
     */

    if (Rivet_chdir_file(r->filename) < 0)
    {
        /* something went wrong doing chdir into r->filename, we are not specific
         * at this. We simply emit an internal server error and print a log message
         */
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": Error accessing %s, could not chdir into directory", 
                     r->filename);

        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }

    /* Initialize this the first time through and keep it around. */
    if (request_init == NULL) {
        request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
        Tcl_IncrRefCount(request_init);
    }

    if (Tcl_EvalObjEx(interp, request_init, 0) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                            MODNAME ": Could not create request namespace (%s)\n" ,
                            Tcl_GetStringResult(interp));
        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }

    /* Apache Request stuff */

    TclWeb_InitRequest(globals->req, interp, r);
    ApacheRequest_set_post_max(globals->req->apachereq, rsc->upload_max);
    ApacheRequest_set_temp_dir(globals->req->apachereq, rsc->upload_dir);

#if 0
    if (upload_files_to_var)
    {
        globals->req->apachereq->hook_data = interp;
        globals->req->apachereq->upload_hook = Rivet_UploadHook;
    }
#endif

    errstatus = ApacheRequest_parse(globals->req->apachereq);

    if (errstatus != OK) {
        retval = errstatus;
        goto sendcleanup;
    }

    if (r->header_only && !rsc->honor_header_only_reqs)
    {
        TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, globals->req);
        TclWeb_PrintHeaders(globals->req);
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
 
/*
 * if strlen(req->content_type) > strlen([RIVET|TCL]_FILE_CTYPE)
 * a charset parameters might be there 
 */

    {
        int content_type_len = strlen(r->content_type);

        if (((ctype==RIVET_TEMPLATE) && (content_type_len > strlen(RIVET_TEMPLATE_CTYPE))) || \
             ((ctype==RIVET_TCLFILE) && (content_type_len > strlen(RIVET_TCLFILE_CTYPE)))) {
            
            char* charset;

            /* we parse the content type: we are after a 'charset' parameter definition */
            
            charset = strstr(r->content_type,"charset");
            if (charset != NULL) {
                charset = apr_pstrdup(r->pool,charset);

                /* ther's some freedom about spaces in the AddType lines: let's strip them off */

                apr_collapse_spaces(charset,charset);
                globals->req->charset = charset;
            }
        }
    }

    if (Rivet_ParseExecFile(globals->req, r->filename, 1) != TCL_OK)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": Error parsing exec file '%s': %s",
                     r->filename,
                     Tcl_GetVar(interp, "errorInfo", 0));
    }

    if (request_cleanup == NULL) {
        request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
        Tcl_IncrRefCount(request_cleanup);
    }

    if (Tcl_EvalObjEx(interp, request_cleanup, 0) == TCL_ERROR) {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                     MODNAME ": Error evaluating cleanup request: %s",
                     Tcl_GetVar(interp, "errorInfo", 0));
    }

    /* We execute also the AfterEveryScript if one was set */

    if (rsc->after_every_script) {
        if (Tcl_EvalObjEx(interp,rsc->after_every_script,0) == TCL_ERROR)
        {
            CONST84 char *errorinfo = Tcl_GetVar( interp, "errorInfo", 0 );
            TclWeb_PrintError("<b>Rivet AfterEveryScript failed!</b>",1,globals->req);
            TclWeb_PrintError( errorinfo, 0, globals->req );
        }
    }

    /* Reset globals */
    Rivet_CleanupRequest(r);

    retval = OK;
sendcleanup:

    /* Everything is done and we flush the rivet channel before resetting the status */

    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(*(rsc->outchannel));

    globals->req->content_sent = 0;
    globals->page_aborting = 0;
    if (globals->abort_code != NULL)
    {
        Tcl_DecrRefCount(globals->abort_code);
        globals->abort_code = NULL;
    }

    /* We reset this pointer to signal we have terminated the request processing */

    globals->r = NULL;

    Tcl_MutexUnlock(&sendMutex);
    return retval;
}

static void
rivet_register_hooks (apr_pool_t *p)
{
    //static const char * const aszPre[] = {
    //    "http_core.c", "mod_mime.c", NULL };
    //static const char * const aszPreTranslate[] = {"mod_alias.c", NULL};

    ap_hook_post_config (Rivet_InitHandler, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler (Rivet_SendContent, NULL, NULL, APR_HOOK_LAST);
    ap_hook_child_init (Rivet_ChildInit, NULL, NULL, APR_HOOK_LAST);
}

/* mod_rivet basic structures */

const command_rec rivet_cmds[] =
{
    AP_INIT_TAKE2 ("RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, NULL),
    AP_INIT_TAKE2 ("RivetDirConf", Rivet_DirConf, NULL, ACCESS_CONF, NULL),
    AP_INIT_TAKE2 ("RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO,
                   "RivetUserConf key value: sets RivetUserConf(key) = value"),
    {NULL}
};

module AP_MODULE_DECLARE_DATA rivet_module =
{
    STANDARD20_MODULE_STUFF,
    Rivet_CreateDirConfig,
    Rivet_MergeDirConfig,
    Rivet_CreateConfig,
    Rivet_MergeConfig,
    rivet_cmds,
    rivet_register_hooks
};


