/* rivetCore.c -- Core commands which are compiled into mod_rivet itself */

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

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <sys/stat.h>

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_main.h"
#include "util_script.h"
#include "http_config.h"

#include <tcl.h>
#include <string.h>
#include <stdio.h>
#include <apr_errno.h>
#include <apr_strings.h>
#include <apr_portable.h>

#include "apache_request.h"
#include "mod_rivet.h"
#include "rivet.h"
#include "TclWeb.h"
/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivetParser.h"
#include "mod_rivet_generator.h"
#include "mod_rivet_cache.h"

#define ENV_ARRAY_NAME     "::request::env"
#define HEADERS_ARRAY_NAME "::request::headers"
#define COOKIES_ARRAY_NAME "cookies"

extern module rivet_module;
extern char* TclWeb_GetRawPost (TclWebRequest *req);
extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;

#define POOL (private->r->pool)

 /* define a convenience macro to cast the ClientData
  * into the thread private data pointer */

#define THREAD_PRIVATE_DATA(p)  p = (rivet_thread_private *)clientData;

/*
 * -- Rivet_NoRequestRec
 *
 * Adds standard error information to the interpreter. This procedure makes 
 * sense only when called by C functions implementing Tcl commands that
 * are meaningful only if a valid requiest_rec object is defined. These
 * procedures must return TCL_ERROR right away after Rivet_NoRequestRecord
 * returns
 *
 * Arguments:
 *
 *  Tcl_Interp*: current Tcl interpreter
 *  Tcl_Obj*: Tcl string object with the command name
 *
 * Results:
 *
 *  None
 *
 */

