/*
 * rivetCore.c - Core commands which are compiled into mod_rivet itself.
 */

/* $Id$ */

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_main.h"
#include "util_script.h"
#include "http_conf_globals.h"

#include <tcl.h>
#include <string.h>

#include "apache_request.h"
#include "mod_rivet.h"
#include "rivet.h"
#include "TclWeb.h"

#define ENV_ARRAY_NAME "env"
#define HEADERS_ARRAY_NAME "headers"
#define COOKIES_ARRAY_NAME "cookies"

extern module rivet_module;

#define POOL (globals->r->pool)

/* Make a self-referencing URL  */
static int
Rivet_MakeURL(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
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

/* Include and parse a file */

static int
Rivet_Parse(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    char *filename;
    struct stat finfo;
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

    if (!strcmp(filename, globals->r->filename))
    {
	Tcl_AddErrorInfo(interp, "Cannot recursively call the same file!");
	return TCL_ERROR;
    }

    if (stat(filename, &finfo))
    {
	Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
	return TCL_ERROR;
    }
    if (Rivet_ParseExecFile(globals->req, filename, 0) == TCL_OK) {
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}

/* Tcl command to include flat files */

static int
Rivet_Include(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    int sz;
    Tcl_Channel fd;
    Tcl_Channel stdout;
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
    sz = Tcl_ReadChars(fd, outobj, -1, 0);
    if (sz == -1)
    {
	Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
	return TCL_ERROR;
    }

    /* What we are doing is saving the translation and encoding
     * options, setting them both to binary, and the restoring the
     * previous settings. */
    Tcl_DStringInit(&transoptions);
    Tcl_DStringInit(&encoptions);
    stdout = Tcl_GetChannel(interp, "stdout", NULL);
    Tcl_GetChannelOption(interp, stdout, "-translation", &transoptions);
    Tcl_GetChannelOption(interp, stdout, "-encoding", &encoptions);
    Tcl_SetChannelOption(interp, stdout, "-translation", "binary");
    Tcl_WriteObj(stdout, outobj);
    Tcl_SetChannelOption(interp, stdout, "-translation", Tcl_DStringValue(&transoptions));
    Tcl_SetChannelOption(interp, stdout, "-encoding", Tcl_DStringValue(&encoptions));
    Tcl_DStringFree(&transoptions);
    Tcl_DStringFree(&encoptions);
    return Tcl_Close(interp, fd);
}

/* Tcl command to manipulate headers */

static int
Rivet_Headers(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
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
	ap_table_set(globals->r->headers_out, "Location",
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
	TclWeb_SetHeaderType(Tcl_GetStringFromObj(objv[2], (int *)NULL), globals->req);
    } else if (!strcmp("numeric", opt)) /* ### numeric ### */
    {
	int st = 200;

	if (objc != 3)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, "response code");
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

/* Get the environmental variables, but do it from a tcl function, so
   we can decide whether we wish to or not */

static int
Rivet_LoadEnv(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
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

/* Get the headers set by the client. */

static int
Rivet_LoadHeaders(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
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
   var get foo ?default?
   var list foo
   var names
   var number
   var all
*/

static int
Rivet_Var(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    char *command;
    Tcl_Obj *result = NULL;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc < 2 || objc > 4)
    {
	Tcl_WrongNumArgs(interp, 1, objv,
			 "(get varname|list varname|exists varname|names"
			 "|number|all)");
	return TCL_ERROR;
    }
    command = Tcl_GetString(objv[1]);
    result = Tcl_NewObj();

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
	    deflt = Tcl_GetStringFromObj(objv[3], NULL);
	}

	if (TclWeb_GetVar(result, key, globals->req) != TCL_OK)
	{
	    if (deflt == NULL) {
		Tcl_AppendResult(interp, key, " does not exist", NULL);
		return TCL_ERROR;
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

	TclWeb_VarExists(result, key, globals->req);
    } else if(!strcmp(command, "list")) {
	char *key;
	if (objc != 3)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, "variablename");
	    return TCL_ERROR;
	}
	key = Tcl_GetStringFromObj(objv[2], NULL);

	if (TclWeb_GetVarAsList(result, key, globals->req) != TCL_OK)
	{
	    result = Tcl_NewStringObj("", -1);
	}
    } else if(!strcmp(command, "names")) {
	if (objc != 2)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}

	if (TclWeb_GetVarNames(result, globals->req) != TCL_OK)
	{
	    result = Tcl_NewStringObj("", -1);
	}
    } else if(!strcmp(command, "number")) {
	if (objc != 2)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}

	TclWeb_VarNumber(result, globals->req);
    } else if(!strcmp(command, "all")) {
	if (objc != 2)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	if (TclWeb_GetAllVars(result, globals->req) != TCL_OK)
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
  upload channel uploadname
  upload save name uploadname
  upload data uploadname
  upload exists uploadname
  upload size uploadname
  upload type uploadname
  upload filename uploadname
  upload names
*/

static int
Rivet_Upload(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    char *varname = NULL;
    char *command = NULL;

    int subcommandindex;

    Tcl_Obj *result = NULL;

    static char *SubCommand[] = {
	"channel",
	"save",
	"data",
	"exists",
	"size",
	"type",
	"filename",
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
	NAMES
    };

    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    command = Tcl_GetString(objv[1]);
    Tcl_GetIndexFromObj(interp, objv[1], SubCommand,
			"channel|save|data|exists|size|type|filename|names",
			0, &subcommandindex);

    /* If it's any of these, we need to find a specific name. */

    /* Excluded cases are EXISTS and NAMES. */
    if ((enum subcommand)subcommandindex == CHANNEL ||
	(enum subcommand)subcommandindex == SAVE ||
	(enum subcommand)subcommandindex == DATA ||
	(enum subcommand)subcommandindex == EXISTS ||
	(enum subcommand)subcommandindex == SIZE ||
	(enum subcommand)subcommandindex == TYPE ||
	(enum subcommand)subcommandindex == FILENAME)
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
	channelname = Tcl_GetChannelName(chan);
	Tcl_SetStringObj(result, channelname, -1);
	break;
    }
    case SAVE:
	/* save data to a specified filename  */
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "uploadname filename");
	    return TCL_ERROR;
	}

	if (TclWeb_UploadSave(varname, objv[4], globals->req) != TCL_OK)
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
			 "channel|save ?name?|data|exists|size|type|filename|names");
    }
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/* Tcl command to erase body, so that only header is returned.
   Necessary for 304 responses */

static int
Rivet_NoBody(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (globals->req->content_sent == 1)
	return TCL_ERROR;

    globals->req->content_sent = 1;
    return TCL_OK;
}

TCL_CMD_HEADER( Rivet_AbortPageCmd )
{
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc != 1)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }

    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(Tcl_GetChannel(interp, "stdout", NULL));
    TclWeb_StopSending(globals->req);
    return TCL_RETURN;
}

/* Get a single environment variable.  This saves us from having to load
 * the entire environment when all we need is one variable.
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

    TCL_OBJ_CMD( "abort_page", Rivet_AbortPageCmd );
    TCL_OBJ_CMD( "virtual_filename", Rivet_VirtualFilenameCmd );

    return TCL_OK;
}
