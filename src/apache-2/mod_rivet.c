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
#define RIVET_FILE          1
#define TCL_FILE            2

/* rivet return codes */
#define RIVET_OK            0
#define RIVET_ERROR         1
 
TCL_DECLARE_MUTEX(sendMutex);

#define RIVET_FILE_CTYPE    "application/x-httpd-rivet"
#define TCL_FILE_CTYPE      "application/x-rivet-tcl"

static Tcl_Interp* Rivet_CreateTclInterp (server_rec* s);
static void Rivet_CreateCache (server_rec *s, apr_pool_t *p);


/* This snippet of code came from the mod_ruby project, which is under a BSD license. */
 
static int Rivet_chdir_file (const char *file)
{
    const  char *x;
    int    chdir_retval = 0;
    char chdir_buf[HUGE_STRING_LEN];

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

/* Function to be used should we desire to upload files to a variable */

#if 0
int
Rivet_UploadHook(void *ptr, char *buf, int len, ApacheUpload *upload)
{
    Tcl_Interp *interp = ptr;
    static int usenum = 0;
    static int uploaded = 0;

    if (oldptr != upload)
    {
    } else {
    }

    return len;
}
#endif /* 0 */

static int
Rivet_CheckType (request_rec *req)
{
    int ctype = CTYPE_NOT_HANDLED;

    if ( req->content_type != NULL ) {
        if( STRNEQU( req->content_type, RIVET_FILE_CTYPE) ) {
            ctype  = RIVET_FILE;
        } else if( STRNEQU( req->content_type, TCL_FILE_CTYPE) ) {
            ctype = TCL_FILE;
        } 
    }
    return ctype; 
}

/*
 * Rivet_ParseFileArgString (char *szDocRoot, char *szArgs, char **file)
 *
 * parses a string like /path/file.ext?arg1=value1&arg2=value2 into
 * the filename and arguments. A Tcl_Hashtable is utilized for the
 * arguments and returned. The file name is appended to the szDocRoot
 * argument and stored in the *file pointer.
 */
#if 0
static int
Rivet_ParseFileArgString (const char *szDocRoot, const char *szArgs, char **file,apr_pool_t *p, 
        Tcl_HashTable *argTbl)
{
    int flen = 1, argslen, newEntry, i;
    const char *rootPtr = szDocRoot, *argsPtr = szArgs, *argPos;
    char *filePtr; 
    char *argument, *value;
    Tcl_HashEntry *entryPtr;
    
    /* 
     * parse filename, that is everything before ? in the args string.
     * Append it to the document root, if this is not NULL. 
     */

    while (*argsPtr != '?' && *argsPtr != '\0') {
        argsPtr++;
    }
    if (*argsPtr == '?') 
        argslen = argsPtr-szArgs ;
    else
        argslen = 0;
    flen += strlen(szDocRoot) + argslen ;
    *file = (char*)apr_palloc(p,flen);
    strcat(*file,szDocRoot);
    strncat(*file,szArgs,argslen);

    if (argTbl == NULL)
        return RIVET_OK;
    
    /*
     * past this point, parse remainder of the args string. For every argument, 
     * create a new entry in the argTbl Tcl_HashTable.
     */
    Tcl_InitHashTable (argTbl, TCL_STRING_KEYS);
    
    for (i = 0, argPos = argsPtr; i < strlen(argsPtr); i++) {
        if (*(argsPtr + i) == '=') {
            /* argument ends */
            argument = (char*) malloc ((strlen(szArgs)+1) * sizeof(char));
            char *ap = argument;
            while ((*ap++ = *argPos++) != '=')
                ;
            *--ap = '\0';
        } else if (*(argsPtr + i) == '&' || i == strlen(argsPtr)-1) {
            /* value ends */
            value = (char*) malloc ((strlen(szArgs)+1) * sizeof(char));
            char *vp = value;
            while ((*vp++ = *argPos++) != '&')
                ;
            *--vp = '\0';

            if (argument == NULL) {
                argument = value;
                value = "1";
            }
            entryPtr = Tcl_CreateHashEntry (argTbl, (CONST char*) argument, 
                    &newEntry);
            Tcl_SetHashValue (entryPtr, value);
            argument = NULL;
            value = NULL;
        }
    }
    
    return RIVET_OK;
}
#endif

/*
 * Setup an array in each interpreter to tell us things about Apache.
 * This saves us from having to do any real call to load an entire
 * environment.  This routine only gets called once, when the child process
 * is created.
 *
 * SERVER_ROOT - Apache's root location
 * SERVER_CONF - Apache's configuration file
 * RIVET_DIR   - Rivet's Tcl source directory
 * RIVET_INIT  - Rivet's init.tcl file
 */

static void
Rivet_InitServerVariables( Tcl_Interp *interp, apr_pool_t *p )
{
    Tcl_Obj *obj;

    obj = Tcl_NewStringObj(ap_server_root, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_ROOT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p,SERVER_CONFIG_FILE), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_CONF",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_DIR), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_DIR",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_INIT), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_INIT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

#if RIVET_DISPLAY_VERSION
    obj = Tcl_NewStringObj(RIVET_PACKAGE_VERSION, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_PACKAGE_VERSION",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
#endif
}

static void
Rivet_PropagateServerConfArray( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    apr_table_t *t;
    apr_array_header_t *arr;
    apr_table_entry_t  *elts;
    int i, nelts;
    Tcl_Obj *key;
    Tcl_Obj *val;
    Tcl_Obj *arrayName;

    /* Propagate all of the ServerConf variables into an array. */
    t = rsc->rivet_server_vars;
    arr   = (apr_array_header_t*) apr_table_elts( t );
    elts  = (apr_table_entry_t *) arr->elts;
    nelts = arr->nelts;

    arrayName = Tcl_NewStringObj("RivetServerConf", -1);
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
        key = Tcl_NewStringObj( elts[i].key, -1);
        val = Tcl_NewStringObj( elts[i].val, -1);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);
        Tcl_ObjSetVar2(interp,
                arrayName,
                key,
                val,
                TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);
}

