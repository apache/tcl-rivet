/*
 * TclWeb.c --
 * 	Common API layer.
 */

/* $Id$ */

#include <tcl.h>
#include "TclWeb.h"



int
TclWeb_SendHeaders(TclWebRequest *req)
{
#ifdef APACHE_MODULE
    ap_send_header(req->req);
#else

#endif /* APACHE_MODULE */
    return TCL_OK;
}

int
TclWeb_HeaderSet(char *header, char *val, void *arg);

int
TclWeb_SetStatus(int status, void *arg);

int
TclWeb_GetCGIVars(Tcl_Obj *list, void *arg);

int
TclWeb_GetEnvVars(Tcl_Obj *list, void *arg);

int
TclWeb_Base64Encode(char *out, char *in, int len, void *arg);

int
TclWeb_Base64Decode(char *out, char *in, int len, void *arg);

int
TclWeb_EscapeShellCommand(char *out, char *in, void *arg);

/* output/write/flush?  */

/* error (log) ? */
