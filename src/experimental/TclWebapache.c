/*
 * TclWeb.c --
 *
 * 	Common API layer.
 *
 * This file contains the Apache-based versions of the functions in
 * TclWeb.h.  They rely on Apache and apreq to perform the underlying
 * operations.
 *
 */

/* Copyright 2002-2014 The Apache Software Foundation

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

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <tcl.h>
#include <sys/types.h>
#include <pwd.h>

#include <httpd.h>
#include <http_request.h>
#include <ap_compat.h>
#include <apr_strings.h>
#include "apache_request.h"
#include "mod_rivet.h"
#include "TclWeb.h"

extern module rivet_module;
extern apr_threadkey_t* rivet_thread_key;
#define TCLWEBPOOL req->req->pool

#define BUFSZ 4096

/* This is used below to determine what part of the parmsarray to
 * parse. */
#define PARMSARRAY_COORDINATES i = 0; j = parmsarray->nelts; \
if (source == VAR_SRC_QUERYSTRING) { j = req->apachereq->nargs; } \
else if (source == VAR_SRC_POST) { i = req->apachereq->nargs; }

/* 
 * -- TclWeb_NewRequestObject
 *
 *
 */

TclWebRequest*
TclWeb_NewRequestObject (apr_pool_t *p)
{
    TclWebRequest*  req = (TclWebRequest *)apr_pcalloc(p, sizeof(TclWebRequest));

    req->interp             = NULL;
    req->req                = NULL;
    req->apachereq          = ApacheRequest_new(p);
    req->headers_printed    = 0;
    req->headers_set        = 0;
    req->environment_set    = 0;
    req->charset            = NULL;  /* we will test against NULL to check if a charset */
                                     /* was specified in the conf */
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
 *  *req:    a pointer to a TclWebRequest object to be intialized
 *  *interp: current Tcl_Interp object serving the request
 *  *arg:    generic pointer. Current implementation passes the
 *           request_rec object pointer 
 *
 */

int
TclWeb_InitRequest(TclWebRequest *req, Tcl_Interp *interp, void *arg)
{
    request_rec *r = (request_rec *)arg;

    req->interp             = interp;
    req->req                = r;
    req->apachereq          = ApacheRequest_init(req->apachereq,r);
    req->headers_printed    = 0;
    req->headers_set        = 0;
    req->environment_set    = 0;


    req->charset = NULL;
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

//    req->req->content_type = (char *) apr_pstrdup(req->req->pool, header);

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

    if (req->headers_set == 0)
    {
        TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);
    }
    
    /*
     * seems that ap_send_http_header is redefined to ; in Apache2.2
     * ap_send_http_header(req->req);
     */
    
    TclWeb_SendHeaders(req);
    /* ap_send_http_header(req->req); */

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
            ap_rputs(ap_escape_html(TCLWEBPOOL, errstr), req->req);
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
    Tcl_SetStringObj(result,
                ap_construct_url(TCLWEBPOOL,filename,req->req),-1);
    return TCL_OK;
}

int
TclWeb_GetVar(Tcl_Obj *result, char *varname, int source, TclWebRequest *req)
{
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;
    int flag = 0;

    PARMSARRAY_COORDINATES;

    /* This isn't real efficient - move to hash table later on... */
    while (i < j)
    {
        char *parmkey = TclWeb_StringToUtf(parms[i].key, req);
        if (!strncmp(varname, parmkey,
                    strlen(varname) < strlen(parmkey) ?
                    strlen(parmkey) : strlen(varname)))
        {
            /* The following makes sure that we get one string,
               with no sub lists. */
            if (flag == 0)
            {
                flag = 1;
                Tcl_SetStringObj(result,
                        TclWeb_StringToUtf(parms[i].val, req), -1);
            } else {
                Tcl_Obj *tmpobj;
                Tcl_Obj *tmpobjv[2];
                tmpobjv[0] = result;
                tmpobjv[1] = TclWeb_StringToUtfToObj(parms[i].val, req);
                tmpobj = Tcl_ConcatObj(2, tmpobjv);
                Tcl_SetStringObj(result, Tcl_GetString(tmpobj), -1);
            }
        }
        i++;
    }

    if (result->length == 0)
    {
	return TCL_ERROR;
    }

    return TCL_OK;
}

