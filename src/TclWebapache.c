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

#include "apache_request.h"
#include "apache_cookie.h"
#include "mod_rivet.h"

#define TCLWEBPOOL req->req->pool

typedef struct TclWebRequest {
    Tcl_Interp *interp;
    request_rec *req;
    ApacheRequest *apachereq;
} TclWebRequest;

#include "TclWeb.h"

int
TclWeb_InitRequest(TclWebRequest *req, void *arg)
{
    req = (TclWebRequest *)Tcl_Alloc(sizeof(TclWebRequest));
    req->req = (request_rec *)arg;
    req->apachereq = ApacheRequest_new(req->req);
    return TCL_OK;
}

int
TclWeb_SendHeaders(TclWebRequest *req)
{
    ap_send_http_header(req->req);
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
TclWeb_GetVar(Tcl_Obj *result, char *varname, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    result = NULL;

    /* This isn't real efficient - move to hash table later
       on... */
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	if (!strncmp(varname, Rivet_StringToUtf(parms[i].key, TCLWEBPOOL), strlen(varname)))
	{
	    /* The following makes sure that we get one string,
	       with no sub lists. */
	    if (result == NULL)
	    {
		result = STRING_TO_UTF_TO_OBJ(parms[i].val, TCLWEBPOOL);
		Tcl_IncrRefCount(result);
	    } else {
		Tcl_Obj *tmpobjv[2];
		tmpobjv[0] = result;
		tmpobjv[1] = STRING_TO_UTF_TO_OBJ(parms[i].val, TCLWEBPOOL);
		result = Tcl_ConcatObj(2, tmpobjv);
	    }
	}
    }

    if (result == NULL)
    {
	result = Tcl_NewStringObj("", -1);
	Tcl_IncrRefCount(result);
    }

    return TCL_OK;
}

int
TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    /* This isn't real efficient - move to hash table later on. */
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	if (!strncmp(varname, Rivet_StringToUtf(parms[i].key, TCLWEBPOOL), strlen(varname)))
	{
	    if (result == NULL)
	    {
		result = Tcl_NewObj();
		Tcl_IncrRefCount(result);
	    }
	    Tcl_ListObjAppendElement(req->interp, result,
				     STRING_TO_UTF_TO_OBJ(parms[i].val, TCLWEBPOOL));
	}
    }

    if (result == NULL)
    {
	result = Tcl_NewStringObj("", -1);
	Tcl_IncrRefCount(result);
    }
    return TCL_OK;
}

int
TclWeb_GetAllVars(Tcl_Obj *result, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    result = Tcl_NewObj();
    Tcl_IncrRefCount(result);
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 STRING_TO_UTF_TO_OBJ(parms[i].key, TCLWEBPOOL));
	Tcl_ListObjAppendElement(req->interp, result,
				 STRING_TO_UTF_TO_OBJ(parms[i].val, TCLWEBPOOL));
    }

    if (result == NULL)
    {
	result = Tcl_NewStringObj("", -1);
	Tcl_IncrRefCount(result);
    }

    return TCL_OK;
}

int
TclWeb_GetVarNames(Tcl_Obj *result, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    result = Tcl_NewObj();
    Tcl_IncrRefCount(result);

    result = Tcl_NewObj();
    Tcl_IncrRefCount(result);
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 STRING_TO_UTF_TO_OBJ(parms[i].key, TCLWEBPOOL));
    }

    if (result == NULL)
    {
	result = Tcl_NewStringObj("", -1);
	Tcl_IncrRefCount(result);
    }

    return TCL_OK;
}

int
TclWeb_VarExists(char *varname, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    /* This isn't real efficient - move to hash table later on. */
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	if (!strncmp(varname, Rivet_StringToUtf(parms[i].key, TCLWEBPOOL), strlen(varname)))
	{
	    return TCL_OK;
	}
    }
    return TCL_ERROR;
}

int
TclWeb_VarNumber(Tcl_Obj *result, TclWebRequest *req)
{
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);

    result = Tcl_NewIntObj(parmsarray->nelts);
    Tcl_IncrRefCount(result);
    return TCL_OK;
}

