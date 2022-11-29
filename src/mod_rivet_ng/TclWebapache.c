/* -- TclWeb.c: Common API layer. */

/*
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

/* Rivet config */

#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <tcl.h>
#include <sys/types.h>
#ifndef WIN32
#include <pwd.h>
#endif /* WIN32 */

#include <httpd.h>
#include <http_request.h>
#include <ap_compat.h>
#include <apr_strings.h>
#include "apache_request.h"
#include "mod_rivet.h"
#include "TclWeb.h"

extern module rivet_module;
extern mod_rivet_globals* module_globals;

/* It's kind of an overkill: we define macros for handling the
 * flags that control the handling of the three environment variables
 * classes (common, CGI and include variables). */

#define ENV_COMMON_VARS_M    1
#define ENV_CGI_VARS_M       2
#define ENV_VARS_M           4
#define ENV_VARS_RESET(env)  env = 0;
#define ENV_COMMON_VARS(env) env |= ENV_COMMON_VARS_M;
#define ENV_CGI_VARS(env)    env |= ENV_CGI_VARS_M;
#define ENV_VARS(env)        env |= ENV_VARS_M;
#define ENV_LOADED(env)      env |= ENV_COMMON_VARS_M | ENV_CGI_VARS_M | ENV_VARS_M;
#define ENV_IS_LOADED(env)          (env == (ENV_COMMON_VARS_M | ENV_CGI_VARS_M | ENV_VARS_M))
#define ENV_COMMON_VARS_LOADED(env) (env & ENV_COMMON_VARS_M) != 0
#define ENV_CGI_VARS_LOADED(env)    (env & ENV_CGI_VARS_M) != 0
#define ENV_VARS_LOADED(env)        (env & ENV_VARS_M) != 0

/* This is used below to determine what part of the parmsarray to parse. */

#define PARMSARRAY_COORDINATES(i,j,parray,nargs) i = 0; j = parray->nelts; \
if (source == VAR_SRC_QUERYSTRING) { j = nargs; } \
else if (source == VAR_SRC_POST) { i = nargs; }

/*
 * -- TclWeb_NewRequestObject
 *
 *
 */

TclWebRequest*
TclWeb_NewRequestObject (apr_pool_t *p)
{
    TclWebRequest* req = (TclWebRequest *)apr_pcalloc(p, sizeof(TclWebRequest));

    req->interp             = NULL;
    req->req                = NULL;
    req->apachereq          = ApacheRequest_new(p);
    req->headers_printed    = 0;
    req->headers_set        = 0;
    ENV_VARS_RESET(req->environment_set)
    req->charset            = NULL;  /* we will test against NULL to check if a charset *
                                      * was specified in the conf                       */
    return req;
}

/*
 * -- TclWeb_InitRequest
 *
 * called once on every HTTP request initializes fields and
 * objects referenced in a TclWebRequest object
 *
 * Arguments:
 *
 *  TclWebRequest* req:     a pointer to a TclWebRequest object to be intialized
 *  Tcl_Interp*    interp:  current Tcl_Interp object serving the request
 *  void*          arg:     generic pointer. Current implementation passes the
 *                          request_rec object pointer
 *
 */

int
TclWeb_InitRequest(rivet_thread_private* private, Tcl_Interp *interp)
{
    request_rec*        r   = private->r;
    TclWebRequest*      req = private->req;
    size_t content_type_len = strlen(r->content_type);

    req->interp             = interp;
    req->req                = r;
    req->apachereq          = ApacheRequest_init(req->apachereq,r);
    req->headers_printed    = 0;
    req->headers_set        = 0;
    ENV_VARS_RESET(req->environment_set)
    req->charset            = NULL;

    /*
     * if strlen(req->content_type) > strlen([RIVET|TCL]_FILE_CTYPE)
     * a charset parameters might be in the configuration like
     *
     * AddType 'application/x-httpd-rivet;charset=utf-8' rvt
     */

    if (((private->ctype==RIVET_TEMPLATE) && (content_type_len > strlen(RIVET_TEMPLATE_CTYPE))) || \
        ((private->ctype==RIVET_TCLFILE) && (content_type_len > strlen(RIVET_TCLFILE_CTYPE)))) {

        char* charset;

        /* we parse the content type: we are after a 'charset' parameter definition */

        charset = strstr(r->content_type,"charset");
        if (charset != NULL) {
            charset = apr_pstrdup(r->pool,charset);

            /* ther's some freedom about spaces in the AddType lines: let's strip them off */

            apr_collapse_spaces(charset,charset);
            req->charset = charset;
        }
    }

    return TCL_OK;
}