static void
Rivet_NoRequestRec (Tcl_Interp* interp, Tcl_Obj* command)
{
    Tcl_AddErrorInfo(interp, "Cannot call ");
    Tcl_AppendObjToErrorInfo(interp,command);
    Tcl_AppendObjToErrorInfo(interp,Tcl_NewStringObj(" outside a request processing",-1));
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_MakeURL --
 *
 *      Make a self-referencing URL.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_MakeURL )
{
    rivet_thread_private*   private;
    Tcl_Obj*                result  = NULL;
    char*                   url_target_name;
    int                     target_length;

    if (objc > 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::makeurl")
    if (objc == 1)
    {
        url_target_name = TclWeb_GetEnvVar (private,"SCRIPT_NAME");
    }
    else
    {
        url_target_name = Tcl_GetStringFromObj(objv[1],&target_length);

        // we check the first character for a '/' (absolute path)
        // If we are dealing with a relative path we prepend it with
        // the SCRIPT_NAME environment variable

        if (url_target_name[0] != '/')
        {
            /* relative path */
            char* script_name = TclWeb_GetEnvVar (private,"SCRIPT_NAME");
            size_t script_name_l = strlen(script_name);

            // regardless the reason for a SCRIPT_NAME being undefined we
            // prevent a segfault and we revert the behavior of makeurl
            // to the case of an absolute path

            if (script_name_l > 0)
            {
                // script name may have the form a directory path (and mod_rewrite 
                // could have mapped it to a .tcl or .rvt script)
                
                if (script_name[script_name_l-1] == '/')
                {
                    url_target_name = apr_pstrcat(private->req->req->pool,script_name,url_target_name,NULL);
                }
                else
                {
                    url_target_name = apr_pstrcat(private->req->req->pool,script_name,"/",url_target_name,NULL);
                }
            }
            else
            {
                url_target_name = apr_pstrcat(private->req->req->pool,"/",url_target_name,NULL);
            }
        }
    }

    result = Tcl_NewObj();   
    TclWeb_MakeURL(result, url_target_name, private->req);
    Tcl_SetObjResult(interp, result);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Parse --
 *
 *      Include and parse a Rivet file.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side Effects:
 *      Whatever occurs in the Rivet page parsed.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Parse )
{
    rivet_thread_private*   private;
    char*                   filename = 0;
    apr_status_t            stat_s;
    apr_finfo_t             finfo_b;
    char*                   cache_key;
    rivet_thread_interp*    rivet_interp;
    Tcl_HashEntry*          entry  = NULL;
    Tcl_Obj*                script = NULL;
    int                     result;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::parse")

    if( objc < 2 || objc > 3 )
    {
        Tcl_WrongNumArgs(interp, 1, objv, "?-virtual? filename");
        return TCL_ERROR;
    }

    if( objc == 2 ) {

        filename = Tcl_GetStringFromObj( objv[1], (int *)NULL );

    } else {

        if (STREQU( Tcl_GetStringFromObj(objv[1], (int *)NULL), "-virtual")) {

        /* */

            filename = TclWeb_GetVirtualFile(private->req,Tcl_GetStringFromObj(objv[2],(int *)NULL));

        } else if ( STREQU( Tcl_GetStringFromObj(objv[1], (int *)NULL), "-string")) {

            int      res;
            Tcl_Obj* script = objv[2];
            Tcl_Obj* outbuf = Tcl_NewObj();

            /* we parse and compose the script ourselves before passing it to Tcl_EvalObjEx */

            Tcl_IncrRefCount(outbuf);
            Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

            /* If we are not inside a <? ?> section, add the closing ". */
            if (Rivet_Parser(outbuf, script) == 0)
            {
                Tcl_AppendToObj(outbuf, "\"\n", 2);
            } 

            Tcl_AppendToObj(outbuf,"\n",-1);

            res = Tcl_EvalObjEx(interp,outbuf,0);

            Tcl_DecrRefCount(outbuf);
            return res;
            //return Rivet_ParseExecString(private, objv[2]);

        } else { 

            Tcl_WrongNumArgs( interp, 1, objv, "?-virtual? filename | -string template_string" );
            return TCL_ERROR;

        }

    }

    if (!strcmp(filename, private->r->filename))
    {
        Tcl_AddErrorInfo(interp, "Cannot recursively call the same file!");
        return TCL_ERROR;
    }

    stat_s = apr_stat(&finfo_b,filename,APR_FINFO_NORM,private->r->pool);
    if (stat_s != APR_SUCCESS)
    {
        char apr_error_message[RIVET_MSG_BUFFER_SIZE];

        Tcl_AddErrorInfo(interp,apr_strerror(stat_s,apr_error_message,RIVET_MSG_BUFFER_SIZE));
        return TCL_ERROR;
    }

    /* */

    cache_key = 
        RivetCache_MakeKey( private->pool,filename,
                            finfo_b.ctime,finfo_b.mtime,
                            IS_USER_CONF(private->running_conf),0);

    rivet_interp = RIVET_PEEK_INTERP(private,private->running_conf);
    entry = RivetCache_EntryLookup (rivet_interp,cache_key);

    if (entry == NULL)
    {
        script = Tcl_NewObj();
        Tcl_IncrRefCount(script);

        result = Rivet_GetRivetFile(filename,script,interp);
        if (result != TCL_OK)
        {
            Tcl_AddErrorInfo(interp,apr_pstrcat(private->pool,"Could not read file ",filename,NULL));
            Tcl_DecrRefCount(script);
            return result;
        }

        if (rivet_interp->cache_free > 0)
        {
            int isNew;
            Tcl_HashEntry* entry;

            entry = RivetCache_CreateEntry (rivet_interp,cache_key,&isNew);
            ap_assert(isNew == 1);
            RivetCache_StoreScript(rivet_interp,entry,script);
        }
        else if ((rivet_interp->flags & RIVET_CACHE_FULL) == 0)
        {
            rivet_interp->flags |= RIVET_CACHE_FULL;
            ap_log_error (APLOG_MARK, APLOG_NOTICE, APR_EGENERAL, private->r->server,"%s %s (%s),",
                                                                  "Rivet cache full when parsing ",
                                                                  private->r->filename,
                                                                  private->r->server->server_hostname);
        }

        result = Tcl_EvalObjEx(interp,script,0);
        Tcl_DecrRefCount(script);
        return result;
               
    } else {
        script = RivetCache_FetchScript(entry);
        return Tcl_EvalObjEx(interp,script,0); 
    }

}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Include --
 *
 *      Includes a file literally in the output stream.  Useful for
 *      images, plain HTML and the like.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Adds to the output stream.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Include )
{
    rivet_thread_private*   private;
    int                     sz;
    Tcl_Channel             fd;
    Tcl_Channel             tclstdout;
    Tcl_Obj*                outobj;
    char*                   filename;
    Tcl_DString             transoptions;
    Tcl_DString             encoptions;

    if( objc < 2 || objc > 3 )
    {
        Tcl_WrongNumArgs(interp, 1, objv, "?-virtual? filename");
        return TCL_ERROR;
    }

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::include")

    if( objc == 2 ) {
        filename = Tcl_GetStringFromObj( objv[1], (int *)NULL );
    } else {
        if( !STREQU( Tcl_GetStringFromObj(objv[1], (int *)NULL), "-virtual") ) {
            Tcl_WrongNumArgs( interp, 1, objv, "?-virtual? filename" );
            return TCL_ERROR;
        }

        CHECK_REQUEST_REC(private,"::rivet::include -virtual")
        filename = TclWeb_GetVirtualFile(private->req,Tcl_GetStringFromObj(objv[2], (int *)NULL) );
    }

    fd = Tcl_OpenFileChannel(interp, filename, "r", 0664);

    if (fd == NULL)
    {
        return TCL_ERROR;
    }
    Tcl_SetChannelOption(interp, fd, "-translation", "binary");

    outobj = Tcl_NewObj();
    Tcl_IncrRefCount(outobj);
    sz = Tcl_ReadChars(fd, outobj, -1, 0);
    if (sz == -1)
    {
        Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
        Tcl_DecrRefCount(outobj);
        return TCL_ERROR;
    }

    /* What we are doing is saving the translation and encoding
     * options, setting them both to binary, and the restoring the
     * previous settings. */
    Tcl_DStringInit(&transoptions);
    Tcl_DStringInit(&encoptions);
    tclstdout = Tcl_GetChannel(interp, "stdout", NULL);
    Tcl_GetChannelOption(interp, tclstdout, "-translation", &transoptions);
    Tcl_GetChannelOption(interp, tclstdout, "-encoding", &encoptions);
    Tcl_SetChannelOption(interp, tclstdout, "-translation", "binary");
    Tcl_WriteObj(tclstdout, outobj);
    Tcl_SetChannelOption(interp, tclstdout, "-translation", Tcl_DStringValue(&transoptions));
    Tcl_SetChannelOption(interp, tclstdout, "-encoding", Tcl_DStringValue(&encoptions));
    Tcl_DStringFree(&transoptions);
    Tcl_DStringFree(&encoptions);
    Tcl_DecrRefCount(outobj);
    return Tcl_Close(interp, fd);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Headers --
 *
 *      Command to manipulate HTTP headers from Tcl.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Headers )
{
    char *opt;
    rivet_thread_private*   private;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::headers")

    if (objc < 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    opt = Tcl_GetStringFromObj(objv[1], NULL);

    /* Basic introspection returning the value of the headers_printed flag */

    if (!strcmp("sent",opt))
    {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(private->req->headers_printed));
        return TCL_OK;
    }

    if (private->req->headers_printed != 0)
    {
        Tcl_AddObjErrorInfo(interp,"Cannot manipulate headers - already sent", -1);
        return TCL_ERROR;
    }

    if (!strcmp("redirect", opt)) /* ### redirect ### */
    {
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "new-url");
            return TCL_ERROR;
        }
        apr_table_set(private->r->headers_out, "Location",
                     Tcl_GetStringFromObj (objv[2], (int *)NULL));
        TclWeb_SetStatus(301, private->req);
        return TCL_OK;
    }
    else if (!strcmp("get", opt)) /* ### get ### */
    {
        const char* header_value;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername");
            return TCL_ERROR;
        }

        header_value = TclWeb_OutputHeaderGet(Tcl_GetString(objv[2]),private->req); 

        Tcl_SetObjResult(interp,Tcl_NewStringObj(header_value ? header_value : "",-1));
    }
    else if (!strcmp("set", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_HeaderSet(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), private->req);
    }
    else if (!strcmp("add", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_HeaderAdd(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), private->req);
    }
    else if (!strcmp("type", opt)) /* ### set ### */
    {
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "mime/type");
            return TCL_ERROR;
        }
        TclWeb_SetHeaderType(Tcl_GetString(objv[2]), private->req);
    }
    else if (!strcmp("numeric", opt)) /* ### numeric ### */
    {
        int st = 200;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "response_code_number");
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[2], &st) != TCL_ERROR) {
            TclWeb_SetStatus(st, private->req);
        } else {
            return TCL_ERROR;
        }

    } else {

        Tcl_Obj* result = Tcl_NewStringObj("unrecognized subcommand: ",-1);
        Tcl_IncrRefCount(result);
        Tcl_AppendStringsToObj(result,opt,NULL);

        Tcl_SetObjResult(interp, result);
        Tcl_DecrRefCount(result);
        return TCL_ERROR;

    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_LoadEnv --
 *
 *      Load the "environmental variables" - those variables that are
 *      set in the environment in a standard CGI program.  If no array
 *      name is supplied, they are loaded into an array whose name is
 *      the value of the ENV_ARRAY_NAME #define.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_LoadEnv )
{
    rivet_thread_private*   private;
    Tcl_Obj*                ArrayObj;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::load_env")
    if( objc > 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "?arrayName?" );
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        ArrayObj = objv[1];
    } else {
        ArrayObj = Tcl_NewStringObj( ENV_ARRAY_NAME, -1 );
    }

    return TclWeb_GetEnvVars(ArrayObj,private);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_LoadHeaders --
 *
 *      Load the HTTP headers supplied by the client into a Tcl array,
 *      whose name defaults to the value of the HEADERS_ARRAY_NAME
 *      #define.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Creates an array variable if none exists.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER ( Rivet_LoadHeaders )
{
    rivet_thread_private*   private;
    Tcl_Obj*                ArrayObj;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::load_headers")
    if( objc > 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "?arrayName?" );
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        ArrayObj = objv[1];
    } else {
        ArrayObj = Tcl_NewStringObj( HEADERS_ARRAY_NAME, -1 );
    }

    return TclWeb_GetHeaderVars(ArrayObj,private);
}

