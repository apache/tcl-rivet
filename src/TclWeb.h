#ifndef TCLWEB_H
#define TCLWEB_H
/*
 * TclWeb.c --
 * 	Common API layer.
 */

/* $Id$ */

#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"

typedef struct TclWebRequest {
    Tcl_Interp *interp;
    request_rec *req;
    ApacheRequest *apachereq;
    int headers_printed;
} TclWebRequest;

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_InitRequest --
 * Initializes the request structure.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_InitRequest(TclWebRequest *req, Tcl_Interp *interp, void *arg);


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_SendHeaders --
 * Sends HTTP headers.
 *
 * Results:
 * HTTP headers output.  Things like cookies may no longer be manipulated.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_SendHeaders(TclWebRequest *req);


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_HeaderSet --
 * Sets HTTP headers.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_HeaderSet(char *header, char *val, TclWebRequest *req);


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_SetStatus --
 * Sets status number (200, 404) for reply.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_SetStatus(int status, TclWebRequest *req);

int TclWeb_GetVar(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_VarExists(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_VarNumber(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetVarNames(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetAllVars(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetCookieVars(Tcl_Obj *cookievar, TclWebRequest *req);

int TclWeb_GetEnvVars(Tcl_Obj *envvar, TclWebRequest *req);

/* upload stuff goes here */

int TclWeb_Escape(char *out, char *in, int len, void *var);

int TclWeb_UnEscape(char *out, char *in, int len, void *var);

int TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req);

char *TclWeb_StringToUtf(char *in, TclWebRequest *req);

Tcl_Obj * TclWeb_StringToUtfToObj(char *in, TclWebRequest *req);

/* output/write/flush?  */

/* error (log) ? */

#endif /* TCLWEB_H */
