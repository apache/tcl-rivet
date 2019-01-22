/* -- TclWeb.h: Common API layer

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/


#ifndef __tclweb_h__
#define __tclweb_h__

#include <tcl.h>
#include "apache_request.h"

/*
 * TclWeb.c --
 * 	Common API layer.
 */

/* $Id$ */

/* Tcl 8.4 migration. */
#ifndef CONST84
#   define CONST84
#endif

/* Error wrappers  */
#define ER1 "<hr><p><code><pre>\n"
#define ER2 "</pre></code><hr>\n"

#define DEFAULT_HEADER_TYPE "text/html"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"

/* Creates a TclWebRequest object */

TclWebRequest* TclWeb_NewRequestObject (apr_pool_t *p);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_InitRequest --
 * Initializes the request structure.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_InitRequest(rivet_thread_private* private, Tcl_Interp *interp);

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

INLINE int TclWeb_StopSending(TclWebRequest *req);

int TclWeb_SetHeaderType(char *header, TclWebRequest *req);

int TclWeb_PrintHeaders(TclWebRequest *req);

int TclWeb_PrintError(CONST84 char *errstr, int htmlflag, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_HeaderSet --
 * Sets HTTP headers.
 *
 *-----------------------------------------------------------------------------
 */

int         TclWeb_HeaderSet(char *header, char *val, TclWebRequest *req);
void        TclWeb_OutputHeaderSet(char *header, char *val, TclWebRequest *req);
const char* TclWeb_OutputHeaderGet(char *header, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_HeaderSet --
 * Adds an HTTP headers.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_HeaderAdd(char *header, char *val, TclWebRequest *req);


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_SetStatus --
 * Sets status number (200, 404) for reply.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_SetStatus(int status, TclWebRequest *req);

int TclWeb_MakeURL(Tcl_Obj *result, char *filename, TclWebRequest *req);

int TclWeb_GetVar(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);

int TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);

int TclWeb_VarExists(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);

int TclWeb_VarNumber(Tcl_Obj *result, int source, TclWebRequest *req);

int TclWeb_GetVarNames(Tcl_Obj *result, int source, TclWebRequest *req);

int TclWeb_GetAllVars(Tcl_Obj *result, int source, TclWebRequest *req);

int TclWeb_GetEnvVars(Tcl_Obj *envvar, rivet_thread_private *p);

int TclWeb_GetHeaderVars(Tcl_Obj *headersvar, rivet_thread_private *p);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_PrepareUpload --
 *
 * Do any preperation necessary for file uploads.  This must be
 * performed before other upload operations.
 *
 * Results:
 *
 * Stores, if necessary, additional, initialized information in the
 * TclWebRequest structure.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_PrepareUpload(char *varname, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadChannel --
 *
 * Takes the address of a Tcl_Channel and uses it to create a channel
 * pointing to the upload data.
 *
 * Results:
 *
 * 'chan' points to a FileChannel containing the uploaded data.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadChannel(char *varname, Tcl_Channel *chan, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadSave --
 *
 * Saves the uploaded file in 'filename'.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadSave(char *varname, Tcl_Obj *filename, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadData --
 *
 * Fills in the 'data' Tcl_Obj with the uploaded data.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadData(char *varname, Tcl_Obj *data, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadSize --
 *
 * Stores, in 'sz' the size of the data uploaded.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadSize(Tcl_Obj *sz, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadType --
 *
 * Stores, in 'type' the mime type of the file uploaded.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadType(Tcl_Obj *type, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadFilename --
 *
 * Get the original filename of the uploaded data, on the client side.
 *
 * Results:
 *
 * Stores the filename in 'filename'.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadFilename(Tcl_Obj *filename, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadTempname --
 *
 * Get the name of the temp file the uploaded data was stored in.
 *
 * Results:
 *
 * Stores the temp name in 'tempname'.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadTempname(Tcl_Obj *tempname, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadNames --
 *
 * Fetch names of all the uploaded variables.
 *
 * Results:
 *
 * Stores the names of the variables in the list 'names'.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadNames(Tcl_Obj *names, TclWebRequest *req);

int TclWeb_Escape(char *out, char *in, int len, void *var);

int TclWeb_UnEscape(char *out, char *in, int len, void *var);

int TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req);

char *TclWeb_StringToUtf(char *in, TclWebRequest *req);

Tcl_Obj * TclWeb_StringToUtfToObj(char *in, TclWebRequest *req);

char *TclWeb_GetEnvVar(rivet_thread_private *,char *);

char *TclWeb_GetVirtualFile( TclWebRequest *req, char *virtualname );

/* output/write/flush?  */

/* error (log) ? */

#endif /* TCLWEB_H */