int
TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, int source, TclWebRequest *req)
{
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES;

    /* This isn't real efficient - move to hash table later on. */
    while (i < j)
    {

	if (!strncmp(varname, TclWeb_StringToUtf(parms[i].key, req),
		     strlen(varname) < strlen(parms[i].key) ?
		     strlen(parms[i].key) : strlen(varname)))
	{
	    Tcl_ListObjAppendElement(req->interp, result,
				     TclWeb_StringToUtfToObj(parms[i].val, req));
	}
	i++;
    }

    if (result == NULL)
    {
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
TclWeb_GetAllVars(Tcl_Obj *result, int source, TclWebRequest *req)
{
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES;

    while (i < j)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].key, req));
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].val, req));
	i++;
    }

    if (result == NULL)
    {
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
TclWeb_GetVarNames(Tcl_Obj *result, int source, TclWebRequest *req)
{
    int i, j;
    apr_array_header_t *parmsarray = (apr_array_header_t *)
        apr_table_elts(req->apachereq->parms);
    apr_table_entry_t *parms = (apr_table_entry_t *)parmsarray->elts;

    PARMSARRAY_COORDINATES;

    while (i < j)
    {
	Tcl_ListObjAppendElement(req->interp, result,
				 TclWeb_StringToUtfToObj(parms[i].key, req));
	i++;
    }

    if (result == NULL)
    {
	return TCL_ERROR;
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

    PARMSARRAY_COORDINATES;

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

/*
 * Load the Apache environment and CGI variables into the request.  If we
 * have already done so, we don't need to do it again.
 */
static void
TclWeb_InitEnvVars( TclWebRequest *req )
{
    rivet_server_conf *rsc;
    apr_table_t *table = req->req->subprocess_env;
    char *timefmt = DEFAULT_TIME_FORMAT;
    char *t;
    apr_time_t date = req->req->request_time;
#ifndef WIN32
    struct passwd *pw;
#endif /* ndef WIN32 */
    rivet_thread_private* private;

    if( req->environment_set ) return;

    rsc = RIVET_SERVER_CONF( req->req->server->module_config );

    /* Retrieve cgi variables. */
    ap_add_cgi_vars( req->req );
    ap_add_common_vars( req->req );

    /* These were the "include vars"  */

    apr_table_set( table, "DATE_LOCAL",
            ap_ht_time( TCLWEBPOOL, date, timefmt, 0 ) );
    apr_table_set( table, "DATE_GMT",
            ap_ht_time( TCLWEBPOOL, date, timefmt, 1 ) );
    apr_table_set( table, "LAST_MODIFIED",
            ap_ht_time( TCLWEBPOOL, req->req->finfo.mtime, timefmt, 1 ) );
    apr_table_set( table, "DOCUMENT_URI", req->req->uri );
    apr_table_set( table, "DOCUMENT_PATH_INFO", req->req->path_info );

    if ((t = strrchr(req->req->filename, '/'))) {
        apr_table_set( table, "DOCUMENT_NAME", ++t );
    } else {
        apr_table_set( table, "DOCUMENT_NAME", req->req->uri );
    }

    if( req->req->args ) {
        char *arg_copy = (char*) apr_pstrdup(TCLWEBPOOL, req->req->args);
        ap_unescape_url(arg_copy);
        apr_table_set( table, "QUERY_STRING_UNESCAPED",
                ap_escape_shell_cmd( TCLWEBPOOL, arg_copy ) );
    }

#ifndef WIN32
    pw = (struct passwd *) getpwuid(req->req->finfo.user);
    if( pw ) {
        //apr_table_set( table, "USER_NAME",
        //        apr_pstrdup( TCLWEBPOOL, pw->pw_name ) );
    } else {
        apr_table_set( table, "USER_NAME",
                (char*) apr_psprintf( TCLWEBPOOL, "user#%lu",
                    (unsigned long)req->req->finfo.user ) );
    }
#endif

    /* Here we create some variables with Rivet internal information. */

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);
    apr_table_set (table, "RIVET_CACHE_FREE",
            (char*) apr_psprintf (TCLWEBPOOL, "%d",private->interps[rsc->idx]->cache_free));
    apr_table_set (table, "RIVET_CACHE_SIZE",
            (char*) apr_psprintf (TCLWEBPOOL, "%d",private->interps[rsc->idx]->cache_size));

    req->environment_set = 1;
}

int
TclWeb_GetEnvVars(Tcl_Obj *envvar, TclWebRequest *req)
{
    int i;

    apr_array_header_t *env_arr;
    apr_table_entry_t  *env;
    Tcl_Obj *key;
    Tcl_Obj *val;

    TclWeb_InitEnvVars( req );

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
     * Bug 48963 
     */

        Tcl_ObjSetVar2(req->interp, envvar, key, val, 0);

 	Tcl_DecrRefCount(key);
	Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(envvar);

    return TCL_OK;
}

int
TclWeb_GetHeaderVars(Tcl_Obj *headersvar, TclWebRequest *req)
{
    int i;

    apr_array_header_t *hdrs_arr;
    apr_table_entry_t  *hdrs;
    Tcl_Obj *key;
    Tcl_Obj *val;

    TclWeb_InitEnvVars( req );

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

        /* See comment in TclWeb_GetEnvVars concerning Bug 48963*/

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
    tmp = (char*) apr_pstrdup(TCLWEBPOOL, Tcl_DStringValue(&dstr));
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

int TclWeb_UploadChannel(char *varname, Tcl_Channel *chan, TclWebRequest *req)
{
    *chan = Tcl_OpenFileChannel (req->interp, req->upload->tempname, "r", 0);
    if (chan == NULL) {
	return TCL_ERROR;
    } else {
	if (Tcl_SetChannelOption(req->interp, *chan,
			         "-translation", "binary") == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (Tcl_SetChannelOption(req->interp, *chan,
				 "-encoding", "binary") == TCL_ERROR) {
	    return TCL_ERROR;
	}
	Tcl_RegisterChannel (req->interp, *chan);
	return TCL_OK;
    }
}

int TclWeb_UploadTempname(Tcl_Obj *tempname, TclWebRequest *req)
{
    Tcl_SetStringObj(tempname,
		     TclWeb_StringToUtf(req->upload->tempname,
					req), -1);
    return TCL_OK;
}


int TclWeb_UploadSave(char *varname, Tcl_Obj *filename, TclWebRequest *req)
{
	apr_status_t	status;
	status = apr_file_copy(req->upload->tempname ,Tcl_GetString(filename),APR_FILE_SOURCE_PERMS,req->req->pool);
	if ( status == 0 ) {
	    return TCL_OK;
	} else {
		return TCL_ERROR;
	}
}

int TclWeb_UploadData(char *varname, Tcl_Obj *data, TclWebRequest *req)
{
    rivet_server_conf *rsc = NULL;

    rsc  = RIVET_SERVER_CONF( req->req->server->module_config );
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
	Tcl_ReadChars(chan, data, ApacheUpload_size(req->upload), 0);
	if (Tcl_Close(req->interp, chan) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(req->interp,
			 "RivetServerConf UploadFilesToVar is not set", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

int TclWeb_UploadSize(Tcl_Obj *sz, TclWebRequest *req)
{
    Tcl_SetIntObj(sz, ApacheUpload_size(req->upload));
    return TCL_OK;
}

int TclWeb_UploadType(Tcl_Obj *type, TclWebRequest *req)
{
    /* If there is a type, return it, if not, return blank. */
    Tcl_SetStringObj(type, ApacheUpload_type(req->upload)
		     ? (char *)ApacheUpload_type(req->upload) : (char *)"", -1);
    return TCL_OK;
}

int TclWeb_UploadFilename(Tcl_Obj *filename, TclWebRequest *req)
{
    Tcl_SetStringObj(filename,
		     TclWeb_StringToUtf(req->upload->filename,
					req), -1);
    return TCL_OK;
}

int TclWeb_UploadNames(Tcl_Obj *names, TclWebRequest *req)
{
    ApacheUpload *upload;

    upload = ApacheRequest_upload(req->apachereq);
    while (upload)
    {
	Tcl_ListObjAppendElement(
	    req->interp, names,
	    TclWeb_StringToUtfToObj(upload->name,req));
	upload = upload->next;
    }

    return TCL_OK;
}

char *
TclWeb_GetEnvVar( TclWebRequest *req, char *key )
{
    char *val;

    TclWeb_InitEnvVars( req );

    /* Check to see if it's a header variable first. */
    val = (char *)apr_table_get( req->req->headers_in, key );

    if( !val ) {
        val = (char *)apr_table_get( req->req->subprocess_env, key );
    }

    return val;
}

char *
TclWeb_GetVirtualFile( TclWebRequest *req, char *virtualname )
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