/* Tcl command to return a particular variable.  */

/* Use:
*/

/*
 *-----------------------------------------------------------------------------
 *
 *  Rivet_Var --
 *
 *      Returns information about GET or POST variables:
 *
 *      var get foo ?default?
 *      var list foo
 *      var names
 *      var number
 *      var all
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER ( Rivet_Var )
{
    rivet_thread_private*   private;
    const char*             cmd; 
    char*                   command;
    Tcl_Obj*                result = NULL;
    int                     source;
    register const char     *p;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::var,::rivet::var_post,::rivet::var_qs")
    if (objc < 2 || objc > 4)
    {
        Tcl_WrongNumArgs(interp, 1, objv,
                         "(get varname ?default?|list varname|exists varname|names"
                         "|number|all)");
        return TCL_ERROR;
    }
    cmd     = Tcl_GetString(objv[0]);
    command = Tcl_GetString(objv[1]);
    result  = Tcl_NewObj();

    /* determine if var_qs, var_post or var was called */

    /* first of all we have to skip the namespace string at the beginning of the command:
     * 
     * This fragment of code is taken from tcl 8.6.6 (tclNamesp.c) and it's part of the
     * function implementing Tcl "namespace tail", as such it should be authoritative
     * regarding the stripping of the namespace from a FQ command name
     */

    for (p = cmd;  *p != '\0';  p++) {
	    /* empty body */
    }
    
    while (--p > cmd) {
        if ((*p == ':') && (*(p-1) == ':')) {
            p++;			/* Just after the last "::" */
            break;
        }
    }
    cmd = p;

    if (!strcmp(cmd, "var_qs")) source = VAR_SRC_QUERYSTRING;
    else if (!strcmp(cmd, "var_post")) source = VAR_SRC_POST;
    else source = VAR_SRC_ALL;

    if (!strcmp(command, "get"))
    {
        char *key = NULL;
        char *deflt = NULL;
        if (objc != 3 && objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "variablename ?defaultval?");
            return TCL_ERROR;
        }
        key = Tcl_GetStringFromObj(objv[2], NULL);
        if (objc == 4)
        {
            deflt = Tcl_GetString(objv[3]);
        }

        if (TclWeb_GetVar(result, key, source, private->req) != TCL_OK)
        {
            if (deflt == NULL) {
                Tcl_SetStringObj(result, "", -1);
            } else {
                Tcl_SetStringObj(result, deflt, -1);
            }
        }
    } else if(!strcmp(command, "exists")) {
        char *key;
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "variablename");
            return TCL_ERROR;
        }
        key = Tcl_GetString(objv[2]);

        TclWeb_VarExists(result, key, source, private->req);
    } else if(!strcmp(command, "list")) {
        char *key;
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "variablename");
            return TCL_ERROR;
        }
        key = Tcl_GetStringFromObj(objv[2], NULL);

        if (TclWeb_GetVarAsList(result, key, source, private->req) != TCL_OK)
        {
            Tcl_SetStringObj(result,"",-1);
        }
    } else if(!strcmp(command, "names")) {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }

        if (TclWeb_GetVarNames(result, source, private->req) != TCL_OK)
        {
            Tcl_SetStringObj(result,"", -1);
        }
    } else if(!strcmp(command, "number")) {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }

        TclWeb_VarNumber(result, source, private->req);
    } else if(!strcmp(command, "all")) {

        if (objc < 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }

        if (TclWeb_GetAllVars(result, source, private->req) != TCL_OK)
        {
            Tcl_SetStringObj(result,"", -1);
        }
        else
        {
            if (objc >= 3)
            {
                /* We interpret the extra argument as a dictionary of default values */
                /* Since 'result' as created by TclWeb_GetAllVars is a flat list of key-value
                   pairs we assume it to be itself a dictionary */

                Tcl_DictSearch search;
                Tcl_Obj *key, *value;
                int done;
                Tcl_Obj *valuePtr = NULL;

                /* we loop through the dictionary using the code
                 * fragment shown in the manual page for Tcl_NewDictObj
                 */
                if (Tcl_DictObjFirst(interp,objv[2],&search,&key,&value,&done) != TCL_OK) {

                    /* If the object passed as optional argument is
                     * a valid dictionary we shouldn't get here */

                    Tcl_SetStringObj(result,"invalid_dictionary_value",-1);

                    /* We use the result Tcl_Obj to assign an error code. This should also release the object memory */

                    Tcl_SetObjErrorCode(interp,result);
                    Tcl_AddObjErrorInfo(interp,"Impossible to interpret the optional defaults argument as a dictionary value",-1);

                    return TCL_ERROR;
                }

                for (; !done ; Tcl_DictObjNext(&search, &key, &value, &done)) 
                {
                    if (Tcl_DictObjGet(interp,result,key,&valuePtr) == TCL_OK) 
                    {
                        if (valuePtr == NULL)
                        {
                            Tcl_DictObjPut(interp,result,key,value);
                        }
                    }
                }
                Tcl_DictObjDone(&search);
            }
        }

    } else {

        /* bad command  */
        Tcl_AppendResult(interp,"bad option: must be one of ",
                                "'get, list, names, number, all'", NULL);
        return TCL_ERROR;

    }
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/*
*/

static int append_key_callback (void *data, const char *key, const char *val)
{
    Tcl_Obj *list = data;

    Tcl_ListObjAppendElement (NULL, list, Tcl_NewStringObj (key, -1));
    return 1;
}