/* Calls Tcl_EvalObjEx() and checks for errors
 * Prints the error buffer if any.
 */
static int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *req)
{
    rivet_server_conf *conf = Rivet_GetConf(req);
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    Tcl_Preserve (interp);
    if ( Tcl_EvalObjEx(interp, outbuf, 0) == TCL_ERROR ) {
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
            if (strcmp (errorCodeSubString, "ABORTPAGE") == 0) 
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

        Tcl_SetVar( interp, "errorOutbuf",
                Tcl_GetStringFromObj( outbuf, NULL ),
                TCL_GLOBAL_ONLY );

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
    Tcl_Flush(*(conf->outchannel));
    Tcl_Release(interp);

    return TCL_OK;
}

/* This is a separate function so that it may be called from 'Parse' */
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
        //ctime = req->req->finfo.st_ctime;
        //mtime = req->req->finfo.st_mtime;
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
//              Tcl_NewStringObj(rsc->rivet_before_script, -1));
            }
        }

/*
 * We check whether we are dealing with a pure Tcl script or a Rivet template.
 * Actually this check is done only if we are processing a toplevel file, every nested 
 * file (files included through the 'parse' command) is treated as a template.
 */

        if (!toplevel || (Rivet_CheckType(req->req) == RIVET_FILE))
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
        if (toplevel) {
            if (rsc->rivet_after_script) {
//              Tcl_AppendObjToObj(outbuf,Tcl_NewStringObj(rsc->rivet_after_script, -1));
                Tcl_AppendObjToObj(outbuf,rsc->rivet_after_script);
            }
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
 * Rivet_CopyConfig --
 *
 *  Copy the rivet_server_conf struct.
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
Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
{
    FILEDEBUGINFO;

    newrsc->server_interp = oldrsc->server_interp;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;
    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;
    newrsc->rivet_abort_script = oldrsc->rivet_abort_script;
    newrsc->after_every_script = oldrsc->after_every_script;

    newrsc->user_scripts_updated = oldrsc->user_scripts_updated;

    newrsc->rivet_default_error_script = oldrsc->rivet_default_error_script;

    /* These are pointers so that they can be passed around... */
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->upload_max = oldrsc->upload_max;
    newrsc->upload_files_to_var = oldrsc->upload_files_to_var;
    newrsc->separate_virtual_interps = oldrsc->separate_virtual_interps;
    newrsc->honor_header_only_reqs = oldrsc->honor_header_only_reqs;
    newrsc->server_name = oldrsc->server_name;
    newrsc->upload_dir = oldrsc->upload_dir;
    newrsc->rivet_server_vars = oldrsc->rivet_server_vars;
    newrsc->rivet_dir_vars = oldrsc->rivet_dir_vars;
    newrsc->rivet_user_vars = oldrsc->rivet_user_vars;
    newrsc->objCacheList = oldrsc->objCacheList;
    newrsc->objCache = oldrsc->objCache;

    newrsc->outchannel = oldrsc->outchannel;
}

/*
 * Merge the per-directory configuration options into a new configuration.
 */

static void
Rivet_MergeDirConfigVars(apr_pool_t *p, rivet_server_conf *new,
              rivet_server_conf *base, rivet_server_conf *add )
{
    FILEDEBUGINFO;

    new->rivet_before_script = add->rivet_before_script ?
        add->rivet_before_script : base->rivet_before_script;
    new->rivet_after_script = add->rivet_after_script ?
        add->rivet_after_script : base->rivet_after_script;
    new->rivet_error_script = add->rivet_error_script ?
        add->rivet_error_script : base->rivet_error_script;
    new->rivet_abort_script = add->rivet_abort_script ?
        add->rivet_abort_script : base->rivet_abort_script;
    new->after_every_script = add->after_every_script ?
        add->after_every_script : base->after_every_script;

    new->user_scripts_updated = add->user_scripts_updated ?
        add->user_scripts_updated : base->user_scripts_updated;

    new->upload_dir = add->upload_dir ?
        add->upload_dir : base->upload_dir;

    /* Merge the tables of dir and user variables. */
    if (base->rivet_dir_vars && add->rivet_dir_vars) {
        new->rivet_dir_vars =
            apr_table_overlay ( p, base->rivet_dir_vars, add->rivet_dir_vars );
    } else {
        new->rivet_dir_vars = base->rivet_dir_vars;
    }
    if (base->rivet_user_vars && add->rivet_user_vars) {
        new->rivet_user_vars =
            apr_table_overlay ( p, base->rivet_user_vars, add->rivet_user_vars );
    } else {
        new->rivet_user_vars = base->rivet_user_vars;
    }
}

/* Function to get a config and merge the directory/server options  */
rivet_server_conf *
Rivet_GetConf( request_rec *r )
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->server->module_config );
    void *dconf = r->per_dir_config;
    rivet_server_conf *newconfig = NULL;
    rivet_server_conf *rdc;
    
    FILEDEBUGINFO;

    /* If there is no per dir config, just return the server config */
    if (dconf == NULL) {
        return rsc;
    }
    rdc = RIVET_SERVER_CONF( dconf ); 
    
    newconfig = RIVET_NEW_CONF( r->pool );

    Rivet_CopyConfig( rsc, newconfig );
    Rivet_MergeDirConfigVars( r->pool, newconfig, rsc, rdc );

    return newconfig;
}

