/*
 * TclWeb.c --
 * 	Common API layer.
 */


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_InitRequest --
 * Initializes the request structure.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_InitRequest(TclWebRequest *req, void *arg);


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


/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_Cookie --
 * Make cookie.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_Cookie(Tcl_Obj *list, TclWebRequest *req);

int TclWeb_GetCookie(Tcl_Obj *list, TclWebRequest *req);

int TclWeb_GetCGIVars(Tcl_Obj *list, TclWebRequest *req);

int TclWeb_GetEnvVars(Tcl_HashTable *envs, TclWebRequest *req);

/* upload stuff goes here */

int TclWeb_Escape(char *out, char *in, int len, void *var);

int TclWeb_UnEscape(char *out, char *in, int len, void *var);

int TclWeb_Base64Encode(char *out, char *in, int len, TclWebRequest *req);

int TclWeb_Base64Decode(char *out, char *in, int len, TclWebRequest *req);

int TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req);

/* output/write/flush?  */

/* error (log) ? */