INLINE int
TclWeb_SendHeaders(TclWebRequest *req)
{
    //TODO: fix ap_send_http_header

    ap_send_http_header(req->req);

    return TCL_OK;
}

INLINE int
TclWeb_StopSending(TclWebRequest *req)
{
    req->req->connection->aborted = 1;
    return TCL_OK;
}

/* Set up the content type header */

int
TclWeb_SetHeaderType(char *header, TclWebRequest *req)
{

    if (req->headers_set)
        return TCL_ERROR;

    ap_set_content_type(req->req,apr_pstrdup(req->req->pool, header));
    req->headers_set = 1;
    return TCL_OK;
}

/* Printer headers if they haven't been printed yet */
int
TclWeb_PrintHeaders(TclWebRequest *req)
{
    if (req->headers_printed)
        return TCL_ERROR;

    /* Let's set the charset in the headers if one was set in the configuration  */

    if (!req->headers_set && (req->charset != NULL)) {
        TclWeb_SetHeaderType(apr_pstrcat(req->req->pool,"text/html;",req->charset,NULL),req);
    }

    if (req->headers_set == 0)
    {
        TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);
    }

    /*
     * seems that ap_send_http_header is redefined to ; in Apache2.2
     * ap_send_http_header(req->req);
     */

    TclWeb_SendHeaders(req);

    req->headers_printed = 1;
    return TCL_OK;
}

/* Print nice HTML formatted errors */
int
TclWeb_PrintError(CONST84 char *errstr, int htmlflag, TclWebRequest *req)
{
    TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);
    TclWeb_PrintHeaders(req);

    if (htmlflag != 1)
	ap_rputs(ER1, req->req);

    if (errstr != NULL)
    {
        if (htmlflag != 1)
        {
            ap_rputs(ap_escape_html(req->req->pool,errstr),req->req);
        } else {
            ap_rputs(errstr, req->req);
        }
    }
    if (htmlflag != 1)
	ap_rputs(ER2, req->req);

    return TCL_OK;
}

INLINE int
TclWeb_HeaderSet(char *header, char *val, TclWebRequest *req)
{
    apr_table_set(req->req->headers_out, header, val);
    return TCL_OK;
}

/*  * accessing output headers *
 *
 *  -- TclWeb_OutputHeaderSet: replicates the role of TclWeb_HeaderSet
 *
 *  - the name stresses the fact it's an accessor to the output
 *    headers
 *  - it returns nothing since it's a wrapper around an APR call
 *    that doesn't return anything
 *
 *  -- TclWeb_OutputHeaderGet: reads from the output headers and
 *  returns the value associated to a key. If the key is not
 *  existing it returns NULL
 *
 */

INLINE void
TclWeb_OutputHeaderSet(char *header, char *val, TclWebRequest *req)
{
    apr_table_set(req->req->headers_out, header, val);
}

INLINE const char*
TclWeb_OutputHeaderGet(char *header, TclWebRequest *req)
{
    return apr_table_get(req->req->headers_out, header);
}

INLINE int
TclWeb_HeaderAdd(char *header, char *val, TclWebRequest *req)
{
    apr_table_add(req->req->headers_out, header, val);
    return TCL_OK;
}