static void *
Rivet_CreateConfig(apr_pool_t *p, server_rec *s )
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    rsc->server_interp          = NULL;

/* scripts obj pointers *must* be initialized to NULL */

    rsc->rivet_server_init_script   = NULL;
    rsc->rivet_global_init_script   = NULL;
    rsc->rivet_child_init_script    = NULL;
    rsc->rivet_child_exit_script    = NULL;
    rsc->rivet_before_script        = NULL;
    rsc->rivet_after_script         = NULL;
    rsc->rivet_error_script         = NULL;
    rsc->rivet_abort_script         = NULL;
    rsc->after_every_script         = NULL;

    rsc->user_scripts_updated = 0;

    rsc->rivet_default_error_script = Tcl_NewStringObj("::Rivet::handle_error", -1);
    Tcl_IncrRefCount(rsc->rivet_default_error_script);

    /* these are pointers so that they can be passed around...  */
    rsc->cache_size = apr_pcalloc(p, sizeof(int));
    rsc->cache_free = apr_pcalloc(p, sizeof(int));
    *(rsc->cache_size)              = -1;
    *(rsc->cache_free)              = 0;
    rsc->upload_max                 = RIVET_MAX_POST;
    rsc->upload_files_to_var        = RIVET_UPLOAD_FILES_TO_VAR;
    rsc->separate_virtual_interps   = RIVET_SEPARATE_VIRTUAL_INTERPS;
    rsc->honor_header_only_reqs     = RIVET_HEAD_REQUESTS;
    rsc->upload_dir                 = RIVET_UPLOAD_DIR;
    rsc->server_name                = NULL;
    rsc->objCacheList               = NULL;
    rsc->objCache                   = NULL;

    rsc->outchannel                 = NULL;

    rsc->rivet_server_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_dir_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_user_vars = (apr_table_t *) apr_table_make ( p, 4 );

    return rsc;
}