static int
append_key_value_callback (void *data, const char *key, const char *val)
{
    Tcl_Obj *list = data;

    Tcl_ListObjAppendElement (NULL, list, Tcl_NewStringObj (key, -1));
    Tcl_ListObjAppendElement (NULL, list, Tcl_NewStringObj (val, -1));
    return 1;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ApacheTable --
 *
 *      Deals with Rivet key-value tables in the request structure
 *
 *      apache_table get tablename key
 *      apache_table set tablename key value
 *      apache_table set tablename list
 *      apache_table exists tablename key
 *      apache_table unset tablename key
 *      apache_table names tablename
 *      apache_table array_get tablename
 *      apache_table clear tablename
 *
 *      Table names can be "notes", "headers_in", "headers_out",
 *      "err_headers_out", and "subprocess_env".
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_ApacheTable )
{
    apr_table_t *table = NULL;
    int subcommandindex;

    static CONST84 char *SubCommand[] = {
        "get",
        "set",
        "exists",
        "unset",
        "names",
        "array_get",
        "clear",
        NULL
    };

    enum subcommand {
        SUB_GET,
        SUB_SET,
        SUB_EXISTS,
        SUB_UNSET,
        SUB_NAMES,
        SUB_ARRAY_GET,
        SUB_CLEAR
    };

    static CONST84 char *tableNames[] = {
        "notes",
        "headers_in",
        "headers_out",
        "err_headers_out",
        "subprocess_env",
        NULL
    };

    int tableindex;

    enum tablename {
        TABLE_NOTES,
        TABLE_HEADERS_IN,
        TABLE_HEADERS_OUT,
        TABLE_ERR_HEADERS_OUT,
        TABLE_SUBPROCESS_ENV
    };

    rivet_thread_private*   private;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::apache_table")

    if ((objc < 3) || (objc > 5)) {
        Tcl_WrongNumArgs(interp, 1, objv, "option tablename ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], SubCommand,
                        "get|set|unset|list",
                        0, &subcommandindex) == TCL_ERROR) {
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj (interp, objv[2], tableNames,
                    "notes|headers_in|headers_out|err_header_out|subprocess_env",
                    0, &tableindex) == TCL_ERROR) {
	    return TCL_ERROR;
    }

    switch ((enum tablename)tableindex)
    {
        case TABLE_NOTES: {
            table = private->r->notes;
            break;
        }

        case TABLE_HEADERS_IN: {
            table = private->r->headers_in;
            break;
        }

        case TABLE_HEADERS_OUT: {
            table = private->r->headers_out;
            break;
        }

        case TABLE_ERR_HEADERS_OUT: {
            table = private->r->err_headers_out;
            break;
        }

        case TABLE_SUBPROCESS_ENV: {
            table = private->r->subprocess_env;
            break;
        }
    }

    switch ((enum subcommand)subcommandindex)
    {
        case SUB_GET: {
            const char *key;
            const char *value;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "tablename key");
                return TCL_ERROR;
            }
            key = Tcl_GetString (objv[3]);
            value = apr_table_get (table, key);

            if (value != NULL) {
                Tcl_SetObjResult (interp, Tcl_NewStringObj (value, -1));
            }
            break;
        }
        case SUB_EXISTS: {
            const char *key;
            const char *value;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "tablename key");
                return TCL_ERROR;
            }

            key = Tcl_GetString (objv[3]);
            value = apr_table_get (table, key);

            Tcl_SetObjResult (interp, Tcl_NewBooleanObj (value != NULL));
            break;
        }
        case SUB_SET: {
            int i;
            char *key;
            char *value;

            if (objc == 4) {
                int listObjc;
                Tcl_Obj **listObjv;

                if (Tcl_ListObjGetElements (interp, objv[3], &listObjc, &listObjv) == TCL_ERROR) {
                    return TCL_ERROR;
                }

                if (listObjc % 2 == 1) {
                    Tcl_SetObjResult (interp, Tcl_NewStringObj ("list must have even number of elements", -1));
                    return TCL_ERROR;
                }

                for (i = 0; i < listObjc; i += 2) {
                    apr_table_set (table, Tcl_GetString (listObjv[i]), Tcl_GetString (listObjv[i+1]));
                }

                break;
            }

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 2, objv, "tablename key value");
                return TCL_ERROR;
            }

            key = Tcl_GetString (objv[3]);
            value = Tcl_GetString (objv[4]);

            apr_table_set (table, key, value);
            break;
        }

        case SUB_UNSET: {
            char *key;

            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "tablename key");
                return TCL_ERROR;
            }

            key = Tcl_GetString (objv[3]);
            apr_table_unset (table, key);
            break;
        }

        case SUB_NAMES: {
            Tcl_Obj *list = Tcl_NewObj ();

            apr_table_do(append_key_callback, (void*)list, table, NULL);

            Tcl_SetObjResult (interp, list);
            break;
        }

        case SUB_ARRAY_GET: {
            Tcl_Obj *list = Tcl_NewObj ();

            apr_table_do(append_key_value_callback, (void*)list, table, NULL);

            Tcl_SetObjResult (interp, list);
            break;
        }

        case SUB_CLEAR: {
            apr_table_clear (table);
        }
    }

    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Upload --
 *
 *      Deals with file uploads (multipart/form-data):
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Has the potential to create files on the file system, or work
 *      with large amounts of data.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Upload )
{
    char*   varname = NULL;
    int     subcommandindex;

    /* ::rivet::upload subcommands must register
     *
     * - subcommand definition
     * - subcommand integer progressive index
     * - subcommand required (minimum) number of arguments
     *
     * +----------------------------------------+-------+
     * |         argv[1]    argv[2]   argv[3]   | argc  |
     * +----------------------------------------+-------+
     * |  upload channel   uploadname           |   3   |
     * |  upload save      uploadname filename  |   4   |
     * |  upload data      uploadname           |   3   |
     * |  upload exists    uploadname           |   3   |
     * |  upload size      uploadname           |   3   |
     * |  upload type      uploadname           |   3   |
     * |  upload filename  uploadname           |   3   |
     * |  upload tempname  uploadname           |   3   |
     * |  upload names                          |   2   |
     * +----------------------------------------+-------+
     *
     * a subcommand first optional argument must be the name
     * of an upload
     */

    static CONST84 char *SubCommand[] = {
        "channel",
        "save",
        "data",
        "exists",
        "size",
        "type",
        "filename",
        "tempname",
        "names",
        NULL
    };

    enum subcommand {
        CHANNEL,
        SAVE,
        DATA,
        EXISTS,
        SIZE,
        TYPE,
        FILENAME,
        TEMPNAME,
        NAMES
    };

    static CONST84 int cmds_objc[] = { 3,4,3,3,3,3,3,3,2 };
    int expected_objc;

    rivet_thread_private* private;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::upload")
    if (Tcl_GetIndexFromObj(interp, objv[1], SubCommand,
                        "channel|save|data|exists|size|type|filename|tempname|names",
                        0, &subcommandindex) == TCL_ERROR) {
        return TCL_ERROR;
    }

    expected_objc = cmds_objc[subcommandindex];

    if (objc != expected_objc) {
        Tcl_Obj* infoobj = Tcl_NewStringObj("Wrong argument numbers: ",-1);

        Tcl_IncrRefCount(infoobj);
        Tcl_AppendObjToObj(infoobj,Tcl_NewIntObj(expected_objc));
        Tcl_AppendStringsToObj(infoobj," arguments expected");
        Tcl_AppendObjToErrorInfo(interp, infoobj);
        Tcl_DecrRefCount(infoobj);

        if (subcommandindex == SAVE) {
            Tcl_WrongNumArgs(interp, 2, objv, "uploadname filename");
        } else {
            Tcl_WrongNumArgs(interp, objc, objv, "uploadname");
        }
        return TCL_ERROR;
    }

    /* We check whether an upload with a given name exists */

    if (objc >= 3) {
        int tcl_status;
        varname = Tcl_GetString(objv[2]);

        /* TclWeb_PrepareUpload calls ApacheUpload_find and returns
         * TCL_OK if the named upload exists in the current request */
        tcl_status = TclWeb_PrepareUpload(varname, private->req);

        if (subcommandindex == EXISTS) {
            Tcl_Obj* result = NULL;
            int upload_prepared = 0;

            if (tcl_status == TCL_OK) upload_prepared = 1;

            result = Tcl_NewObj();
            Tcl_SetIntObj(result,upload_prepared);
            Tcl_SetObjResult(interp, result);
            return TCL_OK;
                
        }

        if (tcl_status != TCL_OK)
        {
            Tcl_AddErrorInfo(interp, "Unable to find the upload named '");
            Tcl_AppendObjToErrorInfo(interp,Tcl_NewStringObj(varname,-1));
            Tcl_AppendObjToErrorInfo(interp,Tcl_NewStringObj("'",-1));
            return TCL_ERROR;
        }
    }

    /* CHANNEL  : get the upload channel name
     * SAVE     : save data to a specified filename
     * DATA     : get the uploaded data into a Tcl variable
     * SIZE     : uploaded data size
     * TYPE     : upload mimetype
     * FILENAME : upload original filename
     * TEMPNAME : temporary file where the upload is taking place
     * NAMES    : list of uploads
     *
     * the procedure shouldn't reach for the default case
     */

    switch ((enum subcommand)subcommandindex)
    {
        case CHANNEL:
            return TclWeb_UploadChannel(varname, private->req);
        case SAVE:
            return TclWeb_UploadSave(varname, objv[3], private->req);
        case DATA:
            return TclWeb_UploadData(varname, private->req);
        case SIZE:
            return TclWeb_UploadSize(private->req);
        case TYPE:
            return TclWeb_UploadType(private->req);
        case FILENAME:
            return TclWeb_UploadFilename(private->req);
        case TEMPNAME:
            return TclWeb_UploadTempname(private->req);
        case NAMES:
            return TclWeb_UploadNames(private->req);
        default:
            Tcl_WrongNumArgs(interp, 1, objv,"Rivet internal error: inconsistent argument");
    }
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_RawPost --
 *
 *      Returns the raw POST data.
 *
 * Results:
 *      The raw post data, or an empty string if there is none.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER ( Rivet_RawPost )
{
    char*                   data;
    Tcl_Obj*                retval;
    rivet_thread_private*   private;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::raw_post")

    data = TclWeb_GetRawPost(private->req);

    if (!data) {
        data = "";
    }
    retval = Tcl_NewStringObj(data, -1);
    Tcl_SetObjResult(interp, retval);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_NoBody --
 *
 *      Tcl command to erase body, so that only header is returned.
 *      Necessary for 304 responses.
 *
 * Results:
 *      A standard Tcl return value.
 *
 * Side Effects:
 *      Eliminates any body returned in the HTTP response.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_NoBody )
{
    rivet_thread_private*   private;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::no_body")

    if (private->req->content_sent == 1) {
        Tcl_AddErrorInfo(interp, "Content already sent");
        return TCL_ERROR;
    }

    private->req->content_sent = 1;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_AbortPageCmd --
 *
 *      Similar in purpose to PHP's "die" command, which halts all
 *      further output to the user.  Like an "exit" for web pages, but
 *      without actually exiting the apache child.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Flushes the standard (apache) output channel, and tells apache
 *      to stop sending data.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_AbortPageCmd )
{
    rivet_thread_private* private;
    static char *errorMessage = "Page generation terminated by abort_page directive";

    if (objc > 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    THREAD_PRIVATE_DATA(private)

    if (objc == 2)
    {
        char* cmd_arg = Tcl_GetStringFromObj(objv[1],NULL);
        
        if (strcmp(cmd_arg,"-aborting") == 0)
        {
            Tcl_SetObjResult (interp,Tcl_NewBooleanObj(private->page_aborting));
            return TCL_OK;
        }

        if (strcmp(cmd_arg,"-exiting") == 0)
        {
            Tcl_SetObjResult (interp,Tcl_NewBooleanObj(private->thread_exit));
            return TCL_OK;
        }
 
    /* 
     * we assume abort_code to be null, as abort_page shouldn't run twice while
     * processing the same request 
     */
       
        if (private->abort_code == NULL)
        {
            private->abort_code = objv[1];
            Tcl_IncrRefCount(private->abort_code);
        }
    }

    /* 
     * If page_aborting is true then this is the second call to abort_page
     * processing the same request: we ignore it and return a normal
     * completion code
     */

    if (private->page_aborting)
    {
        return TCL_OK;
    }

    /* this is the first (and supposedly unique) abort_page call during this request */

    /* we eleveta the page_aborting flag to the actual flag controlling the page abort execution. 
     * We still return the RIVET and ABORTPAGE_CODE, but internally
     * its page_aborting that will drive the code execution after abort_page
     */

    private->page_aborting = 1;

    Tcl_AddErrorInfo (interp, errorMessage);
    Tcl_SetErrorCode (interp, "RIVET", ABORTPAGE_CODE, errorMessage, (char *)NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 * Rivet_AbortCodeCmd -- 
 *
 * Returns the abort code stored internally by passing a user defined parameter 
 * to the command 'abort_page'.
 *
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_AbortCodeCmd )
{
    rivet_thread_private*   private;
    
    THREAD_PRIVATE_DATA(private)

    if (private->abort_code != NULL)
    {
        Tcl_SetObjResult(interp,private->abort_code);
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EnvCmd --
 *
 *      Loads a single environmental variable, to avoid the overhead
 *      of storing all of them when only one is needed.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_EnvCmd )
{
    char*                   key;
    char*                   val;
    rivet_thread_private*   private;
    
    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::env")

    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "variable" );
        return TCL_ERROR;
    }

    key = Tcl_GetStringFromObj (objv[1],NULL);

    val = TclWeb_GetEnvVar (private,key);

    Tcl_SetObjResult(interp, Tcl_NewStringObj( val, -1 ) );
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ExitCmd --
 *
 *      Calls the MPM specific exit procedure. For a threaded MPM (such
 *      as 'worker') the procedure should cause a thread to exit, not the
 *      whole process with all its threads. In this case the procedure
 *      returns an TCL_ERROR code that has to be handled in mod_rivet so that
 *      the error is ignored and the request procedure interrupted. 
 *      For a non threaded MPM (such as 'prefork') the single child process 
 *      exits thus reproducing an ordinary 'exit' command. 
 *
 * Result:
 *
 *      TCL_ERROR 
 * 
 * Side Effects:
 *
 *      - non threaded MPMs: the child process exits for good
 *      - threaded MPMs: the child process exits after all Tcl threads
 *      are told to exit
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_ExitCmd )
{
    int value;
    rivet_thread_private*   private;
    char* errorMessage = "page generation interrupted by exit command";

    if ((objc != 1) && (objc != 2)) {
        Tcl_WrongNumArgs(interp, 1, objv, "?returnCode?");
        return TCL_ERROR;
    }

    if (objc == 1) {
        value = 0;
    } else if (Tcl_GetIntFromObj(interp, objv[1], &value) != TCL_OK) {
        return TCL_ERROR;
    }

    THREAD_PRIVATE_DATA(private)

    private->page_aborting = 1;
    private->abort_code = Tcl_NewDictObj();

    /* The private->abort_code ref count is decremented before 
     * request processing terminates*/

    Tcl_IncrRefCount(private->abort_code);

    /*
     * mod_rivet traps every call to ::rivet::exit to offers a chance 
     * to the Tcl application interrupt execution much in 
     * the same way it can be done by calling ::rivet::abort_page
     */

    Tcl_DictObjPut(interp,private->abort_code,
                   Tcl_NewStringObj("error_code",-1),
                   Tcl_NewStringObj("exit",-1));

    Tcl_DictObjPut(interp,private->abort_code,
                   Tcl_NewStringObj("return_code",-1),
                   Tcl_NewIntObj(value));

    private->thread_exit = 1;
    private->exit_status = value;

    /* this call actually could never return for a non-threaded MPM bridge
     * as it eventually will call Tcl_Exit
     */
    Tcl_AddErrorInfo (interp, errorMessage);
    Tcl_SetErrorCode (interp, "RIVET", THREAD_EXIT_CODE, errorMessage, (char *)NULL);

    return TCL_ERROR;
}
/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_VirtualFilenameCmd --
 *
 *      Gets file according to its relationship with the request's
 *      root. (FIXME - check this).
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_VirtualFilenameCmd )
{
    rivet_thread_private*   private;
    char*                   filename;
    char*                   virtual;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::virtual_filename")
    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "filename" );
        return TCL_ERROR;
    }

    virtual  = Tcl_GetStringFromObj( objv[1], NULL );
    filename = TclWeb_GetVirtualFile( private->req, virtual );

    Tcl_SetObjResult(interp, Tcl_NewStringObj( filename, -1 ) );
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Inspect --
 *
 *      Rivet configuration introspection. Command '::rivet::inspect' 
 *      returns a dictionary of configuration data:
 *
 * Results:
 *      A dictionary or parameter value
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_InspectCmd )
{
    rivet_thread_private*   private;
    rivet_server_conf*      rsc; 
    int                     status = TCL_OK;

    THREAD_PRIVATE_DATA(private)

    if (objc == 1)
    {
        Tcl_Obj* dictObj;

        CHECK_REQUEST_REC(private,"::rivet::inspect")
        rsc = Rivet_GetConf(private->r); 
        dictObj = Rivet_BuildConfDictionary(interp,rsc);
        if (dictObj != NULL) {
            Tcl_IncrRefCount(dictObj);
            Tcl_SetObjResult(interp,dictObj);
            Tcl_DecrRefCount(dictObj);
        } else {
            status = TCL_ERROR;
        }

    }
    else if (objc == 2)
    {
        Tcl_Obj* par_name = objv[1];
        char*    cmd_arg;

        Tcl_IncrRefCount(par_name);
        cmd_arg = Tcl_GetStringFromObj(par_name,NULL);

        if (STRNEQU(cmd_arg,"script"))
        {
            Tcl_Obj* cmd;

            if (private != NULL)
            {
                if (private->r != NULL)
                {
                    Tcl_SetObjResult(interp,Tcl_NewStringObj(private->r->filename,-1));
                    return TCL_OK;
                }
            }

            cmd = Tcl_NewStringObj("info script",-1);

            Tcl_IncrRefCount(cmd); 
            status = Tcl_EvalObjEx(interp,cmd,TCL_EVAL_DIRECT);
            Tcl_DecrRefCount(cmd);

        } 
        else if (STRNEQU(cmd_arg,"-all"))
        {
            Tcl_Obj* dictObj;
            
            CHECK_REQUEST_REC(private,"::rivet::inspect -all")
            rsc = Rivet_GetConf(private->r); 
            dictObj = Rivet_CurrentConfDict(interp,rsc);
            Tcl_IncrRefCount(dictObj);
            Tcl_SetObjResult(interp,dictObj);            
            Tcl_DecrRefCount(dictObj);

        }
        else if (STRNEQU(cmd_arg,"server"))
        {
            Tcl_Obj*    dictObj;
            server_rec* srec;

            if (private == NULL) {
                srec = module_globals->server;
            } else {

                if (private->r == NULL) {
                    srec = module_globals->server; 
                } else {
                    srec = private->r->server;
                }

            }

            /* we read data from the server_rec */

            dictObj = Rivet_CurrentServerRec(interp,srec);
            Tcl_IncrRefCount(dictObj);
            Tcl_SetObjResult(interp,dictObj);            
            Tcl_DecrRefCount(dictObj);

        }
        else if (STRNEQU(cmd_arg,"exit"))
        {
            Tcl_Obj* exit_flag;
            
            CHECK_REQUEST_REC(private,"::rivet::inspect")
            /* thread exit status flag */
            exit_flag = Tcl_NewIntObj(private->thread_exit);

            Tcl_IncrRefCount(exit_flag);
            Tcl_SetObjResult(interp,exit_flag);
            Tcl_DecrRefCount(exit_flag);
        }
        else
        {
            Tcl_Obj* par_value = NULL;

            //CHECK_REQUEST_REC(private,"::rivet::inspect")
            if (private == NULL) {
                rsc = RIVET_SERVER_CONF(module_globals->server->module_config);
            } else {

                if (private->r == NULL) {
                    rsc = private->running_conf;
                } else {
                    rsc = Rivet_GetConf(private->r); 
                }

            }

            par_value = Rivet_ReadConfParameter(interp,rsc,par_name);
            if (par_value == NULL)
            {
                Tcl_Obj* errorinfo = Tcl_NewStringObj("mod_rivet internal error invalid argument: ",-1);

                Tcl_IncrRefCount(errorinfo);
                Tcl_AppendObjToObj(errorinfo,par_name);
                Tcl_AppendObjToErrorInfo(interp,errorinfo);
                Tcl_DecrRefCount(errorinfo);
                status = TCL_ERROR;
            }
            else
            {
                Tcl_IncrRefCount(par_value);
                Tcl_SetObjResult(interp,par_value);
                Tcl_DecrRefCount(par_value);
            }

        }

        Tcl_DecrRefCount(par_name);
    }
    else 
    {
        Tcl_WrongNumArgs( interp, 1, objv, "?server | dir | user? ?parameter name?" );
        status = TCL_ERROR;
    }
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_LogError --
 *
 *      Log an error from Rivet
 *
 *      log_error priority message
 *
 *        priority can be one of "emerg", "alert", "crit", "err",
 *            "warning", "notice", "info", "debug"
 *
 * Results:
 *      A message is logged to the Apache error log.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_LogErrorCmd )
{
    char *message = NULL;

    server_rec *serverRec;

    int loglevelindex;
    int  apLogLevel = 0;

    static CONST84 char *logLevel[] = {
        "emerg",
        "alert",
        "crit",
        "err",
        "warning",
        "notice",
        "info",
        "debug",
        NULL
    };

    enum loglevel {
        EMERG,
        ALERT,
        CRIT,
        ERR,
        WARNING,
        NOTICE,
        INFO,
        DEBUG
    };

    rivet_thread_private*   private;

    if( objc != 3 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "loglevel message" );
        return TCL_ERROR;
    }

    THREAD_PRIVATE_DATA(private)
    message = Tcl_GetString (objv[2]);
    if (Tcl_GetIndexFromObj(interp, objv[1], logLevel,
                        "emerg|alert|crit|err|warning|notice|info|debug",
                        0, &loglevelindex) == TCL_ERROR) {
        return TCL_ERROR;
    }

    switch ((enum loglevel)loglevelindex)
    {
      case EMERG:
        apLogLevel = APLOG_EMERG;
        break;

      case ALERT:
        apLogLevel = APLOG_ALERT;
        break;

      case CRIT:
        apLogLevel = APLOG_CRIT;
        break;

      case ERR:
        apLogLevel = APLOG_ERR;
        break;

      case WARNING:
        apLogLevel = APLOG_WARNING;
        break;

      case NOTICE:
        apLogLevel = APLOG_NOTICE;
        break;

      case INFO:
        apLogLevel = APLOG_INFO;
        break;

      case DEBUG:
        apLogLevel = APLOG_DEBUG;
        break;
    }

    /* if we are serving a page, we know our server, 
     * else send null for server
     */
    serverRec = ((private == NULL) || (private->r == NULL)) ? module_globals->server : private->r->server;

    ap_log_error (APLOG_MARK, apLogLevel, 0, serverRec, "%s", message);
    return TCL_OK;
}

