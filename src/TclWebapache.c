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
#include "TclWeb.h"

#define TCLWEBPOOL req->req->pool

int
TclWeb_InitRequest(TclWebRequest *req, Tcl_Interp *interp, void *arg)
{
    request_rec *r;

    r = (request_rec *)arg;
    req->interp = interp;
    req->req = r;
    req->apachereq = ApacheRequest_new(r);
    req->headers_printed = 0;
    req->headers_set = 0;
    return TCL_OK;
}

int
TclWeb_SendHeaders(TclWebRequest *req)
{
    ap_send_http_header(req->req);
    return TCL_OK;
}

/* Set up the content type header */

int
TclWeb_SetHeaderType(char *header, TclWebRequest *req)
{
    if(req->headers_set)
	return TCL_ERROR;

    req->req->content_type = header;
    req->headers_set = 1;
    return TCL_OK;
}

/* Printer headers if they haven't been printed yet */
int
TclWeb_PrintHeaders(TclWebRequest *req)
{
    if (req->headers_printed)
	return TCL_ERROR;

    if (req->headers_set == 0)
	TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);

    ap_send_http_header(req->req);
    req->headers_printed = 1;
    return TCL_OK;
}

/* Print nice HTML formatted errors */
int
TclWeb_PrintError(char *errstr, int htmlflag, TclWebRequest *req)
{
    TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);
    TclWeb_PrintHeaders(req);

    if (htmlflag != 1)
	ap_rputs(ER1, req->req);

    if (errstr != NULL)
    {
	if (htmlflag != 1)
	{
	    ap_rputs(ap_escape_html(TCLWEBPOOL, errstr), req->req);
	} else {
	    ap_rputs(errstr, req->req);
	}
    }
    if (htmlflag != 1)
	ap_rputs(ER2, req->req);

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
    int flag = 0;

    /* This isn't real efficient - move to hash table later
       on... */
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req),
		     strlen(varname)))
	{
	    /* The following makes sure that we get one string,
	       with no sub lists. */
	    if (flag == 0)
	    {
		flag = 1;
		Tcl_SetStringObj(result,
				 TclWeb_StringToUtf(parms[i].val, req), -1);
		Tcl_IncrRefCount(result);
	    } else {
		Tcl_Obj *tmpobj;
		Tcl_Obj *tmpobjv[2];
		tmpobjv[0] = result;
		tmpobjv[1] = TclWeb_StringToUtfToObj(parms[i].val, req);
		tmpobj = Tcl_ConcatObj(2, tmpobjv);
		Tcl_SetStringObj(result, Tcl_GetString(tmpobj), -1);
	    }
	}
    }

    if (result == NULL)
    {
	return TCL_ERROR;
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

	if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req), strlen(varname)))
	{
	    Tcl_ListObjAppendElement(req->interp, result,
				     TclWeb_StringToUtfToObj(parms[i].val, req));
	}
    }

    if (result == NULL)
    {
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
TclWeb_GetAllVars(Tcl_Obj *result, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    for (i = 0; i < parmsarray->nelts; ++i)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].key, req));
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].val, req));
    }

    if (result == NULL)
    {
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
TclWeb_GetVarNames(Tcl_Obj *result, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    for (i = 0; i < parmsarray->nelts; ++i)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].key, req));
    }

    if (result == NULL)
    {
	return TCL_ERROR;
    }

    return TCL_OK;
}

int
TclWeb_VarExists(Tcl_Obj *result, char *varname, TclWebRequest *req)
{
    int i;
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);
    table_entry *parms = (table_entry *)parmsarray->elts;

    /* This isn't real efficient - move to hash table later on. */
    for (i = 0; i < parmsarray->nelts; ++i)
    {
	if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req), strlen(varname)))
	{
	    Tcl_SetIntObj(result, 1);
	    Tcl_IncrRefCount(result);
	    return TCL_OK;
	}
    }
    Tcl_SetIntObj(result, 0);
    Tcl_IncrRefCount(result);
    return TCL_OK;
}