static void
Rivet_PropagatePerDirConfArrays( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    apr_table_t *t;
    apr_array_header_t *arr;
    apr_table_entry_t  *elts;
    int i, nelts;
    Tcl_Obj *arrayName;
    Tcl_Obj *key;
    Tcl_Obj *val;

    /* Make sure RivetDirConf doesn't exist from a previous request. */
    Tcl_UnsetVar( interp, "RivetDirConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the DirConf variables into an array. */
    t = rsc->rivet_dir_vars;
    arr   = (apr_array_header_t*) apr_table_elts( t );
    elts  = (apr_table_entry_t *) arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetDirConf", -1 );
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
        key = Tcl_NewStringObj( elts[i].key, -1);
        val = Tcl_NewStringObj( elts[i].val, -1);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);
        Tcl_ObjSetVar2(interp, arrayName, key, val, TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);

    /* Make sure RivetUserConf doesn't exist from a previous request. */
    Tcl_UnsetVar( interp, "RivetUserConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the UserConf variables into an array. */
    t = rsc->rivet_user_vars;
    arr   = (apr_array_header_t*) apr_table_elts( t );
    elts  = (apr_table_entry_t *)arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetUserConf", -1 );
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
        key = Tcl_NewStringObj( elts[i].key, -1);
        val = Tcl_NewStringObj( elts[i].val, -1);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);
        Tcl_ObjSetVar2(interp, arrayName, key, val, TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);
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
Rivet_PerInterpInit(server_rec *s, rivet_server_conf *rsc, apr_pool_t *p)
{
    Tcl_Interp *interp = rsc->server_interp;
    rivet_interp_globals *globals = NULL;
    Tcl_Obj* auto_path = NULL;
    Tcl_Obj* rivet_tcl = NULL;

    ap_assert (interp != (Tcl_Interp *)NULL);
    Tcl_Preserve (interp);

    /* Create TCL commands to deal with Apache's BUFFs. */
    rsc->outchannel = apr_pcalloc(p, sizeof(Tcl_Channel));
    *(rsc->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rsc, TCL_WRITABLE);

    Tcl_SetStdChannel(*(rsc->outchannel), TCL_STDOUT);

    /* Set up interpreter associated data */

    globals = apr_pcalloc(p, sizeof(rivet_interp_globals));
    Tcl_SetAssocData(interp,"rivet",NULL,globals);
    
    /* 
     * abort_page status variables in globals are set here and then 
     * reset in Rivet_SendContent just before the request processing is 
     * completed 
     */

    /* Rivet commands namespace is created */
    globals->rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,
                                            (Tcl_NamespaceDeleteProc *)NULL);
    globals->page_aborting = 0;
    globals->abort_code = NULL;

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    /* We put in front the auto_path list the path to the directory where
     * init.tcl is located (provides package RivetTcl)
     */

    auto_path = Tcl_GetVar2Ex(interp,"auto_path",NULL,TCL_GLOBAL_ONLY);

    rivet_tcl = Tcl_NewStringObj(RIVET_DIR,-1);
    Tcl_IncrRefCount(rivet_tcl);

    if (Tcl_ListObjReplace(interp,auto_path,0,0,1,&rivet_tcl) == TCL_ERROR)
    {
        ap_log_error( APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                    "error setting auto_path: %s",Tcl_GetStringFromObj(auto_path,NULL));
    } 
    Tcl_DecrRefCount(rivet_tcl);

    /* Initialize the interpreter with Rivet's Tcl commands. */
    Rivet_InitCore(interp);

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables(interp, p );
    Rivet_PropagateServerConfArray( interp, rsc );

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    if (Tcl_PkgRequire(interp, "RivetTcl", "2.1", 1) == NULL)
    {
        ap_log_error( APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                "init.tcl must be installed correctly for Apache Rivet to function: %s",
                Tcl_GetStringResult(interp) );
        exit(1);
    }

    /* Loading into the interpreter the commands provided by librivet.so */

   /* It would be nice to have to whole set of Rivet commands
    * loaded into the interpreter at this stage. Unfortunately
    * a problem with the dynamic loader of some OS prevents us 
    * from callingTcl_PkgRequire for 'rivetlib' because Apache segfaults
    * shortly after the extension library is loaded.
    * The problem was investigated on Linux and it became clear 
    * that it's linked to the way Tcl calls dlopen (Bug #3216070)
    * The problem could be solved in Tcl8.6 

    if (Tcl_PkgRequire(interp, RIVETLIB_TCL_PACKAGE, "1.2", 1) == NULL)
    {
        ap_log_error( APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                "Error loading rivetlib package: %s", Tcl_GetStringResult(interp) );
        exit(1);
    } 
    */

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*(rsc->outchannel), 1000000);
    Tcl_RegisterChannel(interp, *(rsc->outchannel));
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_AssignStringtoConf --
 *
 *  Assign a string to a Tcl_Obj valued configuration parameter
 *
 * Arguments:
 *
 *  - objPnt: Pointer to a pointer to a Tcl_Obj. If the pointer *objPnt
 *  is NULL (configuration script obj pointers are initialized to NULL)
 *      a new Tcl_Obj is created
 *  - string_value: a string to be assigned to the Tcl_Obj
 *
 * Results:
 *  
 *  - Pointer to a Tcl_Obj containing the parameter value.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj* 
