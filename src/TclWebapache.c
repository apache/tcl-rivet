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
    Tcl_Interp *interp;
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
TclWeb_GetEnvVars(Tcl_HashTable *envs, TclWebRequest *req)
{
    char *timefmt = DEFAULT_TIME_FORMAT;
#ifndef WIN32
    struct passwd *pw;
#endif /* ndef WIN32 */
    char *t;
    char *authorization = NULL;

    time_t date;

    int i;

    array_header *hdrs_arr;
    table_entry *hdrs;
    array_header *env_arr;
    table_entry  *env;
    Tcl_HashEntry *entry;

    date = req->req->request_time;
    /* ensure that the system area which holds the cgi variables is empty */
    ap_clear_table(req->req->subprocess_env);

    /* retrieve cgi variables */
    ap_add_cgi_vars(req->req);
    ap_add_common_vars(req->req);

    hdrs_arr = ap_table_elts(req->req->headers_in);
    hdrs = (table_entry *) hdrs_arr->elts;

    env_arr =  ap_table_elts(req->req->subprocess_env);
    env     = (table_entry *) env_arr->elts;

    if (envs == NULL)
    {
	Tcl_InitHashTable(envs, TCL_STRING_KEYS);
    }


    /* Get the user/pass info for Basic authentication */
    (const char*)authorization =
	ap_table_get(req->req->headers_in, "Authorization");
    if (authorization
	&& !strcasecmp(ap_getword_nc(POOL, &authorization, ' '), "Basic"))
    {
	char *tmp;
	char *user;
	char *pass;

	tmp = ap_pbase64decode(POOL, authorization);
	user = ap_getword_nulls_nc(POOL, &tmp, ':');
	pass = tmp;
 	Tcl_ObjSetVar2(interp, Tcl_NewStringObj("::request::USER", -1),
		       Tcl_NewStringObj("user", -1),
		       STRING_TO_UTF_TO_OBJ(user, POOL),
		       0);
 	Tcl_ObjSetVar2(interp, Tcl_NewStringObj("::request::USER", -1),
		       Tcl_NewStringObj("pass", -1),
		       STRING_TO_UTF_TO_OBJ(pass, POOL),
		       0);
    }

    /* These were the "include vars"  */
    Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DATE_LOCAL", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(POOL,
					date, timefmt, 0), POOL), 0);
    Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DATE_GMT", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(POOL,
					date, timefmt, 1), POOL), 0);
    Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("LAST_MODIFIED", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(POOL,
					req->req->finfo.st_mtime,
					timefmt, 0), POOL), 0);
    Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DOCUMENT_URI", -1),
		   STRING_TO_UTF_TO_OBJ(req->req->uri, POOL), 0);
    Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DOCUMENT_PATH_INFO", -1),
		   STRING_TO_UTF_TO_OBJ(req->req->path_info, POOL), 0);

#ifndef WIN32
    pw = getpwuid(req->req->finfo.st_uid);
    if (pw)
	Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("USER_NAME", -1),
	       STRING_TO_UTF_TO_OBJ(ap_pstrdup(POOL, pw->pw_name), POOL), 0);
    else
	Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("USER_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(
			   ap_psprintf(POOL, "user#%lu",
			   (unsigned long)req->req->finfo.st_uid), POOL), 0);
#endif

    if ((t = strrchr(req->req->filename, '/')))
	Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(++t, POOL), 0);
    else
	Tcl_ObjSetVar2(interp, ArrayObj, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(req->req->uri, POOL), 0);

    if (req->req->args)
    {
	char *arg_copy = ap_pstrdup(POOL, req->req->args);
	ap_unescape_url(arg_copy);
	Tcl_ObjSetVar2(interp, ArrayObj,
	   Tcl_NewStringObj("QUERY_STRING_UNESCAPED", -1),
	   STRING_TO_UTF_TO_OBJ(ap_escape_shell_cmd(POOL, arg_copy), POOL), 0);
    }

    /* ----------------------------  */

    /* transfer client request headers to TCL request namespace */
    for (i = 0; i < hdrs_arr->nelts; ++i)
    {
	if (!hdrs[i].key)
	    continue;
	else {
	    Tcl_ObjSetVar2(interp, ArrayObj,
			   STRING_TO_UTF_TO_OBJ(hdrs[i].key, POOL),
			   STRING_TO_UTF_TO_OBJ(hdrs[i].val, POOL), 0);
	}
    }

    /* transfer apache internal cgi variables to TCL request namespace */
    for (i = 0; i < env_arr->nelts; ++i)
    {
	if (!env[i].key)
	    continue;
	Tcl_ObjSetVar2(interp, ArrayObj, STRING_TO_UTF_TO_OBJ(env[i].key, POOL),
		       STRING_TO_UTF_TO_OBJ(env[i].val, POOL), 0);
    }

    /* cleanup system cgi variables */
    ap_clear_table(req->req->subprocess_env);

    return TCL_OK;
}

int
TclWeb_Base64Encode(char *out, char *in, int len, TclWebRequest *req);

int
TclWeb_Base64Decode(char *out, char *in, int len, TclWebRequest *req);

int
TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req);

/* output/write/flush?  */

/* error (log) ? */