int
TclWeb_GetCookieVars(Tcl_Obj *cookievar, TclWebRequest *req)
{
    int i;
    ApacheCookieJar *cookies = ApacheCookie_parse(req->req, NULL);

    for (i = 0; i < ApacheCookieJarItems(cookies); i++) {
	ApacheCookie *c = ApacheCookieJarFetch(cookies, i);
	int j;
	for (j = 0; j < ApacheCookieItems(c); j++) {
	    char *name = c->name;
	    char *value = ApacheCookieFetch(c, j);
	    Tcl_ObjSetVar2(req->interp, cookievar,
			   Tcl_NewStringObj(name, -1),
			   Tcl_NewStringObj(value, -1), 0);
	}
    }

    return TCL_OK;
}

int
TclWeb_GetEnvVars(Tcl_Obj *envvar, TclWebRequest *req)
{
    char *timefmt = DEFAULT_TIME_FORMAT;
#ifndef WIN32
    struct passwd *pw;
#endif /* ndef WIN32 */
    char *t;
    time_t date;

    int i;

    array_header *hdrs_arr;
    table_entry *hdrs;
    array_header *env_arr;
    table_entry  *env;

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

    /* These were the "include vars"  */
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DATE_LOCAL", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(TCLWEBPOOL,
					date, timefmt, 0), TCLWEBPOOL), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DATE_GMT", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(TCLWEBPOOL,
					date, timefmt, 1), TCLWEBPOOL), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("LAST_MODIFIED", -1),
		   STRING_TO_UTF_TO_OBJ(ap_ht_time(TCLWEBPOOL,
					req->req->finfo.st_mtime,
					timefmt, 0), TCLWEBPOOL), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_URI", -1),
		   STRING_TO_UTF_TO_OBJ(req->req->uri, TCLWEBPOOL), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_PATH_INFO", -1),
		   STRING_TO_UTF_TO_OBJ(req->req->path_info, TCLWEBPOOL), 0);

#ifndef WIN32
    pw = getpwuid(req->req->finfo.st_uid);
    if (pw)
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("USER_NAME", -1),
	       STRING_TO_UTF_TO_OBJ(ap_pstrdup(TCLWEBPOOL, pw->pw_name), TCLWEBPOOL), 0);
    else
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("USER_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(
			   ap_psprintf(TCLWEBPOOL, "user#%lu",
			   (unsigned long)req->req->finfo.st_uid), TCLWEBPOOL), 0);
#endif

    if ((t = strrchr(req->req->filename, '/')))
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(++t, TCLWEBPOOL), 0);
    else
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       STRING_TO_UTF_TO_OBJ(req->req->uri, TCLWEBPOOL), 0);

    if (req->req->args)
    {
	char *arg_copy = ap_pstrdup(TCLWEBPOOL, req->req->args);
	ap_unescape_url(arg_copy);
	Tcl_ObjSetVar2(req->interp, envvar,
	   Tcl_NewStringObj("QUERY_STRING_UNESCAPED", -1),
	   STRING_TO_UTF_TO_OBJ(ap_escape_shell_cmd(TCLWEBPOOL, arg_copy), TCLWEBPOOL), 0);
    }

    /* ----------------------------  */

    /* transfer client request headers to TCL request namespace */
    for (i = 0; i < hdrs_arr->nelts; ++i)
    {
	if (!hdrs[i].key)
	    continue;
	else {
	    Tcl_ObjSetVar2(req->interp, envvar,
			   STRING_TO_UTF_TO_OBJ(hdrs[i].key, TCLWEBPOOL),
			   STRING_TO_UTF_TO_OBJ(hdrs[i].val, TCLWEBPOOL), 0);
	}
    }

    /* transfer apache internal cgi variables to TCL request namespace */
    for (i = 0; i < env_arr->nelts; ++i)
    {
	if (!env[i].key)
	    continue;
	Tcl_ObjSetVar2(req->interp, envvar, STRING_TO_UTF_TO_OBJ(env[i].key, TCLWEBPOOL),
		       STRING_TO_UTF_TO_OBJ(env[i].val, TCLWEBPOOL), 0);
    }

    /* cleanup system cgi variables */
    ap_clear_table(req->req->subprocess_env);

    return TCL_OK;
}

int
TclWeb_Base64Encode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64encode(TCLWEBPOOL, in);
    return TCL_OK;
}

int
TclWeb_Base64Decode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64decode(TCLWEBPOOL, in);
    return TCL_OK;
}

int
TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req)
{
    out = ap_escape_shell_cmd(TCLWEBPOOL, in);
    return TCL_OK;
}

/* output/write/flush?  */

/* error (log) ?  send to stderr. */
