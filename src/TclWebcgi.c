/*
 * TclWeb.c --
 * 	Common API layer.
 *
 * These are the standalone (CGI) variants of the functions found in
 * TclWeb.h.  Low-level implementations are provided in this file.
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

#include <tcl.h>
#include "TclWeb.h"

typedef struct _TclWebRequest {
    Tcl_Interp *interp;
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

/* All these are still TODO. */

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

/* error (log) ? send to stderr with some information. */