INLINE int
TclWeb_SetStatus(int status, TclWebRequest *req)
{
    req->req->status = status;
    return TCL_OK;
}

INLINE int
TclWeb_MakeURL(Tcl_Obj *result, char *filename, TclWebRequest *req)
{
    Tcl_SetStringObj(result,ap_construct_url(req->req->pool,filename,req->req),-1);
    return TCL_OK;
}

int
TclWeb_GetVar(Tcl_Obj *result, char *varname, int source, TclWebRequest *req)
{
    int i,j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)apr_table_elts(req->apachereq->parms);
    apr_table_entry_t  *parms = (apr_table_entry_t *)parmsarray->elts;
    int flag = 0;

    PARMSARRAY_COORDINATES(i,j,parmsarray,req->apachereq->nargs)

    /* This isn't real efficient - move to hash table later on... */
    while (i < j)
    {
        char *parmkey = TclWeb_StringToUtf(parms[i].key, req);
        if (!strncmp(varname,parmkey,
                    strlen(varname) < strlen(parmkey) ?
                    strlen(parmkey) : strlen(varname)))
        {

            /* The following makes sure that we get one string,
               with no sub lists. */

            if (flag == 0)
            {

                flag = 1;
                Tcl_SetStringObj (result,TclWeb_StringToUtf(parms[i].val,req),-1);

            } else {

                Tcl_Obj *tmpobj;
                Tcl_Obj *tmpobjv[2];
                tmpobjv[0] = result;
                tmpobjv[1] = TclWeb_StringToUtfToObj (parms[i].val,req);
                tmpobj = Tcl_ConcatObj (2,tmpobjv);
                Tcl_SetStringObj (result,Tcl_GetString(tmpobj),-1);

            }

        }
        i++;
    }

    /*
     * We are assuming that checking result->length is a sane way to
     * establish the Tcl object representation character lenght but it
     * would obviously be more appropriate to call Tcl_GetCharLength(result)
     */

    if (result->length == 0) {
        Tcl_AddErrorInfo(req->interp,apr_psprintf(req->req->pool,"Variable '%s' not found",varname));
        return TCL_ERROR;
    }

    return TCL_OK;
}

int
TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, int source, TclWebRequest *req) {
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES(i,j,parmsarray,req->apachereq->nargs)

    /* This isn't real efficient - move to hash table later on. */
    while (i < j)
    {
        int tclcode;

        if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req),
                 strlen(varname) < strlen(parms[i].key) ?
                 strlen(parms[i].key) : strlen(varname)))
        {
            tclcode = Tcl_ListObjAppendElement(req->interp,result,
                                               TclWeb_StringToUtfToObj(parms[i].val, req));
            if (tclcode != TCL_OK) { return tclcode; }
        }
        i++;
    }

    return TCL_OK;
}

int
TclWeb_GetAllVars(Tcl_Obj *result, int source, TclWebRequest *req)
{
    int i,j;
    apr_array_header_t *parmsarray = (apr_array_header_t *) apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES(i,j,parmsarray,req->apachereq->nargs)

    while (i < j)
    {
        int tclcode;
        tclcode = Tcl_ListObjAppendElement(req->interp,result,
                       TclWeb_StringToUtfToObj(parms[i].key,req));
        if (tclcode != TCL_OK) { return tclcode; }
        tclcode = Tcl_ListObjAppendElement(req->interp,result,
                       TclWeb_StringToUtfToObj(parms[i].val,req));
        if (tclcode != TCL_OK) { return tclcode; }

        i++;
    }

    return TCL_OK;
}

int
TclWeb_GetVarNames(Tcl_Obj *result, int source, TclWebRequest *req)
{
    int i,j;
    apr_array_header_t *parmsarray = (apr_array_header_t *) apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES(i,j,parmsarray,req->apachereq->nargs)

    while (i < j)
    {
        int tclcode;
        tclcode= Tcl_ListObjAppendElement(req->interp, result,
                                          TclWeb_StringToUtfToObj(parms[i].key, req));
        if (tclcode != TCL_OK) { return tclcode; }
        i++;
    }

    return TCL_OK;
}

