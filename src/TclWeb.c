/*
 * TclWeb.c --
 * 	Common API layer.
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
