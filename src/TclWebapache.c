/*
 * TclWeb.c --
 * 	Common API layer.
 *
 * This file contains the Apache-based versions of the functions in
 * TclWeb.h.  They rely on Apache and apreq to perform the underlying
 * operations.
 */

/* $Id$ */

#include <tcl.h>
#include "TclWeb.h"

typedef struct _TclWebRequest {
    request_rec *req;
    ApacheRequest *apachereq;
} TclWebRequest;

int
TclWeb_InitRequest(TclWebRequest *req, void *arg)
{
    req = Tcl_Alloc(sizeof(TclWebRequest));
    req->req = (request_rec *)arg;
    req->apacherequest = ApacheRequest_new(r);
    return TCL_OK;
}

int
TclWeb_SendHeaders(TclWebRequest *req)
{
    ap_send_header(req->req);
    return TCL_OK;
}

int
TclWeb_HeaderSet(char *header, char *val, TclWebRequest *req)
{
    ap_table_set(req->req->headers_out, header, val);
    return TCL_OK;
}

int
TclWeb_SetStatus(int status, TclWebRequest *req)
{
    req->req->status = status;
    return TCL_OK;
}

int
TclWeb_GetCGIVars(Tcl_Obj *list, TclWebRequest *req)
{

}

int
TclWeb_GetEnvVars(Tcl_Obj *list, TclWebRequest *req);

int
TclWeb_Base64Encode(char *out, char *in, int len, TclWebRequest *req);

int
TclWeb_Base64Decode(char *out, char *in, int len, TclWebRequest *req);

int
TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req);

/* output/write/flush?  */

/* error (log) ? */