int
TclWeb_VarExists(Tcl_Obj *result, char *varname, int source, TclWebRequest *req)
{
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES(i,j,parmsarray,req->apachereq->nargs)

    /* This isn't real efficient - move to hash table later on. */
    while (i < j)
    {
        if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req),
                    strlen(varname) < strlen(parms[i].key) ?
                    strlen(parms[i].key) : strlen(varname)))
        {
            Tcl_SetIntObj(result, 1);
            return TCL_OK;
        }
        i++;
    }
    Tcl_SetIntObj(result, 0);
    return TCL_OK;
}

int
TclWeb_VarNumber(Tcl_Obj *result, int source, TclWebRequest *req)
{
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);

    if (source == VAR_SRC_QUERYSTRING) {
	    Tcl_SetIntObj(result, req->apachereq->nargs);
    } else if (source == VAR_SRC_POST) {
	    Tcl_SetIntObj(result, parmsarray->nelts - req->apachereq->nargs);
    } else {
	    Tcl_SetIntObj(result, parmsarray->nelts);
    }

    return TCL_OK;
}

/* Environment variables. Include variables handling */

/* These 2 array must be aligned and a one-to-one correspondence preserved
 * The enum include_vars_idx *must* be terminated by 'invalid_env_var'
 * Adding a new env variable requires
 *    + the name of the variable be listed in include_env_vars
 *    + a new value in the enumerator include_vars_idx must be added in the
 *      corresponding position
 *    + the switch construct in function TclWeb_SelectEnvIncludeVar must
 *      be expanded to handle the new case identified by the new enumerator value
 */

static const char* include_env_vars[] =
{
    "DATE_LOCAL","DATE_GMT","LAST_MODIFIED","DOCUMENT_URI","DOCUMENT_PATH_INFO","DOCUMENT_NAME",
    "QUERY_STRING_UNESCAPED","USER_NAME","RIVET_CACHE_FREE","RIVET_CACHE_SIZE",
    NULL
};
enum include_vars_idx {
    date_local=0,date_gmt,last_modified,document_uri,document_path_info,document_name,
    query_string_unescaped,user_name,rivet_cache_free,rivet_cache_size,
    invalid_env_var
};

/*  -- TclWeb_SelectEnvIncludeVar
 *
 *  Depending on the value idx of the enumerator a method is selected
 *  to return a string of a specific environment variable methods
 *  Adding new environment variables need new cases of the switch
 *  construct to be added, provided the data can be obtained from
 *  the rivet_thread_private structure
 *
 *   Arguments:
 *
 *      + rivet_thread_private* private: pointer to a thread private data structure
 *      + int idx: an integer value listed in the enumerator include_vars_idx
 *
 *   Results:
 *
 *      A character string pointer to the value of the environment variable or
 *      NULL if the enumerator value idx was invalid or resolving the environment
 *      variable was impossible
 *
 */