int
TclWeb_VarNumber(Tcl_Obj *result, TclWebRequest *req)
{
    array_header *parmsarray = ap_table_elts(req->apachereq->parms);

    Tcl_SetIntObj(result, parmsarray->nelts);
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
		   TclWeb_StringToUtfToObj(ap_ht_time(TCLWEBPOOL, date, timefmt, 0), req), 0);

    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DATE_GMT", -1),
		   TclWeb_StringToUtfToObj(ap_ht_time(TCLWEBPOOL, date, timefmt, 1), req), 0);


    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("LAST_MODIFIED", -1),
		   TclWeb_StringToUtfToObj(ap_ht_time(TCLWEBPOOL,
						      req->req->finfo.st_mtime,
						      timefmt, 1), req), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_URI", -1),
		   TclWeb_StringToUtfToObj(req->req->uri, req), 0);
    Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_PATH_INFO", -1),
		   TclWeb_StringToUtfToObj(req->req->path_info, req), 0);

#ifndef WIN32
    pw = getpwuid(req->req->finfo.st_uid);
    if (pw)
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("USER_NAME", -1),
	       TclWeb_StringToUtfToObj(ap_pstrdup(TCLWEBPOOL, pw->pw_name), req), 0);
    else
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("USER_NAME", -1),
		       TclWeb_StringToUtfToObj(
			   ap_psprintf(TCLWEBPOOL, "user#%lu",
			   (unsigned long)req->req->finfo.st_uid), req), 0);
#endif

    if ((t = strrchr(req->req->filename, '/')))
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       TclWeb_StringToUtfToObj(++t, req), 0);
    else
	Tcl_ObjSetVar2(req->interp, envvar, Tcl_NewStringObj("DOCUMENT_NAME", -1),
		       TclWeb_StringToUtfToObj(req->req->uri, req), 0);

    if (req->req->args)
    {
	char *arg_copy = ap_pstrdup(TCLWEBPOOL, req->req->args);
	ap_unescape_url(arg_copy);
	Tcl_ObjSetVar2(req->interp, envvar,
	   Tcl_NewStringObj("QUERY_STRING_UNESCAPED", -1),
	   TclWeb_StringToUtfToObj(ap_escape_shell_cmd(TCLWEBPOOL, arg_copy), req), 0);
    }

    /* ----------------------------  */

    /* transfer client request headers to TCL request namespace */
    for (i = 0; i < hdrs_arr->nelts; ++i)
    {
	if (!hdrs[i].key)
	    continue;
	else {
	    Tcl_ObjSetVar2(req->interp, envvar,
			   TclWeb_StringToUtfToObj(hdrs[i].key, req),
			   TclWeb_StringToUtfToObj(hdrs[i].val, req), 0);
	}
    }

    /* transfer apache internal cgi variables to TCL request namespace */
    for (i = 0; i < env_arr->nelts; ++i)
    {
	if (!env[i].key)
	    continue;
	Tcl_ObjSetVar2(req->interp, envvar, TclWeb_StringToUtfToObj(env[i].key, req),
		       TclWeb_StringToUtfToObj(env[i].val, req), 0);
    }

    /* cleanup system cgi variables */
    ap_clear_table(req->req->subprocess_env);

    return TCL_OK;
}

INLINE int
TclWeb_Base64Encode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64encode(TCLWEBPOOL, in);
    return TCL_OK;
}

INLINE int
TclWeb_Base64Decode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64decode(TCLWEBPOOL, in);
    return TCL_OK;
}

INLINE int
TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req)
{
    out = ap_escape_shell_cmd(TCLWEBPOOL, in);
    return TCL_OK;
}

/* Functions to convert strings to UTF encoding */

/* These API's are a bit different, because it's so much more
 * practical. */

char *TclWeb_StringToUtf(char *in, TclWebRequest *req)
{
    char *tmp;
    Tcl_DString dstr;
    Tcl_DStringInit(&dstr);
    Tcl_ExternalToUtfDString(NULL, in, (signed)strlen(in), &dstr);

    tmp = ap_pstrdup(TCLWEBPOOL, Tcl_DStringValue(&dstr));
    Tcl_DStringFree(&dstr);
    return tmp;
}

INLINE Tcl_Obj *
TclWeb_StringToUtfToObj(char *in, TclWebRequest *req)
{
    return Tcl_NewStringObj(TclWeb_StringToUtf(in, req), -1);
}


/* output/write/flush?  */

/* error (log) ?  send to stderr. */
