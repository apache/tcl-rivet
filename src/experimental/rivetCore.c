/*
 * rivetCore.c - Core commands which are compiled into mod_rivet itself.
 */

/* Copyright 2002-2004 The Apache Software Foundation

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

#include "apache_request.h"
#include "mod_rivet.h"
#include "rivet.h"
#include "TclWeb.h"

#define ENV_ARRAY_NAME     "::request::env"
#define HEADERS_ARRAY_NAME "::request::headers"
#define COOKIES_ARRAY_NAME "cookies"

extern module rivet_module;
extern char* TclWeb_GetRawPost (TclWebRequest *req);
extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;

#define POOL (globals->r->pool)

   

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
    Tcl_Obj*                result  = NULL;
    rivet_interp_globals*   globals = Tcl_GetAssocData(interp,"rivet",NULL);
    char*                   url_target_name;
    int                     target_length;

    CHECK_REQUEST_REC(globals->r,"::rivet::makeurl");

    if (objc > 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    if (objc == 1)
    {
        url_target_name = TclWeb_GetEnvVar (globals->req,"SCRIPT_NAME");
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
            char* script_name = TclWeb_GetEnvVar (globals->req,"SCRIPT_NAME");
            int   script_name_l = strlen(script_name);

            // regardless the reason for a SCRIPT_NAME being undefined we
            // prevent a segfault and we revert the behavior of makeurl
            // to the case of an absolute path

            if (script_name_l > 0)
            {
                // script name may have the form a directory path (and mod_rewrite 
                // could have mapped it to a .tcl or .rvt script)
                
                if (script_name[script_name_l-1] == '/')
                {
                    url_target_name = apr_pstrcat(globals->req->req->pool,script_name,url_target_name,NULL);
                }
                else
                {
                    url_target_name = apr_pstrcat(globals->req->req->pool,script_name,"/",url_target_name,NULL);
                }
            }
            else
            {
                url_target_name = apr_pstrcat(globals->req->req->pool,"/",url_target_name,NULL);
            }
        }
    }

    result = Tcl_NewObj();   
    TclWeb_MakeURL(result, url_target_name, globals->req);
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
    char            *filename;
    apr_status_t    stat_s;
    apr_finfo_t     finfo_b;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::parse");

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

            filename = TclWeb_GetVirtualFile(globals->req,Tcl_GetStringFromObj(objv[2],(int *)NULL));

        } else if ( STREQU( Tcl_GetStringFromObj(objv[1], (int *)NULL), "-string")) {

        /* we treat the argument as a string and we pass it as is to Rivet_ParseExecString */

            return Rivet_ParseExecString(globals->req, objv[2]);

        } else { 

            Tcl_WrongNumArgs( interp, 1, objv, "?-virtual? filename | -string template_string" );
            return TCL_ERROR;

        }

    }

    if (!strcmp(filename, globals->r->filename))
    {
        Tcl_AddErrorInfo(interp, "Cannot recursively call the same file!");
        return TCL_ERROR;
    }

    stat_s = apr_stat(&finfo_b,filename,APR_FINFO_NORM,globals->r->pool);
    if (stat_s != APR_SUCCESS)
    {
        char apr_error_message[256];

        Tcl_AddErrorInfo(interp,apr_strerror(stat_s,apr_error_message,256));
        return TCL_ERROR;
    }

    return Rivet_ParseExecFile(globals->private, filename, 0);
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
    int sz;
    Tcl_Channel fd;
    Tcl_Channel tclstdout;
    Tcl_Obj *outobj;
    char *filename;
    Tcl_DString transoptions;
    Tcl_DString encoptions;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if( objc < 2 || objc > 3 )
    {
        Tcl_WrongNumArgs(interp, 1, objv, "?-virtual? filename");
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        filename = Tcl_GetStringFromObj( objv[1], (int *)NULL );
    } else {
        if( !STREQU( Tcl_GetStringFromObj(objv[1], (int *)NULL), "-virtual") ) {
            Tcl_WrongNumArgs( interp, 1, objv, "?-virtual? filename" );
            return TCL_ERROR;
        }

        CHECK_REQUEST_REC(globals->r,"::rivet::include -virtual");
        filename = TclWeb_GetVirtualFile( globals->req,
                                          Tcl_GetStringFromObj(objv[2], (int *)NULL) );
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
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::headers");

    if (objc < 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    opt = Tcl_GetStringFromObj(objv[1], NULL);

    /* Basic introspection returning the value of the headers_printed flag */

    if (!strcmp("sent",opt))
    {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(globals->req->headers_printed));
        return TCL_OK;
    }

    if (globals->req->headers_printed != 0)
    {
        Tcl_AddObjErrorInfo(interp,
                            "Cannot manipulate headers - already sent", -1);
        return TCL_ERROR;
    }

    if (!strcmp("redirect", opt)) /* ### redirect ### */
    {
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "new-url");
            return TCL_ERROR;
        }
        apr_table_set(globals->r->headers_out, "Location",
                     Tcl_GetStringFromObj (objv[2], (int *)NULL));
        TclWeb_SetStatus(301, globals->req);
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

        header_value = TclWeb_OutputHeaderGet(Tcl_GetString(objv[2]),globals->req); 

        Tcl_SetObjResult(interp,Tcl_NewStringObj(header_value ? header_value : "",-1));
    }
    else if (!strcmp("set", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_HeaderSet(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), globals->req);
    }
    else if (!strcmp("add", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_HeaderAdd(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), globals->req);
    }
    else if (!strcmp("type", opt)) /* ### set ### */
    {
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "mime/type");
            return TCL_ERROR;
        }
        TclWeb_SetHeaderType(Tcl_GetString(objv[2]), globals->req);
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
            TclWeb_SetStatus(st, globals->req);
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
    Tcl_Obj *ArrayObj;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::load_env");
    if( objc > 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "?arrayName?" );
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        ArrayObj = objv[1];
    } else {
        ArrayObj = Tcl_NewStringObj( ENV_ARRAY_NAME, -1 );
    }

    return TclWeb_GetEnvVars(ArrayObj, globals->req);
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
    Tcl_Obj *ArrayObj;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::load_headers");
    if( objc > 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "?arrayName?" );
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        ArrayObj = objv[1];
    } else {
        ArrayObj = Tcl_NewStringObj( HEADERS_ARRAY_NAME, -1 );
    }

    return TclWeb_GetHeaderVars(ArrayObj, globals->req);
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
    char *cmd;
    char *command;
    Tcl_Obj *result = NULL;
    int source;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::var,var_post,var_qs");
    if (objc < 2 || objc > 4)
    {
        Tcl_WrongNumArgs(interp, 1, objv,
                         "(get varname ?default?|list varname|exists varname|names"
                         "|number|all)");
        return TCL_ERROR;
    }
    cmd = Tcl_GetString(objv[0]);
    command = Tcl_GetString(objv[1]);
    result = Tcl_NewObj();

    /* determine if var_qs, var_post or var was called */
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

        if (TclWeb_GetVar(result, key, source, globals->req) != TCL_OK)
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

        TclWeb_VarExists(result, key, source, globals->req);
    } else if(!strcmp(command, "list")) {
        char *key;
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "variablename");
            return TCL_ERROR;
        }
        key = Tcl_GetStringFromObj(objv[2], NULL);

        if (TclWeb_GetVarAsList(result, key, source, globals->req) != TCL_OK)
        {
            result = Tcl_NewStringObj("", -1);
        }
    } else if(!strcmp(command, "names")) {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }

        if (TclWeb_GetVarNames(result, source, globals->req) != TCL_OK)
        {
            result = Tcl_NewStringObj("", -1);
        }
    } else if(!strcmp(command, "number")) {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }

        TclWeb_VarNumber(result, source, globals->req);
    } else if(!strcmp(command, "all")) {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, NULL);
            return TCL_ERROR;
        }
        if (TclWeb_GetAllVars(result, source, globals->req) != TCL_OK)
        {
            result = Tcl_NewStringObj("", -1);
        }
    } else {
        /* bad command  */
        Tcl_AppendResult(interp, "bad option: must be one of ",
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

    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::apache_table");
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
            table = globals->r->notes;
            break;
        }

        case TABLE_HEADERS_IN: {
            table = globals->r->headers_in;
            break;
        }

        case TABLE_HEADERS_OUT: {
            table = globals->r->headers_out;
            break;
        }

        case TABLE_ERR_HEADERS_OUT: {
            table = globals->r->err_headers_out;
            break;
        }

        case TABLE_SUBPROCESS_ENV: {
            table = globals->r->subprocess_env;
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
 *      Deals with file uploads (multipart/form-data) like so:
 *
 *      upload channel uploadname
 *      upload save name uploadname
 *      upload data uploadname
 *      upload exists uploadname
 *      upload size uploadname
 *      upload type uploadname
 *      upload filename uploadname
 *      upload names
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
    char *varname = NULL;

    int subcommandindex;

    Tcl_Obj *result = NULL;

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

    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    CHECK_REQUEST_REC(globals->r,"::rivet::upload");
    if (Tcl_GetIndexFromObj(interp, objv[1], SubCommand,
                        "channel|save|data|exists|size|type|filename|names|tempname"
                        "|tempname|names",
                        0, &subcommandindex) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /* If it's any of these, we need to find a specific name. */

    /* Excluded case is NAMES. */

    if ((enum subcommand)subcommandindex == CHANNEL     ||
        (enum subcommand)subcommandindex == SAVE        ||
        (enum subcommand)subcommandindex == DATA        ||
        (enum subcommand)subcommandindex == EXISTS      ||
        (enum subcommand)subcommandindex == SIZE        ||
        (enum subcommand)subcommandindex == TYPE        ||
        (enum subcommand)subcommandindex == FILENAME    ||
        (enum subcommand)subcommandindex == TEMPNAME)
    {
        varname = Tcl_GetString(objv[2]);
        if ((enum subcommand)subcommandindex != EXISTS)
        {
            if (TclWeb_PrepareUpload(varname, globals->req) != TCL_OK)
            {
                Tcl_AddErrorInfo(interp, "Unable to find variable");
                return TCL_ERROR;
            }
        }

        /* If it's not the 'save' command, then it has to have an objc
           of 3. */
        if ((enum subcommand)subcommandindex != SAVE && objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "varname");
            return TCL_ERROR;
        }
    }

    result = Tcl_NewObj();

    switch ((enum subcommand)subcommandindex)
    {
        case CHANNEL: {
            Tcl_Channel chan;
            char *channelname = NULL;

            if (TclWeb_UploadChannel(varname, &chan, globals->req) != TCL_OK) {
                return TCL_ERROR;
            }
            channelname = (char *)Tcl_GetChannelName(chan);
            Tcl_SetStringObj(result, channelname, -1);
            break;
        }
        case SAVE:
            /* save data to a specified filename  */
            if (objc != 4) {
                Tcl_WrongNumArgs(interp, 2, objv, "uploadname filename");
                return TCL_ERROR;
            }

            if (TclWeb_UploadSave(varname, objv[3], globals->req) != TCL_OK)
            {
                return TCL_ERROR;
            }
            break;
        case DATA:
            if (TclWeb_UploadData(varname, result, globals->req) != TCL_OK) {
                return TCL_ERROR;
            }
            break;
        case EXISTS:
            if (TclWeb_PrepareUpload(varname, globals->req) != TCL_OK)
            {
                Tcl_SetIntObj(result, 0);
            } else {
                Tcl_SetIntObj(result, 1);
            }
            break;
        case SIZE:
            TclWeb_UploadSize(result, globals->req);
            break;
        case TYPE:
            TclWeb_UploadType(result, globals->req);
            break;
        case FILENAME:
            TclWeb_UploadFilename(result, globals->req);
            break;
        case TEMPNAME:
            TclWeb_UploadTempname(result,globals->req);
            break;
        case NAMES:
            if (objc != 2)
            {
                Tcl_WrongNumArgs(interp, 1, objv, "names");
                return TCL_ERROR;
            }
            TclWeb_UploadNames(result, globals->req);
            break;
        default:
            Tcl_WrongNumArgs(interp, 1, objv,
                             "channel|save ?name?|data|exists|size|type|filename|names|tempname");
    }
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
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
    char *data;
    Tcl_Obj *retval;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::raw_post");
    data = TclWeb_GetRawPost(globals->req);

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
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    CHECK_REQUEST_REC(globals->r,"::rivet::no_body");
    if (globals->req->content_sent == 1) {
        Tcl_AddErrorInfo(interp, "Content already sent");
        return TCL_ERROR;
    }

    globals->req->content_sent = 1;
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
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    static char *errorMessage = "Page generation terminated by abort_page directive";

    if (objc > 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    if (objc == 2)
    {
        char* cmd_arg = Tcl_GetStringFromObj(objv[1],NULL);
        
        if (strcmp(cmd_arg,"-aborting") == 0)
        {
            Tcl_SetObjResult (interp,Tcl_NewBooleanObj(globals->page_aborting));
            return TCL_OK;
        }
 
    /* 
     * we assume abort_code to be null, as abort_page shouldn't run twice while
     * processing the same request 
     */
       
        if (globals->abort_code == NULL)
        {
            globals->abort_code = objv[1];
            Tcl_IncrRefCount(globals->abort_code);
        }
    }

    /* 
     * If page_aborting is true then this is the second call to abort_page
     * processing the same request: we ignore it and return a normal
     * completion code
     */

    if (globals->page_aborting)
    {
        return TCL_OK;
    }

    /* this is the first (and supposedly unique) abort_page call during this request */

    globals->page_aborting = 1;

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
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    
    if (globals->abort_code != NULL)
    {
        Tcl_SetObjResult(interp,globals->abort_code);
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
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    char *key;
    char *val;

    CHECK_REQUEST_REC(globals->r,"::rivet::env");
    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "variable" );
        return TCL_ERROR;
    }

    key = Tcl_GetStringFromObj( objv[1], NULL );

    val = TclWeb_GetEnvVar( globals->req, key );

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
 *      TCL_ERROR when running a threaded MPM, it should never return
 *      if a non threaded MPM is run, because Tcl_Exit will be called
 *      by the MPM specific function of the corresponding bridge
 * 
 * Side Effects:
 *
 *      - non threaded MPMs: the child process exits for good
 *      - threaded MPMs: the logical variable controlling a bridge thread
 *      is set to zero and the request processing is interrupted
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_ExitCmd )
{
    rivet_interp_globals *globals = Tcl_GetAssocData(interp,"rivet",NULL);
    int                   value;

    CHECK_REQUEST_REC(globals->r,"::rivet::thread_exit");
    if ((objc != 1) && (objc != 2)) {
        Tcl_WrongNumArgs(interp, 1, objv, "?returnCode?");
        return TCL_ERROR;
    }

    if (objc == 1) {
        value = 0;
    } else if (Tcl_GetIntFromObj(interp, objv[1], &value) != TCL_OK) {
        return TCL_ERROR;
    }

    /* this call actually could never return for a non-threaded MPM bridge
     * as it eventually will call Tcl_Exit
     */

    return (*module_globals->mpm_exit_handler)(value);
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
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    char *filename;
    char *virtual;

    CHECK_REQUEST_REC(globals->r,"::rivet::virtual_filename");
    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "filename" );
        return TCL_ERROR;
    }

    virtual   = Tcl_GetStringFromObj( objv[1], NULL );
    filename  = TclWeb_GetVirtualFile( globals->req, virtual );

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
    rivet_interp_globals*   globals = Tcl_GetAssocData( interp, "rivet", NULL );
    rivet_server_conf*      rsc; 
    int                     status = TCL_OK;

    if (objc == 2)
    {
        Tcl_Obj* par_name = objv[1];

        if (STRNEQU(Tcl_GetStringFromObj(par_name,NULL),"script"))
        {
            if (globals->r == NULL)
            {
                Tcl_Obj* cmd = Tcl_NewStringObj("return [info script]",-1);

                Tcl_IncrRefCount(cmd); 
                status = Tcl_EvalObjEx(interp,cmd,TCL_EVAL_DIRECT);
                Tcl_DecrRefCount(cmd); 
            }            
            else
            {
                Tcl_SetObjResult(interp,Tcl_NewStringObj(globals->r->filename,-1));
            }
            return TCL_OK;
        }
    }

    if (objc == 1)
    {
        Tcl_Obj* dictObj;

        CHECK_REQUEST_REC(globals->r,"::rivet::inspect");
        rsc = Rivet_GetConf(globals->r); 
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
        char*    cmd_arg  = Tcl_GetStringFromObj(par_name,NULL);

        Tcl_IncrRefCount(par_name);
        if (STRNEQU(cmd_arg,"-all"))
        {
            Tcl_Obj* dictObj;
            
            CHECK_REQUEST_REC(globals->r,"::rivet::inspect -all");
            rsc = Rivet_GetConf(globals->r); 
            dictObj = Rivet_CurrentConfDict(interp,rsc);
            Tcl_IncrRefCount(dictObj);
            Tcl_SetObjResult(interp,dictObj);            
            Tcl_DecrRefCount(dictObj);

        }
        else if (STRNEQU(cmd_arg,"server"))
        {
            /* we read data from the server_rec */
            Tcl_Obj* dictObj;

            dictObj = Rivet_CurrentServerRec(interp,globals->srec);
            Tcl_IncrRefCount(dictObj);
            Tcl_SetObjResult(interp,dictObj);            
            Tcl_DecrRefCount(dictObj);

        }
        else if (STRNEQU(cmd_arg,"exit"))
        {
            /* thread exit status flag */
            Tcl_Obj* exit_flag = Tcl_NewIntObj(globals->private->thread_exit);

            Tcl_IncrRefCount(exit_flag);
            Tcl_SetObjResult(interp,exit_flag);
            Tcl_DecrRefCount(exit_flag);
        }
        else
        {
            Tcl_Obj* par_value = NULL;

            CHECK_REQUEST_REC(globals->r,"::rivet::inspect");
            rsc = Rivet_GetConf(globals->r); 
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

    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if( objc != 3 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "loglevel message" );
        return TCL_ERROR;
    }

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
    serverRec = (globals->r == NULL) ? NULL : globals->r->server;

    ap_log_error (APLOG_MARK, apLogLevel, 0, serverRec, "%s", message);
    return TCL_OK;
}