static char*
TclWeb_SelectEnvIncludeVar (rivet_thread_private* private,int idx)
{
    switch (idx)
    {
        case date_local:
        {
            apr_pool_t* pool = private->req->req->pool;
            apr_time_t date = private->req->req->request_time;

            return ap_ht_time(pool,date,DEFAULT_TIME_FORMAT,0);
        }
        case date_gmt:
        {
            apr_pool_t* pool = private->req->req->pool;
            apr_time_t date = private->req->req->request_time;

            return ap_ht_time(pool,date,DEFAULT_TIME_FORMAT,1);
        }
        case last_modified:
        {
            apr_pool_t* pool = private->req->req->pool;

            return ap_ht_time(pool,private->req->req->finfo.mtime,DEFAULT_TIME_FORMAT,1);
        }
        case document_uri:
        {
            return private->req->req->uri;
        }
        case document_path_info:
        {
            return private->req->req->path_info;
        }
        case document_name:
        {
            char *t;

            if ((t = strrchr(private->req->req->filename,'/'))) {
                return ++t;
            } else {
                return private->req->req->uri;
            }
        }
        case query_string_unescaped:
        {
            if (private->req->req->args) {
                apr_pool_t* pool = private->req->req->pool;
                char *arg_copy = (char*) apr_pstrdup(pool,private->req->req->args);

                ap_unescape_url(arg_copy);
                return ap_escape_shell_cmd(pool,arg_copy);
            } else {
                return NULL;
            }

        }
        case user_name:
        {
#ifndef WIN32
            struct passwd *pw = (struct passwd *) getpwuid(private->req->req->finfo.user);
            if (pw) {
                //apr_table_set( table, "USER_NAME",
                //        apr_pstrdup( pool, pw->pw_name ) );
                return pw->pw_name;
            } else {
                apr_pool_t* pool = private->req->req->pool;
                return (char*) apr_psprintf(pool,"user#%lu",(unsigned long)private->req->req->finfo.user);
            }
#else
            return NULL;
#endif
        }
        case rivet_cache_free:
        {
            apr_pool_t* pool = private->req->req->pool;
            return (char*) apr_psprintf (pool, "%d",(RIVET_PEEK_INTERP(private,private->running_conf))->cache_free);
        }
        case rivet_cache_size:
        {
            apr_pool_t* pool = private->req->req->pool;
            return (char*) apr_psprintf (pool, "%d",(RIVET_PEEK_INTERP(private,private->running_conf))->cache_size);
        }
    }
    return NULL;
}

/*
 * -- TclWeb_InitEnvVars
 *
 * Load the CGI and environment variables into the request_rec environment structure
 * Variables belong to 3 cathegories
 *
 *   + common variables (ap_add_common_vars)
 *   + CGI variables (ad_cgi_vars)
 *   + a miscellaneous set of variables
 *     listed in the array include_env_vars
 *
 * Each cathegory is controlled by flags in order to reduce the overhead of getting them
 * into request_rec in case previous call to ::rivet::env could have already forced them
 * into request_rec
 */

static void
TclWeb_InitEnvVars (rivet_thread_private* private)
{
    TclWebRequest *req = private->req;

    if (ENV_IS_LOADED(req->environment_set)) return;

    /* Retrieve cgi variables. */
    if (!ENV_CGI_VARS_LOADED(req->environment_set))
    {
        ap_add_cgi_vars(req->req);
    }
    if (!ENV_COMMON_VARS_LOADED(req->environment_set))
    {
        ap_add_common_vars(req->req);
    }

    /* Loading into 'req->req->subprocess_env' the include vars */

    /* actually this check is not necessary. ENV_VARS_M is set only here therefore
     * if it's set this function has been called already and it should have returned
     * at the beginning of ies execution. I keep it for clarity and uniformity with the
     * CGI variables and in case the incremental environment handling is extended
     */

    if (!ENV_VARS_LOADED(req->environment_set))
    {
        apr_table_t   *table;
        int            idx;

        table = req->req->subprocess_env;
        for (idx = 0;idx < invalid_env_var;idx++)
        {
            apr_table_set(table,include_env_vars[idx],TclWeb_SelectEnvIncludeVar(private,idx));
        }
    }

    ENV_LOADED(req->environment_set)
}

/* -- TclWeb_GetEnvIncludeVar
 *
 *  the environment variable named in key is searched among the include
 *  variables and then resolved by calling TclWeb_SelectEnvIncludeVar
 *
 *  Result:
 *
 *    a character string pointer to the environment variable value or
 *    NULL if the environment variable name in invalid or the variable
 *    could not be resolved
 *
 */

