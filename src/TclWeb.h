#ifndef TCLWEB_H
#define TCLWEB_H
/*
 * TclWeb.c --
 * 	Common API layer.
 */

/* $Id$ */

/* Error wrappers  */
#define ER1 "<hr><p><code><pre>\n"
#define ER2 "</pre></code><hr>\n"

#define DEFAULT_HEADER_TYPE "text/html"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"

typedef struct TclWebRequest {
    Tcl_Interp *interp;
    request_rec *req;
    ApacheRequest *apachereq;
    ApacheUpload *upload;
    int headers_printed; 	/* has the header been printed yet? */
    int headers_set;       /* has the header been set yet? */
    int content_sent;
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

INLINE int TclWeb_StopSending(TclWebRequest *req);

int TclWeb_SetHeaderType(char *header, TclWebRequest *req);

int TclWeb_PrintHeaders(TclWebRequest *req);

int TclWeb_PrintError(char *errstr, int htmlflag, TclWebRequest *req);

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

int TclWeb_MakeURL(Tcl_Obj *result, char *filename, TclWebRequest *req);

int TclWeb_GetVar(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_VarExists(Tcl_Obj *result, char *varname, TclWebRequest *req);

int TclWeb_VarNumber(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetVarNames(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetAllVars(Tcl_Obj *result, TclWebRequest *req);

int TclWeb_GetCookieVars(Tcl_Obj *cookievar, TclWebRequest *req);

int TclWeb_GetEnvVars(Tcl_Obj *envvar, TclWebRequest *req);

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

char *TclWeb_GetEnvVar( TclWebRequest *req, char * );

/* output/write/flush?  */

/* error (log) ? */

#endif /* TCLWEB_H */
