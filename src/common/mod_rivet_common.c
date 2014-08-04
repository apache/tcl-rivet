/* Copyright 2000-2014 The Apache Software Foundation

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

/* $Id: mod_rivet.c 1609472 2014-07-10 15:08:52Z mxmanghi $ */

#include <httpd.h>
#include <apr_strings.h>
/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include "mod_rivet.h"
#include "TclWeb.h"
#include "rivetParser.h"
#include "rivet.h"

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
 
int Rivet_chdir_file (const char *file)
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

int
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

int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *tcl_script_obj, request_rec *req)
{
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
    Tcl_Flush(*(conf->outchannel));
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