static char*
TclWeb_GetEnvIncludeVar (rivet_thread_private* private,char* key)
{
    int idx;

    for (idx = 0;idx < invalid_env_var; idx++)
    {
        const char* include_var_p = include_env_vars[idx];
        if (strncmp(key,include_var_p,strlen(key) < strlen(include_var_p) ? strlen(key) : strlen(include_var_p)) == 0)
        {
            return TclWeb_SelectEnvIncludeVar(private,idx);
        }
    }
    return NULL;
}

int
TclWeb_GetEnvVars(Tcl_Obj *envvar,rivet_thread_private* private)
{
    int i;

    apr_array_header_t *env_arr;
    apr_table_entry_t  *env;
    Tcl_Obj *key;
    Tcl_Obj *val;
    TclWebRequest *req;

    TclWeb_InitEnvVars(private);

    req = private->req;
    Tcl_IncrRefCount(envvar);
    /* Transfer Apache internal CGI variables to TCL request namespace. */
    env_arr = (apr_array_header_t *) apr_table_elts(req->req->subprocess_env);
    env     = (apr_table_entry_t *) env_arr->elts;
    for (i = 0; i < env_arr->nelts; ++i)
    {
        if ((!env[i].key) || (!env[i].val)) {
            continue;
        }

        key = TclWeb_StringToUtfToObj(env[i].key, req);
        val = TclWeb_StringToUtfToObj(env[i].val, req);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);

        /* Variable scope resolution changed to default (flags: 0)
         * to enable creation of the array in the caller's local scope.
         * Default behavior (creation in the ::request namespace)
         * is now more consistently constrained by fully qualifying
         * the default array names (see rivetCore.c). This should fix
         * Bug #48963
         */

        Tcl_ObjSetVar2(req->interp, envvar, key, val, 0);

        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(envvar);

    return TCL_OK;
}

int
TclWeb_GetHeaderVars(Tcl_Obj *headersvar,rivet_thread_private* private)
{
    int i;
    TclWebRequest *req;
    apr_array_header_t *hdrs_arr;
    apr_table_entry_t  *hdrs;
    Tcl_Obj *key;
    Tcl_Obj *val;

    req = private->req;

    // I actually don't see why we need to load the whole environment here
    //TclWeb_InitEnvVars(private);

    Tcl_IncrRefCount(headersvar);
    /* Transfer client request headers to TCL request namespace. */
    hdrs_arr = (apr_array_header_t*) apr_table_elts(req->req->headers_in);
    hdrs = (apr_table_entry_t *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; ++i)
    {
        if (!hdrs[i].key)
            continue;

        key = TclWeb_StringToUtfToObj(hdrs[i].key, req);
        val = TclWeb_StringToUtfToObj(hdrs[i].val, req);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);

            /* See comment in TclWeb_GetEnvVars concerning Bug #48963*/

        Tcl_ObjSetVar2(req->interp, headersvar, key, val, 0);
        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }

    /* Transfer Apache internal CGI variables to TCL request namespace. */
    Tcl_DecrRefCount(headersvar);
    return TCL_OK;
}

INLINE int
TclWeb_Base64Encode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64encode(req->req->pool, in);
    return TCL_OK;
}

INLINE int
TclWeb_Base64Decode(char *out, char *in, TclWebRequest *req)
{
    out = ap_pbase64decode(req->req->pool, in);
    return TCL_OK;
}

INLINE int
TclWeb_EscapeShellCommand(char *out, char *in, TclWebRequest *req)
{
    out = ap_escape_shell_cmd(req->req->pool, in);
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
    tmp = (char*) apr_pstrdup(req->req->pool, Tcl_DStringValue(&dstr));
    Tcl_DStringFree(&dstr);
    return tmp;
}

INLINE Tcl_Obj *
TclWeb_StringToUtfToObj(char *in, TclWebRequest *req)
{
    return Tcl_NewStringObj(TclWeb_StringToUtf(in, req), -1);
}

int TclWeb_PrepareUpload(char *varname, TclWebRequest *req)
{
    req->upload = ApacheUpload_find(req->apachereq->upload, varname);
    if (req->upload == NULL) {
	return TCL_ERROR;
    } else {
	return TCL_OK;
    }
}

