#ifndef TCLWEB_H
#define TCLWEB_H

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
 * It opens a new channel and sets its translation and encoding as binary
 * The channel name is retuned as result in the interpreter pointed by req->interp
 *
 * Results:
 *
 * Makes the channel name available to the script level
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadChannel(char *varname, TclWebRequest *req);

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
 * Returns the uploaded data to the Tcl script level.
 * 
 * If the config parameter upload_files_to_var is not set the procedure
 * returs an error
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadData(char *varname, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadSize --
 *
 * Returns the size of the data uploaded.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadSize(TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadType --
 *
 * Returns the mime type of the file uploaded.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadType(TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadFilename --
 *
 * Returns the original filename of the uploaded data, on the client side.
 *
 * Results:
 *
 * Returns the filename to the script level
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadFilename(TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_UploadTempname --
 *
 * Returns the name of the temp file the uploaded data was stored in.
 *
 * Results:
 *
 * the 'tempname' is returned to the script level
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_UploadTempname(TclWebRequest *req);

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

int TclWeb_UploadNames(TclWebRequest *req);

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
