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
#include "apache_cookie.h"
#include "mod_rivet.h"
#include "rivet.h"
#include "TclWeb.h"

#define BUFSZ 4096

#define ENV_ARRAY_NAME "env"
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
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }
    Tcl_SetObjResult( interp, Tcl_NewStringObj(
	ap_construct_url(POOL, Tcl_GetString(objv[1]), globals->r),
	-1 ) );
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
    rivet_server_conf *rsc =
	RIVET_SERVER_CONF( globals->r->server->module_config );

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }

    filename = Tcl_GetStringFromObj (objv[1], (int *)NULL);
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
    if (Rivet_ParseExecFile(globals->r, rsc, filename, 0) == TCL_OK)
	return TCL_OK;
    else
	return TCL_ERROR;
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
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    rivet_server_conf *rsc =
	RIVET_SERVER_CONF( globals->r->server->module_config );
    Tcl_Obj *outobj;

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }

    fd = Tcl_OpenFileChannel(interp,
			     Tcl_GetStringFromObj(objv[1], (int *)NULL),
			     "r", 0664);

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
    Tcl_WriteObj(*(rsc->outchannel), outobj);
    Tcl_DecrRefCount(outobj);
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

    if (!strcmp("setcookie", opt)) /* ### setcookie ### */
    {
	int i;
	ApacheCookie *cookie;
	char *stringopts[12] = {NULL, NULL, NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL};

	if (objc < 4 || objc > 14)
	{
	    Tcl_WrongNumArgs(interp, 2, objv,
			     "-name cookie-name -value cookie-value "
			     "?-expires expires? ?-domain domain? "
			     "?-path path? ?-secure on/off?");
	    return TCL_ERROR;
	}

	/* SetCookie: foo=bar; EXPIRES=DD-Mon-YY HH:MM:SS;
	 * DOMAIN=domain; PATH=path; SECURE
	 */

	for (i = 0; i < objc - 2; i++)
	{
	    stringopts[i] = Tcl_GetString(objv[i + 2]);
	}
	cookie = ApacheCookie_new(globals->r,
				  stringopts[0], stringopts[1],
				  stringopts[2], stringopts[3],
				  stringopts[4], stringopts[5],
				  stringopts[6], stringopts[7],
				  stringopts[8], stringopts[9],
				  stringopts[10], stringopts[11],
				  NULL);
	ApacheCookie_bake(cookie);
    }
    else if (!strcmp("redirect", opt)) /* ### redirect ### */
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
	ap_table_set(globals->r->headers_out,
		     Tcl_GetStringFromObj (objv[2], (int *)NULL),
		     Tcl_GetStringFromObj (objv[3], (int *)NULL));
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

/* Tcl command to get and parse any CGI and environmental variables */

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

    Tcl_IncrRefCount( ArrayObj );

    return TclWeb_GetEnvVars(ArrayObj, globals->req);
}

static int
Rivet_LoadCookies(
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
	ArrayObj = Tcl_NewStringObj( COOKIES_ARRAY_NAME, -1 );
    }

    return TclWeb_GetCookieVars(ArrayObj, globals->req);
}

/* Tcl command to return a particular variable.  */

/* Use:
   var get foo
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

    if (objc < 2 || objc > 3)
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
	if (objc != 3)
	{
	    Tcl_WrongNumArgs(interp, 2, objv, "variablename");
	    return TCL_ERROR;
	}
	key = Tcl_GetStringFromObj(objv[2], NULL);

	if (TclWeb_GetVar(result, key, globals->req) != TCL_OK)
	{
	    result = Tcl_NewStringObj("", -1);
	    Tcl_IncrRefCount(result);
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
	    Tcl_IncrRefCount(result);
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
	    Tcl_IncrRefCount(result);
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
	    Tcl_IncrRefCount(result);
	}
    } else {
	/* bad command  */
	Tcl_AddErrorInfo(interp, "bad option: must be one of "
		"'get, list, names, number, all'");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, result);

    return TCL_OK;
}

/*
upload get XYZ
               channel        # returns channel
	       save (name)    # returns name?
	       data           # returns data

with the third one reporting an error if this hasn't been enabled, or
the first two if it has.

upload info XYZ

                exists
                size
                type
                filename

upload names

gets all the upload names.
*/