int TclWeb_UploadChannel(char *varname, TclWebRequest *req)
{
    Tcl_Channel chan;

    chan = Tcl_OpenFileChannel(req->interp, req->upload->tempname, "r", 0);

    if (chan == NULL) {
	    return TCL_ERROR;
    } else {
        Tcl_Obj* result;

        if (Tcl_SetChannelOption(req->interp,chan,"-translation","binary") == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (Tcl_SetChannelOption(req->interp,chan,"-encoding","binary") == TCL_ERROR) {
            return TCL_ERROR;
        }
        Tcl_RegisterChannel(req->interp,chan);

        result = Tcl_NewObj();
        Tcl_SetStringObj(result, Tcl_GetChannelName(chan), -1);
        Tcl_SetObjResult(req->interp, result);

        return TCL_OK;
    }
}

int TclWeb_UploadTempname(TclWebRequest *req)
{
    Tcl_Obj *tempname = Tcl_NewObj();

    Tcl_SetStringObj(tempname,TclWeb_StringToUtf(req->upload->tempname,req), -1);
    Tcl_SetObjResult(req->interp, tempname);

    return TCL_OK;
}


int TclWeb_UploadSave(char *varname, Tcl_Obj *filename, TclWebRequest *req)
{
	apr_status_t status;

	status = apr_file_copy(req->upload->tempname ,Tcl_GetString(filename),APR_FILE_SOURCE_PERMS,req->req->pool);
	if (status == APR_SUCCESS) {
	    return TCL_OK;
	} else {

        /* apr_strerror docs don't tell anything about a demanded buffer size, we're just adopting a reasonable guess */

        char  error_msg[1024];
        char* tcl_error_msg;
        apr_strerror(status,error_msg,1024);

        tcl_error_msg = apr_psprintf(req->req->pool,"Error copying upload '%s' to '%s' (%s)", req->upload->tempname,
                                                                                              Tcl_GetString(filename),
                                                                                              error_msg);

        Tcl_AddErrorInfo(req->interp,tcl_error_msg);
		return TCL_ERROR;
	}
}

int TclWeb_UploadData(char *varname, TclWebRequest *req)
{
    Tcl_Obj* result;
    rivet_server_conf *rsc = NULL;

    rsc = RIVET_SERVER_CONF( req->req->server->module_config );
    /* This sucks - we should use the hook, but I want to
       get everything fixed and working first */
    if (rsc->upload_files_to_var)
    {
        Tcl_Channel chan;

        chan = Tcl_OpenFileChannel (req->interp, req->upload->tempname, "r", 0);
        if (chan == NULL) {
            return TCL_ERROR;
        }
        if (Tcl_SetChannelOption(req->interp, chan,
                     "-translation", "binary") == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (Tcl_SetChannelOption(req->interp, chan,
                     "-encoding", "binary") == TCL_ERROR) {
            return TCL_ERROR;
        }

        /* Put data in a variable  */
        result = Tcl_NewObj();
        Tcl_ReadChars(chan, result, (int)ApacheUpload_size(req->upload), 0);
        if (Tcl_Close(req->interp, chan) == TCL_ERROR) {
            return TCL_ERROR;
        }

        Tcl_SetObjResult(req->interp, result);
    } else {
        Tcl_AppendResult(req->interp,
                 "RivetServerConf UploadFilesToVar is not set", NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

int TclWeb_UploadSize(TclWebRequest *req)
{
    Tcl_Obj* result = Tcl_NewObj();
    Tcl_SetIntObj(result, (int)ApacheUpload_size(req->upload));
    Tcl_SetObjResult(req->interp, result);
    return TCL_OK;
}

int TclWeb_UploadType(TclWebRequest *req)
{
    Tcl_Obj *type = Tcl_NewObj();

    /* If there is a type, return it, if not, return blank. */
    Tcl_SetStringObj(type, ApacheUpload_type(req->upload)
		     ? (char *)ApacheUpload_type(req->upload) : (char *)"", -1);

    Tcl_SetObjResult(req->interp, type);
    return TCL_OK;
}

int TclWeb_UploadFilename(TclWebRequest *req)
{
    Tcl_Obj *filename = Tcl_NewObj();
    Tcl_SetStringObj(filename,TclWeb_StringToUtf(req->upload->filename,req), -1);

    Tcl_SetObjResult(req->interp, filename);
    return TCL_OK;
}

int TclWeb_UploadNames(TclWebRequest *req)
{
    ApacheUpload *upload;
    Tcl_Obj      *names = Tcl_NewObj();

    upload = ApacheRequest_upload(req->apachereq);
    while (upload)
    {
        Tcl_ListObjAppendElement(req->interp,names,TclWeb_StringToUtfToObj(upload->name,req));
        upload = upload->next;
    }

    Tcl_SetObjResult(req->interp,names);
    return TCL_OK;
}

/*
 * -- TclWeb_GetEnvVar
 *
 * basically is the core of the ::rivet::env rivet command. The argument to
 * the command is stored in 'key' and the function starts a search in various
 * tables following the following order
 *
 *  + though undocumented in the manual the first table checked is HTTP
 *    headers table. ::rivet::env is actually like ::rivet::headers but for
 *    the *request_rec->headers_in table
 *  + the common CGI variables table is checked
 *  + the CGI 1.1 headers table is checked
 *  + the include variables list is checked calling TclWeb_GetEnvIncludeVar
 *
 *  Arguments:
 *
 *   - key: a string with the environment variable name
 *
 *  Results:
 *
 *    - a string pointer to the string with the variable translation or
 *    NULL if the environment variable is not found
 *
 */

char *
TclWeb_GetEnvVar(rivet_thread_private* private,char *key)
{
    char *val;
    TclWebRequest *req = private->req;

    /* Check to see if it's a header variable first. */
    val = (char *)apr_table_get (req->req->headers_in,key);
    if (val) { return val; }

    /* We incrementally prepare subprocess_env */
    /* CGI common vars first */

    if (!ENV_COMMON_VARS_LOADED(req->environment_set))
    {
        ap_add_common_vars(req->req);
        ENV_COMMON_VARS(req->environment_set)
    }
    val = (char *)apr_table_get(req->req->subprocess_env,key);
    if (val) { return val; }

    /* CGI HTTP 1.1 vars */

    if (!ENV_CGI_VARS_LOADED(req->environment_set))
    {
        ap_add_cgi_vars(req->req);
        ENV_CGI_VARS(req->environment_set)
    }
    val = (char *)apr_table_get(req->req->subprocess_env,key);
    if (val) { return val; }

    /* If everything failed we assumed the variable is one of
     * the 'include variables' and we try to resolve it calling
     * TclWeb_GetEnvIncludeVar, which returns NULL if the variable
     * is undefined */

    return TclWeb_GetEnvIncludeVar(private,key);
}

char *
TclWeb_GetVirtualFile(TclWebRequest *req, char *virtualname)
{
    request_rec *apreq;
    char *filename = NULL;

    apreq = ap_sub_req_lookup_uri( virtualname, req->req, NULL );

    //if( apreq->status == 200 && apreq->finfo.st_mode != 0 ) {
    //TODO: is this the right behaviour?
    if( apreq->status == 200 && apreq->finfo.filetype != APR_NOFILE ) {
        filename = apreq->filename;
    }
    if( apreq != NULL ) ap_destroy_sub_req( apreq );
    return( filename );
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_GetRawPost --
 *
 * 	Fetch the raw POST data from the request.
 *
 * Results:
 *	The data, or NULL if it's not a POST or there is no data.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

char *
TclWeb_GetRawPost ( TclWebRequest *req )
{
    return ApacheRequest_get_raw_post(req->apachereq);
}