Rivet_AssignStringToConf (Tcl_Obj** objPnt, const char* string_value)
{
    Tcl_Obj *objarg = NULL;
    
    if (*objPnt == NULL)
    {
        objarg = Tcl_NewStringObj(string_value,-1);
        Tcl_IncrRefCount(objarg);
        *objPnt = objarg;
    } else {
        objarg = *objPnt;
        Tcl_AppendToObj(objarg, string_value, -1);
    }
    Tcl_AppendToObj( objarg, "\n", 1 );
    return objarg;
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_SetScript --
 *
 *  Add the text from an apache directive, such as UserConf, to
 *  the corresponding variable in the rivet_server_conf structure.
 *  In most cases, we append the new value to any previously
 *  existing value, but Before, After and Error scripts override
 *  the old directive completely.
 *
 * Results:
 *
 *  Returns a Tcl_Obj* pointing to the string representation of 
 *  the current value for the directive.
 *
 *----------------------------------------------------------------------
 */


static const char *
Rivet_SetScript (apr_pool_t *pool, rivet_server_conf *rsc, const char *script, const char *string)
{
    Tcl_Obj *objarg = NULL;

    if( STREQU( script, "GlobalInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_global_init_script),string);
    } else if( STREQU( script, "ChildInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_child_init_script),string);
    } else if( STREQU( script, "ChildExitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_child_exit_script),string);
    } else if( STREQU( script, "BeforeScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_before_script),string);
    } else if( STREQU( script, "AfterScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_after_script),string);
    } else if( STREQU( script, "ErrorScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_error_script),string);
    } else if( STREQU( script, "ServerInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_server_init_script),string);
    } else if( STREQU( script, "AbortScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_abort_script),string);
    } else if( STREQU( script, "AfterEveryScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->after_every_script),string);
    }

    if( !objarg ) return string;

    return Tcl_GetStringFromObj( objarg, NULL );
}

/*
 * Implements the RivetServerConf Apache Directive
 *
 * Command Arguments:
 *
 *  RivetServerConf GlobalInitScript <script>
 *  RivetServerConf ChildInitScript <script>
 *  RivetServerConf ChildExitScript <script>
 *  RivetServerConf BeforeScript <script>
 *  RivetServerConf AfterScript <script>
 *  RivetServerConf ErrorScript <script>
 *  RivetServerConf CacheSize <integer>
 *  RivetServerConf UploadDirectory <directory>
 *  RivetServerConf UploadMaxSize <integer>
 *  RivetServerConf UploadFilesToVar <yes|no>
 *  RivetServerConf SeparateVirtualInterps <yes|no>
 *  RivetServerConf HonorHeaderOnlyRequests <yes|no> (2008-06-20: mm)
 */

static const char *
Rivet_ServerConf( cmd_parms *cmd, void *dummy, 
                  const char *var, const char *val )
{
    server_rec *s = cmd->server;
    rivet_server_conf *rsc = RIVET_SERVER_CONF(s->module_config);
    const char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetServerConf requires two arguments";
    }

    if( STREQU( var, "CacheSize" ) ) {
        *(rsc->cache_size) = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadDirectory" ) ) {
        rsc->upload_dir = val;
    } else if( STREQU( var, "UploadMaxSize" ) ) {
        rsc->upload_max = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadFilesToVar" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->upload_files_to_var);
    } else if( STREQU( var, "SeparateVirtualInterps" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->separate_virtual_interps);
    } else if( STREQU( var, "HonorHeaderOnlyRequests" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->honor_header_only_reqs);
    } else {
        string = Rivet_SetScript( cmd->pool, rsc, var, val);
    }

    if (string != NULL) apr_table_set( rsc->rivet_server_vars, var, string );
    return( NULL );
}

/*
 * Implements the RivetDirConf Apache Directive
 *
 * Command Arguments:
 *  RivetDirConf BeforeScript <script>
 *  RivetDirConf AfterScript <script>
 *  RivetDirConf ErrorScript <script>
 *  RivetDirConf UploadDirectory <directory>
 */

static const char *
Rivet_DirConf( cmd_parms *cmd, void *vrdc, 
               const char *var, const char *val )
{
    const char *string = val;
    rivet_server_conf *rdc = (rivet_server_conf *)vrdc;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetDirConf requires two arguments";
    }

    if( STREQU( var, "UploadDirectory" ) ) {
        rdc->upload_dir = val;
    } else {
        string = Rivet_SetScript( cmd->pool, rdc, var, val );
    }

    if (string != NULL) apr_table_set( rdc->rivet_dir_vars, var, string );
    return NULL;
}

/*
 * Implements the RivetUserConf Apache Directive
 *
 * Command Arguments:
 *  RivetUserConf BeforeScript <script>
 *  RivetUserConf AfterScript <script>
 *  RivetUserConf ErrorScript <script>
 */

static const char *
Rivet_UserConf( cmd_parms *cmd, void *vrdc, 
                const char *var, 
                const char *val )
{
    const char *string = val;
    rivet_server_conf *rdc = (rivet_server_conf *)vrdc;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetUserConf requires two arguments";
    }

    /* We have modified these scripts. */
    /* This is less than ideal though, because it will get set to 1
     * every time - FIXME. */
    rdc->user_scripts_updated = 1;

    string = Rivet_SetScript( cmd->pool, rdc, var, val );
    /* XXX Need to figure out what to do about setting the table.  */
    if (string != NULL) apr_table_set( rdc->rivet_user_vars, var, string );
    return NULL;
}

static int
Rivet_InitHandler(apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp,
       server_rec *s)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    rivet_panic_pool = pPool;
    rivet_panic_server_rec = s;

#if RIVET_DISPLAY_VERSION
    ap_add_version_component(pPool, RIVET_PACKAGE_NAME"/"RIVET_PACKAGE_VERSION);
#else
    ap_add_version_component(pPool, RIVET_PACKAGE_NAME);
#endif

    FILEDEBUGINFO;

    rsc->server_interp = Rivet_CreateTclInterp(s) ; /* root interpreter */

    Rivet_PerInterpInit(s, rsc, pPool);
    Rivet_CreateCache(s,pPool);

    /* we create and initialize a master (server) interpreter
     * Rivet_InitTclStuff(s,pPool);
     */

    if (rsc->rivet_server_init_script != NULL) {
        Tcl_Interp* interp = rsc->server_interp;

        if (Tcl_EvalObjEx(interp, rsc->rivet_server_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, "%s",
                    Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, APR_EGENERAL, s, 
                        "RivetServerInit has run");
        }

    }
    
    return OK;
}


static void *
Rivet_CreateDirConfig(apr_pool_t *p, char *dir)
{
    rivet_server_conf *rdc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    rdc->rivet_server_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rdc->rivet_dir_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rdc->rivet_user_vars = (apr_table_t *) apr_table_make ( p, 4 );

    return rdc;
}

static void *
Rivet_MergeDirConfig( apr_pool_t *p, void *basev, void *addv )
{
    rivet_server_conf *base = (rivet_server_conf *)basev;
    rivet_server_conf *add  = (rivet_server_conf *)addv;
    rivet_server_conf *new  = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    Rivet_MergeDirConfigVars( p, new, base, add );

    return new;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_MergeConfig --
 *
 *  This function is called when there is a config option set both
 *  at the 'global' level, and for a virtual host.  It "resolves
 *  the conflicts" so to speak, by creating a new configuration,
 *  and this function is where we get to have our say about how to
 *  go about doing that.  For most of the options, we override the
 *  global option with the local one.
 *
 * Results:
 *  Returns a new server configuration.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */
static void *
Rivet_MergeConfig(apr_pool_t *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);
    rivet_server_conf *base = (rivet_server_conf *) basev;
    rivet_server_conf *overrides = (rivet_server_conf *) overridesv;

    FILEDEBUGINFO;

    /* For completeness' sake, we list the fate of all the members of
     * the rivet_server_conf struct. */

    /* server_interp isn't set at this point. */
    /* rivet_global_init_script is global, not per server. */

    rsc->rivet_child_init_script = overrides->rivet_child_init_script ?
        overrides->rivet_child_init_script : base->rivet_child_init_script;

    rsc->rivet_child_exit_script = overrides->rivet_child_exit_script ?
        overrides->rivet_child_exit_script : base->rivet_child_exit_script;

    rsc->rivet_before_script = overrides->rivet_before_script ?
        overrides->rivet_before_script : base->rivet_before_script;

    rsc->rivet_after_script = overrides->rivet_after_script ?
        overrides->rivet_after_script : base->rivet_after_script;

    rsc->rivet_error_script = overrides->rivet_error_script ?
        overrides->rivet_error_script : base->rivet_error_script;

    rsc->rivet_default_error_script = overrides->rivet_default_error_script ?
        overrides->rivet_default_error_script : base->rivet_default_error_script;

    rsc->rivet_abort_script = overrides->rivet_abort_script ?
        overrides->rivet_abort_script : base->rivet_abort_script;

    rsc->after_every_script = overrides->after_every_script ?
        overrides->after_every_script : base->after_every_script;

    /* cache_size is global, and set up later. */
    /* cache_free is not set up at this point. */

    rsc->upload_max = overrides->upload_max ?
        overrides->upload_max : base->upload_max;

    rsc->separate_virtual_interps = base->separate_virtual_interps;
    rsc->honor_header_only_reqs = base->honor_header_only_reqs;

    /* server_name is set up later. */

    rsc->upload_dir = overrides->upload_dir ?
        overrides->upload_dir : base->upload_dir;

    rsc->rivet_server_vars = overrides->rivet_server_vars ?
        overrides->rivet_server_vars : base->rivet_server_vars;

    rsc->rivet_dir_vars = overrides->rivet_dir_vars ?
        overrides->rivet_dir_vars : base->rivet_dir_vars;

    rsc->rivet_user_vars = overrides->rivet_user_vars ?
        overrides->rivet_user_vars : base->rivet_user_vars;

    /* objCacheList is set up later. */
    /* objCache is set up later. */
    /* outchannel is set up later. */

    return rsc;
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
    buf = (char *) apr_pvsprintf(rivet_panic_pool, format, argList);

    if (rivet_panic_request_rec != NULL) {
    ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
         rivet_panic_server_rec,
         "Critical error in request: %s", 
         rivet_panic_request_rec->unparsed_uri);
    }

    ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
         rivet_panic_server_rec, buf);

    abort();
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
        errmsg = "Error in Child init script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child init script: %s");
    } else {
        parentfunction = top->rivet_child_exit_script;
        errmsg = "Error in Child exit script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child exit script: %s");
    }

    for (sr = s; sr; sr = sr->next)
    {
        rsc = RIVET_SERVER_CONF(sr->module_config);
        function = init ? rsc->rivet_child_init_script :
            rsc->rivet_child_exit_script;

        if (!init && sr == s) {
            Tcl_Preserve(rsc->server_interp);
        }

        /* Execute it if it exists and it's the top level, separate
         * virtual interps are turned on, or it's different than the
         * main script. */
        if(function &&
                ( sr == s || rsc->separate_virtual_interps ||
                  function != parentfunction))
        {
            Tcl_Preserve (rsc->server_interp);
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
 * Rivet_ChildExit --
 *
 *  Run when each Apache child process is about to exit.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Runs Tcl_Finalize.
 *
 *-----------------------------------------------------------------------------
 */

static apr_status_t
Rivet_ChildExit(void *data)
{
    server_rec *s = (server_rec*) data;
    Rivet_ChildHandlers(s, 0);
    Tcl_Finalize();
    return OK;
}

/*
 *-----------------------------------------------------------------------------
 * Rivet_CreateTclInterp --
 *
 * Arguments:
 *  server_rec* s: pointer to a server_rec structure
 * Results:
 *  pointer to a Tcl_Interp structure
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
                "Error in Tcl_CreateInterp, aborting\n");
        exit(1);
    }
    if (Tcl_Init(interp) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
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

    if (rsc->rivet_global_init_script != NULL) {
        if (Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, "%s",
                         Tcl_GetVar(interp, "errorInfo", 0));
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

                /* Separate virtual interps. */
                myrsc->server_interp = Tcl_CreateSlave(interp, slavename, 0);
                if (myrsc->server_interp == NULL) {
                    ap_log_error( APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                                    "slave interp create failed: %s",
                                    Tcl_GetStringResult(interp) );
                    exit(1);
                }
                Rivet_PerInterpInit(s, myrsc, p);
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
    rivet_server_conf   *rsc = NULL;
    rivet_server_conf   *rdc;

    ctype = Rivet_CheckType(r);  
    if (ctype == CTYPE_NOT_HANDLED) {
        return DECLINED;
    }

    Tcl_MutexLock(&sendMutex);

    /* Set the global request req to know what we are dealing with in
     * case we have to call the PanicProc. */
    rivet_panic_request_rec = r;

    rsc = Rivet_GetConf(r);
    interp = rsc->server_interp;
    globals = Tcl_GetAssocData(interp, "rivet", NULL);
    globals->r = r;
    globals->req = (TclWebRequest *)apr_pcalloc(r->pool, sizeof(TclWebRequest));

/* we will test against NULL to check if a charset was specified in the conf */

    globals->req->charset = NULL;

    if (r->per_dir_config != NULL)
        rdc = RIVET_SERVER_CONF( r->per_dir_config );
    else
        rdc = rsc;

    r->allowed |= (1 << M_GET);
    r->allowed |= (1 << M_POST);
    if (r->method_number != M_GET && r->method_number != M_POST) {
        retval = DECLINED;
        goto sendcleanup;
    }

    if (r->finfo.filetype == 0)
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server,
                "File does not exist: %s",
                (r->path_info
                 ? (char*)apr_pstrcat(r->pool, r->filename, r->path_info, NULL)
                 : r->filename));
        retval = HTTP_NOT_FOUND;
        goto sendcleanup;
    }

    if ((errstatus = ap_meets_conditions(r)) != OK) {
        retval = errstatus;
        goto sendcleanup;
    }

//    apr_cpystrn(error, DEFAULT_ERROR_MSG, sizeof(error));
//    apr_cpystrn(timefmt, DEFAULT_TIME_FORMAT, sizeof(timefmt));

    /* This one is the big catch when it comes to moving towards
       Apache 2.0, or one of them, at least. */
    if (Rivet_chdir_file(r->filename) < 0)
    {
        /* something went wrong doing chdir into r->filename, we are not specific
        at this. We simply emit an internal server error and print a log message
        */
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, r->server, 
                    "Error accessing %s, could not chdir into directory", r->filename);

        retval = HTTP_INTERNAL_SERVER_ERROR;
        goto sendcleanup;
    }

    //TODO: clarify whether rsc or rdc
    //Rivet_PropagatePerDirConfArrays( interp, rdc );
    Rivet_PropagatePerDirConfArrays( interp, rsc );

    /* Initialize this the first time through and keep it around. */
    if (request_init == NULL) {
        request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
        Tcl_IncrRefCount(request_init);
    }
    if (Tcl_EvalObjEx(interp, request_init, 0) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                "Could not create request namespace\n");
        retval = HTTP_BAD_REQUEST;
        goto sendcleanup;
    }

    /* Set the script name. */
    {
#if 1
        Tcl_Obj *infoscript = Tcl_NewStringObj("info script ", -1);
        Tcl_IncrRefCount(infoscript);
        Tcl_AppendToObj(infoscript, r->filename, -1);
        Tcl_EvalObjEx(interp, infoscript, TCL_EVAL_DIRECT);
        Tcl_DecrRefCount(infoscript);
#else
        /* This speeds things up, but you have to use Tcl internal
         * declerations, which is not so great... */
        Interp *iPtr = (Interp *) interp;
        if (iPtr->scriptFile != NULL) {
            Tcl_DecrRefCount(iPtr->scriptFile);
        }
        iPtr->scriptFile = Tcl_NewStringObj(r->filename, -1);
        Tcl_IncrRefCount(iPtr->scriptFile);
#endif
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

        if (((ctype==RIVET_FILE) && (content_type_len > strlen(RIVET_FILE_CTYPE))) || \
             ((ctype==TCL_FILE)  && (content_type_len > strlen(TCL_FILE_CTYPE)))) {
            
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
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, "%s",
                Tcl_GetVar(interp, "errorInfo", 0));
    }

    if (request_cleanup == NULL) {
        request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
        Tcl_IncrRefCount(request_cleanup);
    }

    if (Tcl_EvalObjEx(interp, request_cleanup, 0) == TCL_ERROR) {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, "%s",
                Tcl_GetVar(interp, "errorInfo", 0));
    }

    /* Reset globals */
    Rivet_CleanupRequest( r );

    retval = OK;
sendcleanup:
    globals->req->content_sent = 0;

    globals->page_aborting = 0;
    if (globals->abort_code != NULL)
    {
        Tcl_DecrRefCount(globals->abort_code);
        globals->abort_code = NULL;
    }

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


const command_rec rivet_cmds[] =
{
    AP_INIT_TAKE2("RivetServerConf", Rivet_ServerConf,
            NULL, RSRC_CONF, NULL),
    AP_INIT_TAKE2("RivetDirConf", Rivet_DirConf,
            NULL, ACCESS_CONF, NULL),
    AP_INIT_TAKE2("RivetUserConf", Rivet_UserConf, 
            NULL, ACCESS_CONF|OR_FILEINFO,
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