#define TESTPANIC 0

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

int
Rivet_InitCore( Tcl_Interp *interp )
{
#if RIVET_NAMESPACE_EXPORT == 1
    rivet_interp_globals *globals = NULL;
    Tcl_Namespace *rivet_ns;

    globals = Tcl_GetAssocData(interp, "rivet", NULL);
    rivet_ns = globals->rivet_ns;
#endif

    RIVET_OBJ_CMD ("makeurl",Rivet_MakeURL);
    RIVET_OBJ_CMD ("headers",Rivet_Headers);
    RIVET_OBJ_CMD ("load_env",Rivet_LoadEnv);
    RIVET_OBJ_CMD ("load_headers",Rivet_LoadHeaders);
    RIVET_OBJ_CMD ("var",Rivet_Var);
    RIVET_OBJ_CMD ("abort_page",Rivet_AbortPageCmd);
    RIVET_OBJ_CMD ("abort_code", Rivet_AbortCodeCmd);
    RIVET_OBJ_CMD ("virtual_filename",Rivet_VirtualFilenameCmd);
    RIVET_OBJ_CMD ("apache_table",Rivet_ApacheTable);
    RIVET_OBJ_CMD ("var_qs",Rivet_Var);
    RIVET_OBJ_CMD ("var_post",Rivet_Var);
    RIVET_OBJ_CMD ("raw_post",Rivet_RawPost);
    RIVET_OBJ_CMD ("upload",Rivet_Upload);
    RIVET_OBJ_CMD ("include",Rivet_Include);
    RIVET_OBJ_CMD ("parse",Rivet_Parse);
    RIVET_OBJ_CMD ("no_body",Rivet_NoBody);
    RIVET_OBJ_CMD ("env",Rivet_EnvCmd);
    RIVET_OBJ_CMD ("apache_log_error",Rivet_LogErrorCmd);
    RIVET_OBJ_CMD ("inspect",Rivet_InspectCmd);
    RIVET_OBJ_CMD ("exit_thread",Rivet_ExitCmd);

#ifdef TESTPANIC
    RIVET_OBJ_CMD ("testpanic",TestpanicCmd);
#endif

#if RIVET_NAMESPACE_EXPORT == 1
    Tcl_Export(interp,rivet_ns,"*",0);
#endif

//  return Tcl_PkgProvide( interp,RIVET_TCL_PACKAGE,"1.2");
    return TCL_OK;
}
