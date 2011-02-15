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

/* $Id: rivetCore.c,v 1.45 2005/08/21 15:31:38 davidw Exp $ */

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
//#include "http_conf_globals.h"
#include "http_config.h"


#include <tcl.h>
#include <string.h>
#include <stdio.h>
#include <apr_errno.h>

#include "apache_request.h"
#include "mod_rivet.h"
#include "rivet.h"
#include "TclWeb.h"

#define ENV_ARRAY_NAME "env"
#define HEADERS_ARRAY_NAME "headers"
#define COOKIES_ARRAY_NAME "cookies"

extern module rivet_module;
extern char* TclWeb_GetRawPost (TclWebRequest *req);

#define POOL (globals->r->pool)

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_MakeURL --
 *
 * 	Make a self-referencing URL.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_MakeURL )
{
    Tcl_Obj *result = NULL;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }
    result = Tcl_NewObj();
    TclWeb_MakeURL(result, Tcl_GetString(objv[1]), globals->req);
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Parse --
 *
 * 	Include and parse a Rivet file.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side Effects:
 *	Whatever occurs in the Rivet page parsed.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Parse )
{
    char	    *filename;
    apr_status_t    stat_s;
    apr_finfo_t	    finfo_b;

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
        filename = TclWeb_GetVirtualFile(globals->req,Tcl_GetStringFromObj(objv[2],(int *)NULL));
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

    if (Rivet_ParseExecFile(globals->req, filename, 0) == TCL_OK) {
        return TCL_OK;
    } else {
        return TCL_ERROR;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Include --
 *
 * 	Includes a file literally in the output stream.  Useful for
 * 	images, plain HTML and the like.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Adds to the output stream.
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
 * 	Command to manipulate HTTP headers from Tcl.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Headers )
{
    char *opt;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc < 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
	return TCL_ERROR;
    }
    if (globals->req->headers_printed != 0)
    {
	Tcl_AddObjErrorInfo(interp,
			    "Cannot manipulate headers - already sent", -1);
	return TCL_ERROR;
    }
    opt = Tcl_GetStringFromObj(objv[1], NULL);

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
	return TCL_RETURN;
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
    } else if (!strcmp("numeric", opt)) /* ### numeric ### */
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
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_LoadEnv --
 *
 * 	Load the "environmental variables" - those variables that are
 * 	set in the environment in a standard CGI program.  If no array
 * 	name is supplied, they are loaded into an array whose name is
 * 	the value of the ENV_ARRAY_NAME #define.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_LoadEnv )
{
    Tcl_Obj *ArrayObj;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

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
 * 	Load the HTTP headers supplied by the client into a Tcl array,
 * 	whose name defaults to the value of the HEADERS_ARRAY_NAME
 * 	#define.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Creates an array variable if none exists.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER ( Rivet_LoadHeaders )
{
    Tcl_Obj *ArrayObj;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

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
 * 	Returns information about GET or POST variables:
 *
 * 	var get foo ?default?
 *	var list foo
 *	var names
 *	var number
 *	var all
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	None.
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
 * 	Deals with Rivet key-value tables in the request structure
 *
 *	apache_table get tablename key
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
 *	A standard Tcl result.
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
 * 	Deals with file uploads (multipart/form-data) like so:
 *
 *	upload channel uploadname
 *	upload save name uploadname
 *	upload data uploadname
 *	upload exists uploadname
 *	upload size uploadname
 *	upload type uploadname
 *	upload filename uploadname
 *	upload names
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Has the potential to create files on the file system, or work
 *	with large amounts of data.
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
    if (Tcl_GetIndexFromObj(interp, objv[1], SubCommand,
			"channel|save|data|exists|size|type|filename|names|tempname"
			"|tempname|names",
			0, &subcommandindex) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /* If it's any of these, we need to find a specific name. */

    /* Excluded case is NAMES. */

    if ((enum subcommand)subcommandindex == CHANNEL 	||
	(enum subcommand)subcommandindex == SAVE 	||
	(enum subcommand)subcommandindex == DATA 	||
	(enum subcommand)subcommandindex == EXISTS 	||
	(enum subcommand)subcommandindex == SIZE 	||
	(enum subcommand)subcommandindex == TYPE 	||
	(enum subcommand)subcommandindex == FILENAME 	||
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
 * 	Returns the raw POST data.
 *
 * Results:
 *	The raw post data, or an empty string if there is none.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER ( Rivet_RawPost )
{
    char *data;
    Tcl_Obj *retval;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

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
 * 	Tcl command to erase body, so that only header is returned.
 *	Necessary for 304 responses.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side Effects:
 *	Eliminates any body returned in the HTTP response.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_NoBody )
{
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

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
 * 	Similar in purpose to PHP's "die" command, which halts all
 * 	further output to the user.  Like an "exit" for web pages, but
 * 	without actually exiting the apache child.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Flushes the standard (apache) output channel, and tells apache
 *	to stop sending data.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_AbortPageCmd )
{
    static char *errorMessage = "Page generation terminated by abort_page directive";

    if (objc != 1)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }

    Tcl_AddErrorInfo (interp, errorMessage);
    Tcl_SetErrorCode (interp, "RIVET", "ABORTPAGE", errorMessage, (char *)NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EnvCmd --
 *
 * 	Loads a single environmental variable, to avoid the overhead
 * 	of storing all of them when only one is needed.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_EnvCmd )
{
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    char *key;
    char *val;

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
 * Rivet_VirtualFilenameCmd --
 *
 * 	Gets file according to its relationship with the request's
 * 	root. (FIXME - check this).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_VirtualFilenameCmd )
{
    rivet_interp_globals *globals = Tcl_GetAssocData( interp, "rivet", NULL );
    char *filename;
    char *virtual;

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
 * Rivet_LogError --
 *
 * 	Log an error from Rivet
 *
 *	log_error priority message
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
 *	Calls the panic routine.
 *
 * Results:
 *      Always returns TCL_OK. 
 *
 * Side effects:
 *	May exit application.
 *
 *----------------------------------------------------------------------
 */

static int
TestpanicCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    CONST char *argString;

    /*
     *  Put the arguments into a var args structure
     *  Append all of the arguments together separated by spaces
     */

    argString = Tcl_Merge(argc-1, argv+1);
    panic(argString);
    ckfree((char *)argString);

    return TCL_OK;
}
#endif /* TESTPANIC  */

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitCore --
 *
 * 	Creates the core rivet commands.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Creates new commands.
 *
 *-----------------------------------------------------------------------------
 */

int
Rivet_InitCore( Tcl_Interp *interp )
{
    Tcl_CreateObjCommand(interp,
			 "makeurl",
			 Rivet_MakeURL,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "headers",
			 Rivet_Headers,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "load_env",
			 Rivet_LoadEnv,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "load_headers",
			 Rivet_LoadHeaders,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "var",
			 Rivet_Var,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "apache_table",
			 Rivet_ApacheTable,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "var_qs",
			 Rivet_Var,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "var_post",
			 Rivet_Var,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "raw_post",
			 Rivet_RawPost,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "upload",
			 Rivet_Upload,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "include",
			 Rivet_Include,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "parse",
			 Rivet_Parse,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "no_body",
			 Rivet_NoBody,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "env",
			 Rivet_EnvCmd,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "apache_log_error",
			 Rivet_LogErrorCmd,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

#ifdef TESTPANIC
    Tcl_CreateCommand(interp, "testpanic", TestpanicCmd, (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
#endif

    TCL_OBJ_CMD( "abort_page", Rivet_AbortPageCmd );
    TCL_OBJ_CMD( "virtual_filename", Rivet_VirtualFilenameCmd );

    return TCL_OK;
}