#undef TESTPANIC
#ifdef TESTPANIC
/*
 *----------------------------------------------------------------------
 *
 * TestpanicCmd --
 *
 *      Calls the panic routine.
 *
 * Results:
 *      Always returns TCL_OK. 
 *
 * Side effects:
 *      May exit application.
 *
 *----------------------------------------------------------------------
 */

static int
TestpanicCmd(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    CONST char **argv;                  /* Argument strings. */
{
    CONST char *argString;

    /*
     *  Put the arguments into a var args structure
     *  Append all of the arguments together separated by spaces
     */

    argString = Tcl_Merge(argc-1, argv+1);
    panic("%s",argString);
    ckfree((char *)argString);

    return TCL_OK;
}
#endif /* TESTPANIC  */

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_UrlScript --
 *
 *      Builds the full URL referenced script composed by before_script,
 *      url referenced script and after_script. This command should not
 *      be called by ordinary application development
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side Effects:
 *      returns a Tcl script.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_UrlScript )
{
    rivet_thread_private* private;
    char*                cache_key;
    rivet_thread_interp* rivet_interp;
    Tcl_HashEntry*       entry  = NULL;
    Tcl_Obj*             script = NULL;
    int                  result;
    unsigned int         user_conf; 
    time_t               ctime;
    time_t               mtime;

    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::url_script")

    user_conf = IS_USER_CONF(private->running_conf);

    rivet_interp = RIVET_PEEK_INTERP(private,private->running_conf);
    ctime = private->r->finfo.ctime;
    mtime = private->r->finfo.mtime;
    cache_key = RivetCache_MakeKey(private->pool,private->r->filename,ctime,mtime,user_conf,1);

    entry = RivetCache_EntryLookup (rivet_interp,cache_key);
    if (entry == NULL)
    {
        Tcl_Interp*     interp;
        
        interp = rivet_interp->interp;

        script = Tcl_NewObj();
        Tcl_IncrRefCount(script);
        /*
         * We check whether we are dealing with a pure Tcl script or a Rivet template.
         * Actually this check is done only if we are processing a toplevel file, every nested
         * file (files included through the 'parse' command) is treated as a template.
         */

        if (Rivet_CheckType(private->r) == RIVET_TEMPLATE)
        {
            result = Rivet_GetRivetFile(private->r->filename,script,interp);

        } else {

            /* It's a plain Tcl file */
            result = Rivet_GetTclFile(private->r->filename, script, interp);

        }

        if (result == TCL_OK)
        {
            /* let's check the cache for free entries */

            if (rivet_interp->cache_free > 0)
            {
                int isNew;
                Tcl_HashEntry* entry;

                entry = RivetCache_CreateEntry (rivet_interp,cache_key,&isNew);
    
                /* Sanity check: we are here for this reason */

                ap_assert(isNew == 1);
            
                /* we proceed storing the script in the cache */

                RivetCache_StoreScript(rivet_interp,entry,script);
            }
            else if ((rivet_interp->flags & RIVET_CACHE_FULL) == 0)
            {
                rivet_interp->flags |= RIVET_CACHE_FULL;
                ap_log_error (APLOG_MARK, APLOG_NOTICE, APR_EGENERAL,private->r->server,"%s %s (%s),",
                                                                      "Rivet cache full when serving ",
                                                                      private->r->filename,
                                                                      private->r->server->server_hostname);
            }
        }
        Tcl_SetObjResult(rivet_interp->interp, script);
        Tcl_DecrRefCount(script);

    } else {
        Tcl_SetObjResult(rivet_interp->interp,RivetCache_FetchScript(entry));
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 * Rivet_GetThreadId --
 *
 * With a threaded bridge (worker and lazy) command [pid] on Unix systems return
 * the same process id when called from all threads running within that process.
 * For any debugging reason this command returns a unique thread identification
 * that can, for instance, be matched with the thread id (tid) information 
 * in the error log file.
 *
 * Arguments:
 *
 *      Output format: -hex (default) | -decimal
 *
 * Results:
 *
 *      Tcl object with a string representation of the thread id
 *
 * Side effects:
 *
 *      None
 *
 *-----------------------------------------------------------------------------
 */

#define SMALL_BUFFER_SIZE 32

TCL_CMD_HEADER( Rivet_GetThreadId )
{
    char                    buff[SMALL_BUFFER_SIZE];
    apr_os_thread_t         threadid;
    char*                   format_hex = "0x%8.8lx";
    char*                   format_dec = "%ld";
    char*                   output_format = format_hex;
    int                     wrong_args = false;
    //rivet_thread_interp*    interp_obj;

    // interp_obj = RIVET_PEEK_INTERP(private,private->running_conf);
    if (objc == 2)
    {
        Tcl_Obj* argobj = objv[1];
        char*    cmd_arg;

        Tcl_IncrRefCount(argobj);
        cmd_arg = Tcl_GetStringFromObj(argobj,NULL);
        if (STRNEQU(cmd_arg,"-hex"))
        {
            output_format = format_hex;
        }
        else if (STRNEQU(cmd_arg,"-decimal"))
        {
            output_format = format_dec;
        }
        else
        {
            wrong_args = true;
        }
        Tcl_DecrRefCount(argobj);

        if (wrong_args)
        {
            Tcl_AddObjErrorInfo(interp,"Wrong argument: it must be -decimal | -hex", -1);
            return TCL_ERROR;
        }
    } 
    else if (objc > 2)
    {
        Tcl_WrongNumArgs(interp,1,objv,"-decimal | -hex" );
        return TCL_ERROR;
    }

    /* Let's get the thread id and return it in the requested format */

    threadid = apr_os_thread_current();
    snprintf(buff,SMALL_BUFFER_SIZE,output_format,threadid);

    Tcl_SetObjResult(interp,Tcl_NewStringObj(buff,strlen(buff)));
    return TCL_OK;
}

#ifdef RIVET_DEBUG_BUILD
/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CacheContent --
 *
 *      Dumping in a list the cache content. For debugging purposes.
 *      This command will be placed within conditional compilation and
 *      documented within the 'Rivet Internals' section of the manual
 *
 * Results:
 *      
 *      a Tcl list of the keys in the interpreter cache
 *
 * Side Effects:
 *
 *      none
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_CacheContent )
{
    Tcl_Obj*                entry_list;
    rivet_thread_private*   private;
    rivet_thread_interp*    rivet_interp;
    int                     ep;
    THREAD_PRIVATE_DATA(private)
    CHECK_REQUEST_REC(private,"::rivet::cache_content")

    rivet_interp = RIVET_PEEK_INTERP(private,private->running_conf);
    interp = rivet_interp->interp;
    
    entry_list = Tcl_NewObj();
    Tcl_IncrRefCount(entry_list);

    ep = rivet_interp->cache_size - 1;
    
    while ((ep >= 0) && (rivet_interp->objCacheList[ep]))
    {
        int tcl_status;

        tcl_status = Tcl_ListObjAppendElement(interp,entry_list,Tcl_NewStringObj(rivet_interp->objCacheList[ep],-1));

        if (tcl_status != TCL_OK) {
            return tcl_status;
        }

        ep--;
    }
    Tcl_SetObjResult(interp,entry_list);
    Tcl_DecrRefCount(entry_list);
    return TCL_OK;
}

#endif /* RIVET_DEBUG_BUILD */

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitCore --
 *
 *      Creates the core rivet commands.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Creates new commands.
 *
 *-----------------------------------------------------------------------------
 */

DLLEXPORT int
Rivet_InitCore(rivet_thread_interp* interp_obj,rivet_thread_private* private)
{
    Tcl_Interp*         interp = interp_obj->interp;
    rivet_server_conf*  server_conf; 

    RIVET_OBJ_CMD ("makeurl",Rivet_MakeURL,private);
    RIVET_OBJ_CMD ("headers",Rivet_Headers,private);
    RIVET_OBJ_CMD ("load_env",Rivet_LoadEnv,private);
    RIVET_OBJ_CMD ("load_headers",Rivet_LoadHeaders,private);
    RIVET_OBJ_CMD ("var",Rivet_Var,private);
    RIVET_OBJ_CMD ("abort_page",Rivet_AbortPageCmd,private);
    RIVET_OBJ_CMD ("abort_code", Rivet_AbortCodeCmd,private);
    RIVET_OBJ_CMD ("virtual_filename",Rivet_VirtualFilenameCmd,private);
    RIVET_OBJ_CMD ("apache_table",Rivet_ApacheTable,private);
    RIVET_OBJ_CMD ("var_qs",Rivet_Var,private);
    RIVET_OBJ_CMD ("var_post",Rivet_Var,private);
    RIVET_OBJ_CMD ("raw_post",Rivet_RawPost,private);
    RIVET_OBJ_CMD ("upload",Rivet_Upload,private);
    RIVET_OBJ_CMD ("include",Rivet_Include,private);
    RIVET_OBJ_CMD ("parse",Rivet_Parse,private);
    RIVET_OBJ_CMD ("no_body",Rivet_NoBody,private);
    RIVET_OBJ_CMD ("env",Rivet_EnvCmd,private);
    RIVET_OBJ_CMD ("apache_log_error",Rivet_LogErrorCmd,private);
    RIVET_OBJ_CMD ("inspect",Rivet_InspectCmd,private);
    RIVET_OBJ_CMD ("exit",Rivet_ExitCmd,private);
    RIVET_OBJ_CMD ("url_script",Rivet_UrlScript,private);
    RIVET_OBJ_CMD ("thread_id",Rivet_GetThreadId,private);
    
#ifdef RIVET_DEBUG_BUILD
    /* code compiled conditionally for debugging */
    RIVET_OBJ_CMD ("cache_content",Rivet_CacheContent,private);
#endif
#ifdef TESTPANIC
    RIVET_OBJ_CMD ("testpanic",TestpanicCmd,private);
#endif

    /*
     * we don't need to check the virtual host server conf
     * stored in 'private' in order to determine if we are
     * export the command names, as this flag is meaningful
     * at the global level
     */
    server_conf = RIVET_SERVER_CONF(module_globals->server->module_config);

    if (server_conf->export_rivet_ns)
    {
        //rivet_interp_globals*   globals = NULL;
        Tcl_Namespace*          rivet_ns = interp_obj->rivet_ns;

        //globals = Tcl_GetAssocData(interp,"rivet", NULL);
        //rivet_ns = globals->rivet_ns;

        RIVET_EXPORT_CMD(interp,rivet_ns,"makeurl");
        RIVET_EXPORT_CMD(interp,rivet_ns,"headers");
        RIVET_EXPORT_CMD(interp,rivet_ns,"load_env");
        RIVET_EXPORT_CMD(interp,rivet_ns,"load_headers");
        RIVET_EXPORT_CMD(interp,rivet_ns,"var");
        RIVET_EXPORT_CMD(interp,rivet_ns,"abort_page");
        RIVET_EXPORT_CMD(interp,rivet_ns,"abort_code");
        RIVET_EXPORT_CMD(interp,rivet_ns,"virtual_filename");
        RIVET_EXPORT_CMD(interp,rivet_ns,"apache_table");
        RIVET_EXPORT_CMD(interp,rivet_ns,"var_qs");
        RIVET_EXPORT_CMD(interp,rivet_ns,"var_post");
        RIVET_EXPORT_CMD(interp,rivet_ns,"raw_post");
        RIVET_EXPORT_CMD(interp,rivet_ns,"upload");
        RIVET_EXPORT_CMD(interp,rivet_ns,"include");
        RIVET_EXPORT_CMD(interp,rivet_ns,"parse");
        RIVET_EXPORT_CMD(interp,rivet_ns,"no_body");
        RIVET_EXPORT_CMD(interp,rivet_ns,"env");
        RIVET_EXPORT_CMD(interp,rivet_ns,"apache_log_error");
        RIVET_EXPORT_CMD(interp,rivet_ns,"inspect");
        RIVET_EXPORT_CMD(interp,rivet_ns,"thread_id");
        // ::rivet::exit is not exported
    }

    return TCL_OK;
}