static int
Rivet_Upload(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    char *command = NULL;
    Tcl_Obj *result = NULL;
    ApacheUpload *upload;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    rivet_server_conf *rsc =
	RIVET_SERVER_CONF( globals->r->server->module_config );

    if (objc < 2 || objc > 5)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "get ...|info ...|names");
	return TCL_ERROR;
    }
    command = Tcl_GetString(objv[1]);

    result = Tcl_NewObj();
    if (!strcmp(command, "get"))
    {
	char *varname = NULL;
	if (objc < 4)
	{
	    Tcl_WrongNumArgs(interp, 2, objv,
		"varname channel|save filename|var varname");
	    return TCL_ERROR;
	}
	varname = Tcl_GetString(objv[2]);
	upload = ApacheUpload_find(globals->req->apachereq->upload, varname);
	if (upload != NULL) /* make sure we have an upload */
	{
	    Tcl_Channel chan;
	    char *method = Tcl_GetString(objv[3]);
	    if (!strcmp(method, "channel"))
	    {
		if (ApacheUpload_FILE(upload) != NULL)
		{
		    /* create and return a file channel */
		    char *channelname = NULL;
		    chan = Tcl_MakeFileChannel(
			(ClientData)fileno(ApacheUpload_FILE(upload)),
			TCL_READABLE);
		    Tcl_RegisterChannel(interp, chan);
		    channelname = Tcl_GetChannelName(chan);
		    Tcl_SetStringObj(result, channelname, -1);
		}
	    } else if (!strcmp(method, "save")) {
		/* save data to a specified filename  */

		int sz;
		char savebuffer[BUFSZ];
		Tcl_Channel savechan = NULL;
		Tcl_Channel chan = NULL;
		if (objc != 5)
		{
		    Tcl_WrongNumArgs(interp, 4, objv, "filename");
		    return TCL_ERROR;
		}

		savechan = Tcl_OpenFileChannel(interp, Tcl_GetString(objv[4]),
						"w", 0600);
		if (savechan == NULL)
		    return TCL_ERROR;
		else
		    Tcl_SetChannelOption(interp, savechan,
			"-translation", "binary");

		chan = Tcl_MakeFileChannel(
			(ClientData)fileno(ApacheUpload_FILE(upload)),
			TCL_READABLE);
		Tcl_SetChannelOption(interp, chan, "-translation", "binary");

		while ((sz = Tcl_Read(chan, savebuffer, BUFSZ)))
		{
		    if (sz == -1)
		    {
			Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
			return TCL_ERROR;
		    }

		    Tcl_Write(savechan, savebuffer, sz);
		    if (sz < 4096)
			break;
		}
		Tcl_Close(interp, savechan);
		Tcl_SetIntObj(result, 1);
	    } else if (!strcmp(method, "data")) {
		/* this sucks - we should use the hook, but I want to
                   get everything fixed and working first */
		if (rsc->upload_files_to_var)
		{
		    char *bytes = NULL;
		    Tcl_Channel chan = NULL;

		    bytes = Tcl_Alloc((unsigned)ApacheUpload_size(upload));
		    chan = Tcl_MakeFileChannel(
			(ClientData)fileno(ApacheUpload_FILE(upload)),
			TCL_READABLE);
		    Tcl_SetChannelOption(interp, chan,
			"-translation", "binary");
		    Tcl_SetChannelOption(interp, chan, "-encoding", "binary");
		    /* Put data in a variable  */
		    Tcl_ReadChars(chan, result, ApacheUpload_size(upload), 0);
		} else {
		    Tcl_AppendResult(interp,
			"Rivet_UploadFilesToVar is not set", NULL);
		    return TCL_ERROR;
		}
	    }
	} else {
	    /* no variable found  */
	    Tcl_SetStringObj(result, "", -1);
	}
    } else if (!strcmp(command, "info")) {
	char *varname = NULL;
	char *infotype = NULL;
	if (objc != 4)
	{
	    Tcl_WrongNumArgs(interp, 2, objv,
		"varname exists|size|type|filename");
	    return TCL_ERROR;
	}
	varname = Tcl_GetString(objv[2]);
	infotype = Tcl_GetString(objv[3]);

	upload = ApacheUpload_find(globals->req->apachereq->upload, varname);
	if (upload != NULL)
	{
	    if (!strcmp(infotype, "exists"))
	    {
		Tcl_SetIntObj(result, 1);
	    } else if (!strcmp(infotype, "size")) {
		Tcl_SetIntObj(result, ApacheUpload_size(upload));
	    } else if (!strcmp(infotype, "type")) {
		char *type = NULL;
		type = (char *)ApacheUpload_type(upload);
		if (type)
		    Tcl_SetStringObj(result, type, -1);
		else
		    Tcl_SetStringObj(result, "", -1);
	    } else if (!strcmp(infotype, "filename")) {
		Tcl_SetStringObj(result,
				 TclWeb_StringToUtf(upload->filename,
						    globals->req), -1);
	    } else {
		Tcl_AddErrorInfo(interp,"unknown upload info command, should "
			"be exists|size|type|filename");
		return TCL_ERROR;
	    }
	} else {
	    if (!strcmp(infotype, "exists")) {
		Tcl_SetIntObj(result, 0);
	    } else {
		Tcl_AddErrorInfo(interp, "variable doesn't exist");
		return TCL_ERROR;
	    }
	}
    } else if (!strcmp(command, "names")) {
	upload = ApacheRequest_upload(globals->req->apachereq);
	while (upload)
	{
	    Tcl_ListObjAppendElement(interp, result,
				     TclWeb_StringToUtfToObj(upload->name, globals->req));
	    upload = upload->next;
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "upload get|info|names");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}


/* Tcl command to get, and print some information about the current
   state of affairs */

static int
Rivet_Info(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    char *tble;
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    rivet_server_conf *rsc =
	RIVET_SERVER_CONF( globals->r->server->module_config );

    tble = ap_psprintf(POOL,
		       "<table border=0 bgcolor=green><tr><td>\n"
		       "<table border=0 bgcolor=\"#000000\">\n"
		       "<tr><td align=center bgcolor=blue><font color=\"#ffffff\" size=\"+2\">rivet_info</font><br></td></tr>\n"
		       "<tr><td><font color=\"#ffffff\">Free cache size: %d</font><br></td></tr>\n"
		       "<tr><td><font color=\"#ffffff\">PID: %d</font><br></td></tr>\n"
		       "</table>\n"
		       "</td></tr></table>\n", *(rsc->cache_free), getpid());
    Tcl_WriteObj(*(rsc->outchannel), Tcl_NewStringObj(tble, -1));
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
    rivet_server_conf *rsc =
	RIVET_SERVER_CONF( globals->r->server->module_config );

    if (objc != 1)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }

    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(*(rsc->outchannel));

    globals->r->connection->aborted = 1;

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
			"load_cookies",
			Rivet_LoadCookies,
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
			"rivet_info",
			Rivet_Info,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateObjCommand(interp,
			"no_body",
			Rivet_NoBody,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);

    TCL_OBJ_CMD( "abort_page", Rivet_AbortPageCmd );

    return TCL_OK;
}
