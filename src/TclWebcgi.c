/*
 * TclWeb.c --
 * 	Common API layer.
 *
 * These are the standalone (CGI) variants of the functions found in
 * TclWeb.h.  Low-level implementations are provided in this file.
 */

/* $Id$ */

#include <tcl.h>
#include "TclWeb.h"

typedef struct _TclWebRequest {
    int header_sent;
    Tcl_HashTable *headers;
    int status;
} TclWebRequest;

int
TclWeb_InitRequest(TclWebRequest *req, void *arg)
{
    req = Tcl_Alloc(sizeof(TclWebRequest));
    req->headers = Tcl_Alloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(req->headers, TCL_STRING_KEYS);
    return TCL_OK;
}

int
TclWeb_SendHeaders(TclWebRequest *req)
{
    return TCL_OK;
}

int
TclWeb_HeaderSet(char *header, char *val, TclWebRequest *req);

int
TclWeb_SetStatus(int status, TclWebRequest *req);

int
TclWeb_GetCGIVars(Tcl_Obj *list, TclWebRequest *req);

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
