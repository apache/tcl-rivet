/* mod_rivet.c -- The apache module itself, for Apache 1.3. */

/* Copyright 2000-2005 The Apache Software Foundation

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

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Apache includes */
#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_main.h"
#include "util_script.h"
#include "http_conf_globals.h"

/* Tcl includes */
#include <tcl.h>
/* There is code ifdef'ed out below which uses internal
 * declerations. */
/* #include <tclInt.h> */
#include <string.h>

/* Rivet Includes */
#include "mod_rivet.h"
#include "rivet.h"
#include "rivetParser.h"
#include "rivetChannel.h"

module MODULE_VAR_EXPORT rivet_module;

/* This is used *only* in the PanicProc.  Otherwise, don't touch
 * it! */
static request_rec *globalrr;

/* Need some arbitrary non-NULL pointer which can't also be a request_rec */
#define NESTED_INCLUDE_MAGIC	(&rivet_module)

TCL_DECLARE_MUTEX(sendMutex);

static void Rivet_InitTclStuff(server_rec *s, pool *p);
static void Rivet_CopyConfig( rivet_server_conf *oldrsc,
				rivet_server_conf *newrsc);
static int Rivet_SendContent(request_rec *);
static int Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf,
				request_rec *r);

/* Function to be used should we desire to upload files to a variable */

#if 0
int
Rivet_UploadHook(void *ptr, char *buf, int len, ApacheUpload *upload)
{
    Tcl_Interp *interp = ptr;
    static int usenum = 0;
    static int uploaded = 0;

    if (oldptr != upload)
    {
    } else {
    }

    return len;
}
#endif /* 0 */

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ExecuteAndCheck --
 *
 * 	Given a Tcl interpreter, an output buffer Tcl object, and an
 *      Apache request record, evaluate the buffer as Tcl code.
 *
 *      (Since we're evaluating an object, it'll be compiled and the 
 *       compliation cached in the object by Tcl's on-the-fly bytecode 
 *       compiler.)
 *
 *      If there's an error, figure out if it was caused by abort_page
 *      and if so, simply abort the page.  Otherwise, execute the
 *      custom error handler, if defined, else use the default error handler.
 *
 *      If the error handler gets an error, handle that with a primitive,
 *      last-ditch error-in-error-handler error handler.
 *
 * Results:
 *	TCL_OK
 *
 * Side Effects:
 *	The webpage is emitted.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *r)
{
    rivet_server_conf *conf = Rivet_GetConf(r);
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if( Tcl_EvalObjEx(interp, outbuf, 0) == TCL_ERROR ) {
	Tcl_Obj *errscript;
	Tcl_Obj *errorCodeListObj;
	Tcl_Obj *errorCodeElementObj;
	char *errorCodeSubString;

	/* There was an error, see if it's from Rivet and it was caused
	 * by abort_page.
	 */

	errorCodeListObj = Tcl_GetVar2Ex (interp, "errorCode", (char *)NULL, TCL_GLOBAL_ONLY);
	/* errorCode is guaranteed to be set to NONE, but let's make sure
	 * anyway rather than causing a SIGSEGV
	 */
	ap_assert (errorCodeListObj != (Tcl_Obj *)NULL);

	/* dig the first element out of the errorCode list and see if it
	 * says Rivet -- this shouldn't fail either, but let's assert
	 * success so we don't get a SIGSEGV afterwards */
	ap_assert (Tcl_ListObjIndex (interp, errorCodeListObj, 0, &errorCodeElementObj) == TCL_OK);

	/* if the error was thrown by Rivet, see if it's abort_page and,
	 * if so, don't treat it as an error, i.e. don't execute the
	 * installed error handler or the default one, just let the
	 * page emit as normal
	 */
	if (strcmp (Tcl_GetString (errorCodeElementObj), "RIVET") == 0) {

	    /* dig the second element out of the errorCode list, make sure
	     * it succeeds -- it should always
	     */
	    ap_assert (Tcl_ListObjIndex (interp, errorCodeListObj, 1, &errorCodeElementObj) == TCL_OK);

	    errorCodeSubString = Tcl_GetString (errorCodeElementObj);
	    if (strcmp (errorCodeSubString, "ABORTPAGE") == 0) {
		goto good;
	    }
	}

	Tcl_SetVar( interp, "errorOutbuf",
			Tcl_GetStringFromObj( outbuf, NULL ),
			TCL_GLOBAL_ONLY );

	/* If we don't have an error script, use the default error handler. */
	if (conf->rivet_error_script ) {
	    errscript = Tcl_NewStringObj(conf->rivet_error_script, -1);
	} else {
	    errscript = conf->rivet_default_error_script;
	}

	Tcl_IncrRefCount(errscript);
	if (Tcl_EvalObjEx(interp, errscript, 0) == TCL_ERROR) {
	    CONST84 char *errorinfo = Tcl_GetVar( interp, "errorInfo", 0 );
	    TclWeb_PrintError("<b>Rivet ErrorScript failed!</b>", 1,
				    globals->req);
	    TclWeb_PrintError( errorinfo, 0, globals->req );
	}

	/* This shouldn't make the default_error_script go away,
	 * because it gets a Tcl_IncrRefCount when it is created. */
	Tcl_DecrRefCount(errscript);
    }

    /* Make sure to flush the output if buffer_add was the only output */
    good:
    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(*(conf->outchannel));

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ParseExecFile --
 *
 * 	Given a Tcl Web Request structure, a filename, and a toplevel
 *      flag...
 *
 *      If the user scripts have been updated and the page has been
 *      cached, get rid of the cached page.
 *
 *      If the toplevel flag is zero, stat the file for the ctime and
 *      mtime, else get the ctime and mtime from the req structure.
 *
 *      If caching is enabled, see if it's cached, handle caching it,
 *      freeing the cache, etc.
 *
 *      This is a separate function so that it may be called from 'parse' as
 *      well as Rivet_SendContent.
 *
 * Results:
 *	A Tcl result is returned.
 *
 * Side Effects:
 *	The webpage is emitted.
 *
 *-----------------------------------------------------------------------------
 */
int
Rivet_ParseExecFile(TclWebRequest *req, char *filename, int toplevel)
{
    char *hashKey = NULL;
    int isNew = 0;
    int result = 0;

    Tcl_Obj *outbuf = NULL;
    Tcl_HashEntry *entry = NULL;

    time_t ctime;
    time_t mtime;

    rivet_server_conf *rsc;
    Tcl_Interp *interp = req->interp;

    rsc = Rivet_GetConf( req->req );

    /* If the user configuration has indeed been updated, I guess that
       pretty much invalidates anything that might have been
       cached. */

    /* This is all horrendously slow, and means we should *also* be
       doing caching on the modification time of the .htaccess files
       that concern us. FIXME */

    if (rsc->user_scripts_updated && *(rsc->cache_size) != 0) {
	int ct;
	Tcl_HashEntry *delEntry;
	/* Clean out the list. */
	ct = *(rsc->cache_free);
	while (ct < *(rsc->cache_size)) {
	    /* Free the corresponding hash entry. */
	    delEntry = Tcl_FindHashEntry(
		rsc->objCache,
		rsc->objCacheList[ct]);
	    if (delEntry != NULL)
		Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
	    Tcl_DeleteHashEntry(delEntry);

	    free(rsc->objCacheList[ct]);
	    rsc->objCacheList[ct] = NULL;
	    ct ++;
	}
	*(rsc->cache_free) = *(rsc->cache_size);
    }

    /* If toplevel is 0, we are being called from Parse, which means
       we need to get the information about the file ourselves. */
    if (toplevel == 0)
    {
	Tcl_Obj *fnobj;
	Tcl_StatBuf buf;

	fnobj = Tcl_NewStringObj(filename, -1);
	Tcl_IncrRefCount(fnobj);
	if( Tcl_FSStat(fnobj, &buf) < 0 ) return TCL_ERROR;
	Tcl_DecrRefCount(fnobj);
	ctime = buf.st_ctime;
	mtime = buf.st_mtime;
    } else {
	ctime = req->req->finfo.st_ctime;
	mtime = req->req->finfo.st_mtime;
    }

    /* Look for the script's compiled version.  If it's not found,
     * create it.
     */
    if (*(rsc->cache_size))
    {
	hashKey = ap_psprintf(req->req->pool, "%s%lx%lx%d", filename,
			      mtime, ctime, toplevel);
	entry = Tcl_CreateHashEntry(rsc->objCache, hashKey, &isNew);
    }

    /* We don't have a compiled version.  Let's create one. */
    if (isNew || *(rsc->cache_size) == 0)
    {
	outbuf = Tcl_NewObj();
	Tcl_IncrRefCount(outbuf);

	if (toplevel) {
	    if (rsc->rivet_before_script) {
		Tcl_AppendObjToObj(outbuf, Tcl_NewStringObj(rsc->rivet_before_script, -1));
	    }
	}

	if( STREQU( req->req->content_type, "application/x-httpd-rivet")
	    || !toplevel )
	{
	    /* toplevel == 0 means we are being called from the parse
	     * command, which only works on Rivet .rvt files. */
	    result = Rivet_GetRivetFile(filename, toplevel, outbuf, interp);
	} else {
	    /* It's a plain Tcl file */
	    result = Rivet_GetTclFile(filename, outbuf, interp);
	}

	if (result != TCL_OK)
	{
	    Tcl_DecrRefCount(outbuf);
	    return result;
	}
	if (toplevel) {
	    if (rsc->rivet_after_script) {
		Tcl_AppendObjToObj(outbuf,
				   Tcl_NewStringObj(rsc->rivet_after_script, -1));
	    }
	}

	if (*(rsc->cache_size)) {
	    /* We need to incr the reference count of outbuf because we want
	     * it to outlive this function.  This allows it to stay alive
	     * as long as it's in the object cache.
	     */
	    Tcl_IncrRefCount( outbuf );
	    Tcl_SetHashValue(entry, (ClientData)outbuf);
	}

	if (*(rsc->cache_free)) {
	    rsc->objCacheList[-- *(rsc->cache_free) ] = strdup(hashKey);
	} else if (*(rsc->cache_size)) { /* If it's zero, we just skip this. */
	    Tcl_HashEntry *delEntry;
	    delEntry = Tcl_FindHashEntry(
		rsc->objCache,
		rsc->objCacheList[*(rsc->cache_size) - 1]);
	    Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
	    Tcl_DeleteHashEntry(delEntry);
	    free(rsc->objCacheList[*(rsc->cache_size) - 1]);
	    memmove((rsc->objCacheList) + 1, rsc->objCacheList,
		    sizeof(char *) * (*(rsc->cache_size) -1));
	    rsc->objCacheList[0] = strdup(hashKey);
	}
    } else {
	/* We found a compiled version of this page. */
	outbuf = (Tcl_Obj *)Tcl_GetHashValue(entry);
	Tcl_IncrRefCount(outbuf);
    }
    rsc->user_scripts_updated = 0;
    {
	int res = 0;
	res = Rivet_ExecuteAndCheck(interp, outbuf, req->req);
	Tcl_DecrRefCount(outbuf);
	return res;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CleanupRequest --
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */
static void
Rivet_CleanupRequest( request_rec *r )
{
/* FIXME why is this all ifdef'ed out? */
#if 0
    table *t;
    array_header *arr;
    table_entry  *elts;
    int i, nelts;
    Tcl_Obj *arrayName;
    Tcl_Interp *interp;

    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->per_dir_config );

    t = rsc->rivet_user_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetUserConf", -1 );
    interp = rsc->server_interp;

    for( i = 0; i < nelts; ++i )
    {
	Tcl_UnsetVar2(interp,
		      "RivetUserConf",
		      elts[i].key,
		      TCL_GLOBAL_ONLY);
    }
    Tcl_DecrRefCount(arrayName);

    rivet_server_conf *rdc = RIVET_SERVER_CONF( r->per_dir_config );

    if( rdc->rivet_before_script ) {
	Tcl_DecrRefCount( rdc->rivet_before_script );
    }
    if( rdc->rivet_after_script ) {
	Tcl_DecrRefCount( rdc->rivet_after_script );
    }
    if( rdc->rivet_error_script ) {
	Tcl_DecrRefCount( rdc->rivet_error_script );
    }
#endif
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PropagatePerDirConfArrays --
 *
 * 	Propagate all of the Rivet DirConf variables and Rivet
 *      UserConf variables defined in the Apache config 
 *      (typically httpd.conf) into an array.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A global RivetServerConf array is populated with ServerConf
 *      variables and values.
 *
 *	A global RivetUserConf array is populated with UserConf
 *      variables and values.
 *
 *-----------------------------------------------------------------------------
 */
static void
Rivet_PropagatePerDirConfArrays( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    table *t;
    array_header *arr;
    table_entry  *elts;
    int i, nelts;
    Tcl_Obj *arrayName;
    Tcl_Obj *key;
    Tcl_Obj *val;

    /* Make sure RivetDirConf doesn't exist from a previous request. */

    Tcl_UnsetVar( interp, "RivetDirConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the DirConf variables into an array. */

    t = rsc->rivet_dir_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetDirConf", -1 );
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
	key = Tcl_NewStringObj( elts[i].key, -1);
	val = Tcl_NewStringObj( elts[i].val, -1);
	Tcl_IncrRefCount(key);
	Tcl_IncrRefCount(val);
	Tcl_ObjSetVar2(interp,
		       arrayName,
		       key,
		       val,
 		       TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(key);
	Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);

    /* Make sure RivetUserConf doesn't exist from a previous request. */

    Tcl_UnsetVar( interp, "RivetUserConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the UserConf variables into an array. */

    t = rsc->rivet_user_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;
    arrayName = Tcl_NewStringObj( "RivetUserConf", -1 );
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
	key = Tcl_NewStringObj( elts[i].key, -1);
	val = Tcl_NewStringObj( elts[i].val, -1);
	Tcl_IncrRefCount(key);
	Tcl_IncrRefCount(val);
 	Tcl_ObjSetVar2(interp,
		       arrayName,
		       key,
		       val,
		       TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(key);
	Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);
}

/* Set things up to execute a file, then execute */
/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_SendContent --
 *
 * 	Process and send a page, whether x-httpd-rivet style or
 *      x-rivet-tcl style.
 *
 *      Validate that they're allowed to execute us and that the file
 *      exists.
 *
 *      Generate the Rivet server conf by merging per directory config
 *      stuff appropriate to this page.
 *
 *
 * Results:
 *	An Apache webserver result code such as OK, HTTP_BAD_REQUEST, 
 *      HTTP_NOT_FOUND, etc, is returned.
 *
 * Side Effects:
 *      A webpage or webpage header or some kind of error is emitted.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_SendContent(request_rec *r)
{
    char error[MAX_STRING_LEN];
    char timefmt[MAX_STRING_LEN];
    int errstatus;
    int retval;

    Tcl_Interp	*interp;
    static Tcl_Obj	*request_init = NULL;
    static Tcl_Obj	*request_cleanup = NULL;

    rivet_interp_globals *globals = NULL;
    rivet_server_conf *rsc = NULL;
    rivet_server_conf *rdc;

    Tcl_MutexLock(&sendMutex);

    /* Set the global request req to know what we are dealing with in
     * case we have to call the PanicProc. */
    globalrr = r;

    rsc = Rivet_GetConf(r);
    interp = rsc->server_interp;
    globals = Tcl_GetAssocData(interp, "rivet", NULL);
    globals->r = r;
    globals->req = (TclWebRequest *)ap_pcalloc(r->pool, sizeof(TclWebRequest));

    rdc = RIVET_SERVER_CONF( r->per_dir_config );

    r->allowed |= (1 << M_GET);
    r->allowed |= (1 << M_POST);
    if (r->method_number != M_GET && r->method_number != M_POST) {
	retval = DECLINED;
	goto sendcleanup;
    }

    if (r->finfo.st_mode == 0)
    {
	ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r->server,
		     "File does not exist: %s",
		     (r->path_info
		      ? ap_pstrcat(r->pool, r->filename, r->path_info, NULL)
		      : r->filename));
	retval = HTTP_NOT_FOUND;
	goto sendcleanup;
    }

    if ((errstatus = ap_meets_conditions(r)) != OK) {
	retval = errstatus;
	goto sendcleanup;
    }

    ap_cpystrn(error, DEFAULT_ERROR_MSG, sizeof(error));
    ap_cpystrn(timefmt, DEFAULT_TIME_FORMAT, sizeof(timefmt));

    /* This one is the big catch when it comes to moving towards
       Apache 2.0, or one of them, at least. */
    ap_chdir_file(r->filename);

    /* Generate the Rivet server conf by merging per directory config
     * stuff appropriate to this page. */
    Rivet_PropagatePerDirConfArrays( interp, rdc );

    /* Initialize this the first time through and keep it around. */
    if (request_init == NULL) {
	request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
	Tcl_IncrRefCount(request_init);
    }

    /* prep the special ::request namespace by executing the 
     * initialize_request proc from the Rivet package */
    if (Tcl_EvalObjEx(interp, request_init, 0) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
			"Could not create request namespace\n");
	retval = HTTP_BAD_REQUEST;
	goto sendcleanup;
    }

    /* Set the script name. */
    {
#if 1
       Tcl_Obj *infoscript = Tcl_NewStringObj("info script ", -1);
       Tcl_IncrRefCount(infoscript);
       Tcl_AppendToObj(infoscript, r->filename, -1);
       Tcl_EvalObjEx(interp, infoscript, TCL_EVAL_DIRECT);
       Tcl_DecrRefCount(infoscript);
#else
       /* This speeds things up, but you have to use Tcl internal
	* declarations, which is not so great... */
	Interp *iPtr = (Interp *) interp;
	if (iPtr->scriptFile != NULL) {
	    Tcl_DecrRefCount(iPtr->scriptFile);
	}
	iPtr->scriptFile = Tcl_NewStringObj(r->filename, -1);
	Tcl_IncrRefCount(iPtr->scriptFile);
#endif
    }

    /* Apache Request stuff */
    TclWeb_InitRequest(globals->req, interp, r);
    ApacheRequest_set_post_max(globals->req->apachereq, rsc->upload_max);
    ApacheRequest_set_temp_dir(globals->req->apachereq, rsc->upload_dir);

#if 0
    if (upload_files_to_var)
    {
	globals->req->apachereq->hook_data = interp;
	globals->req->apachereq->upload_hook = Rivet_UploadHook;
    }
#endif

    errstatus = ApacheRequest_parse(globals->req->apachereq);
    if (errstatus != OK) {
	retval = errstatus;
	goto sendcleanup;
    }

    if (r->header_only)
    {
	TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, globals->req);
	TclWeb_PrintHeaders(globals->req);
	retval = OK;
	goto sendcleanup;
    }

    if (Rivet_ParseExecFile(globals->req, r->filename, 1) != TCL_OK)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "%s",
		     Tcl_GetVar(interp, "errorInfo", 0));
    }

    if (request_cleanup == NULL) {
	request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
	Tcl_IncrRefCount(request_cleanup);
    }
    if(Tcl_EvalObjEx(interp, request_cleanup, 0) == TCL_ERROR) {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "%s",
		     Tcl_GetVar(interp, "errorInfo", 0));
    }

    /* Reset globals */
    Rivet_CleanupRequest( r );

    retval = OK;
sendcleanup:
    globals->req->content_sent = 0;

    Tcl_MutexUnlock(&sendMutex);

    return retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitServerVariables --
 *
 * 	Setup an array in each interpreter to tell us things about Apache.
 *      This saves us from having to do any real call to load an entire
 *      environment.
 *
 *      This routine gets called once, when the child process is created.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *	A global server array is populated with the following variables:
 *
 *         SERVER_ROOT - Apache's root location
 *         SERVER_CONF - Apache's configuration file
 *         RIVET_DIR   - Rivet's Tcl source directory
 *         RIVET_INIT  - Rivet's init.tcl file
 *-----------------------------------------------------------------------------
 */

static void
Rivet_InitServerVariables( Tcl_Interp *interp, pool *p )
{
    Tcl_Obj *obj;

    /* Create server(SERVER_ROOT) from Apache's ap_server_root */
    obj = Tcl_NewStringObj(ap_server_root, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		  "server",
		  "SERVER_ROOT",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    /* Create server(SERVER_CONF) from Apache's ap_server_confname */
    obj = Tcl_NewStringObj(ap_server_root_relative(p, ap_server_confname), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		 "server",
		 "SERVER_CONF",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    /* Create server(RIVET_DIR) */
    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_DIR), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		  "server",
		  "RIVET_DIR",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    /* Create server(RIVET_INIT) */
    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_INIT), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		 "server",
		 "RIVET_INIT",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PropagateServerConfArray --
 *
 * 	Propagate all of the Rivet ServerConf variables defined in the
 *      Apache config (typically httpd.conf) into an array.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A global RivetServerConf array is populated with ServerConf
 *      variables and values.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_PropagateServerConfArray( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    table *t;
    array_header *arr;
    table_entry  *elts;
    int i, nelts;
    Tcl_Obj *key;
    Tcl_Obj *val;
    Tcl_Obj *arrayName;

    /* Propagate all of the ServerConf variables into an array. */
    t = rsc->rivet_server_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;

    arrayName = Tcl_NewStringObj("RivetServerConf", -1);
    Tcl_IncrRefCount(arrayName);

    for( i = 0; i < nelts; ++i )
    {
	key = Tcl_NewStringObj( elts[i].key, -1);
	val = Tcl_NewStringObj( elts[i].val, -1);
	Tcl_IncrRefCount(key);
	Tcl_IncrRefCount(val);
 	Tcl_ObjSetVar2(interp,
		       arrayName,
		       key,
		       val,
		       TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(key);
	Tcl_DecrRefCount(val);
    }
    Tcl_DecrRefCount(arrayName);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PerInterpInit --
 *
 * 	Do the initialization that needs to happen for every
 * 	interpreter.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *      A Tcl channel handler is defined to allow Tcl "puts" to stdout
 *      to cause its output to be folded into the webpage being generated.
 *
 *      Core Rivet commands such as header, load_env, var, etc, are
 *      added to the interpreter.
 *
 *      The global "server" array is populated.
 *
 *      The RivetServerConf array is populated with Rivet ServerConf
 *      data from the Apache config file(s).
 *
 *      A rivet_interp_globals area is set up and made available as
 *      associated data of the Tcl interpreter.
 *
 *      A "package require rivet" is performed to load the Rivet package,
 *      effectively causing Rivet's init.tcl to be loaded, defining all
 *      Tcl-based procs that are part of Rivet.
 *
 *      The Tcl output buffer size is set to the largest allowed value to
 *      keep any results from getting output unless the Rivet page does
 *      a flush on stdout.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_PerInterpInit(server_rec *s, rivet_server_conf *rsc, pool *p)
{
    Tcl_Interp *interp = rsc->server_interp;
    rivet_interp_globals *globals = NULL;

    /* Create TCL commands to deal with Apache's BUFFs. */
    rsc->outchannel = ap_pcalloc(p, sizeof(Tcl_Channel));
    *(rsc->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rsc,
					   TCL_WRITABLE);

    Tcl_SetStdChannel(*(rsc->outchannel), TCL_STDOUT);

    /* Add Rivet's core commands to the interpreter. */
    Rivet_InitCore( interp );

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables( interp, p );
    Rivet_PropagateServerConfArray( interp, rsc );

    /* Set up interpreter-associated data -- this makes it possible for
     * us to locate rivet_interp_globals given an interpreter pointer */
    globals = ap_pcalloc(p, sizeof(rivet_interp_globals));
    Tcl_SetAssocData(interp, "rivet", NULL, globals);

    /* Require the RivetTcl package, causing Rivet's init.tcl file to load in 
     * Rivet's Tcl-level commands. */
    if (Tcl_PkgRequire(interp, "RivetTcl", "1.1", 1) == NULL) {
	ap_log_error( APLOG_MARK, APLOG_ERR, s,
		      "init.tcl must be installed correctly for Apache Rivet to function: %s",
		      Tcl_GetStringResult(interp) );
	exit(1);
    }

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */
    Tcl_SetChannelOption(interp, *(rsc->outchannel), "-buffersize", "1000000");
    Tcl_RegisterChannel(interp, *(rsc->outchannel));
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PanicProc --
 *
 * 	Called when Tcl panics, usually because of memory problems.
 * 	We log the request, in order to be able to determine what went
 * 	wrong later.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Calls abort(), which does not return - the child exits.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_Panic TCL_VARARGS_DEF(CONST char *, arg1)
{
    va_list argList;
    char *buf;
    char *format;

    format = TCL_VARARGS_START(char *,arg1,argList);
    buf = ap_pvsprintf(globalrr->pool, format, argList);
    ap_log_error(APLOG_MARK, APLOG_CRIT, globalrr->server,
		 "Critical error in request: %s", globalrr->unparsed_uri);
    ap_log_error(APLOG_MARK, APLOG_CRIT, globalrr->server, buf);

    abort();
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitTclStuff --
 *
 * 	Initialize the Tcl system - create interpreters, load commands
 * 	and so forth.
 *
 *      This is called whenever an individual Apache child process is
 *      created.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A Tcl interpreter is created and initialized.
 *
 *      A custom Rivet Tcl panic proc is set up.
 *
 *      Rivet_PerInterpInit is performed, creating the Tcl stdout output
 *      handler, adding core Rivet commands to the interpreter, etc.
 *      (See Rivet_PerInterpInit for details.)
 *
 *      Space for the cache of compiled Rivet pages is allocated.
 *
 *      The global Rivet Tcl init script is executed, if defined.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_InitTclStuff(server_rec *s, pool *p)
{
    Tcl_Interp *interp;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    rivet_server_conf *myrsc;
    server_rec *sr;

    /* Initialize TCL stuff  */
    Tcl_FindExecutable(NAMEOFEXECUTABLE);
    interp = Tcl_CreateInterp();

    if (interp == NULL)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, s,
		"Error in Tcl_CreateInterp, aborting\n");
	exit(1);
    }
    if (Tcl_Init(interp) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, s, Tcl_GetStringResult(interp));
	exit(1);
    }

    Tcl_SetPanicProc(Rivet_Panic);

    rsc->server_interp = interp; /* root interpreter */

    Rivet_PerInterpInit(s, rsc, p);

    /* If the user didn't set a cache size in their configuration, we
     * will assume an arbitrary size for them.
     *
     * If the cache size is 0, the user has requested not to cache
     * documents.
     */
    if(*(rsc->cache_size) < 0) {
	if (ap_max_requests_per_child != 0) {
	    *(rsc->cache_size) = ap_max_requests_per_child / 5;
	} else {
	    *(rsc->cache_size) = 10; /* FIXME: Arbitrary number */
	}
    }

    if (*(rsc->cache_size) != 0) {
	*(rsc->cache_free) = *(rsc->cache_size);
    }

    /* Initialize cache structures */
    if (*(rsc->cache_size)) {
	rsc->objCacheList = ap_pcalloc(
	    p, (signed)(*(rsc->cache_size) * sizeof(char *)));
	rsc->objCache = ap_pcalloc(p, sizeof(Tcl_HashTable));
	Tcl_InitHashTable(rsc->objCache, TCL_STRING_KEYS);
    }

    /* Execute the global Rivet Tcl init script, if defined */
    if (rsc->rivet_global_init_script != NULL) {
	if (Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0) != TCL_OK)
	{
	    ap_log_error(APLOG_MARK, APLOG_ERR, s, "%s",
			 Tcl_GetVar(interp, "errorInfo", 0));
	}
    }

    /* for all of the server_recs */
    for (sr = s; sr; sr = sr->next)
    {
	myrsc = RIVET_SERVER_CONF(sr->module_config);

	/* We only have a different rivet_server_conf if MergeConfig
	 * was called. We really need a separate one for each server,
	 * so we go ahead and create one here, if necessary. */

	/* if it's not the first one and it has the same module config
	 * as the first one, clone the module config
	 */
	if (sr != s && myrsc == rsc) {
 	    myrsc = RIVET_NEW_CONF(p);
	    ap_set_module_config(sr->module_config, &rivet_module, myrsc);
	    Rivet_CopyConfig( rsc, myrsc );
	}

	myrsc->outchannel = rsc->outchannel;
	/* This sets up slave interpreters for other virtual hosts. */
	if (sr != s) /* not the first one  */
	{
	    if (rsc->separate_virtual_interps != 0) {
		char *slavename = ap_psprintf(p, "%s_%d", 
					      sr->server_hostname, 
					      sr->port);
		/* Separate virtual interps. */
		myrsc->server_interp = Tcl_CreateSlave(interp,
						       slavename, 0);
		Rivet_PerInterpInit(s, myrsc, p);
	    } else {
		myrsc->server_interp = rsc->server_interp;
	    }

	    /* Since these things are global, we copy them into the
	     * rivet_server_conf struct. */
	    myrsc->cache_size = rsc->cache_size;
	    myrsc->cache_free = rsc->cache_free;
	    myrsc->objCache = rsc->objCache;
	    myrsc->objCacheList = rsc->objCacheList;
	}
	myrsc->server_name = ap_pstrdup(p, sr->server_hostname);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_SetScript --
 *
 *	Given a script name from a Rivet Apache directive, such as UserConf,
 *      ChildInitScript, etc, and a corresponding string, create or add
 *      the string to the corresponding variable in the rivet_server_conf 
 *      structure.
 *
 *      In the cases of GlobalInitScript, ChildInitScript and ChildExitScript,
 *      if multiple entries are defined, they are appended together, separated
 *      by newlines, and stored as Tcl objects.
 *
 *      For the cases of BeforeScript, AfterScript, and ErrorScript, the
 *      new values overrides the old value, if any was previously defined.
 *      These are stored only as strings, not Tcl objects, as they are
 *      massaged into the text that is ultimately evaluated to produce the
 *      webpage output.
 *
 * Results:
 *	Returns the string representation of the current value for the
 *	directive.
 *
 * Side Effects:
 *	An element of the passed rivet_server_conf structure is
 *      created or appended to.
 *
 *----------------------------------------------------------------------
 */

static char *
Rivet_SetScript( ap_pool *pool, rivet_server_conf *rsc, char *script, char *string )
{
    Tcl_Obj *objarg = NULL;

    if( STREQU( script, "GlobalInitScript" ) ) {
	if( rsc->rivet_global_init_script == NULL ) {
	    objarg = Tcl_NewStringObj( string, -1 );
	    Tcl_IncrRefCount( objarg );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	    rsc->rivet_global_init_script = objarg;
	} else {
	    objarg = rsc->rivet_global_init_script;
	    Tcl_AppendToObj( objarg, string, -1 );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	}
    } else if( STREQU( script, "ChildInitScript" ) ) {
	if( rsc->rivet_child_init_script == NULL ) {
	    objarg = Tcl_NewStringObj( string, -1 );
	    Tcl_IncrRefCount( objarg );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	    rsc->rivet_child_init_script = objarg;
	} else {
	    objarg = rsc->rivet_child_init_script;
	    Tcl_AppendToObj( objarg, string, -1 );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	}
    } else if( STREQU( script, "ChildExitScript" ) ) {
	if( rsc->rivet_child_exit_script == NULL ) {
	    objarg = Tcl_NewStringObj( string, -1 );
	    Tcl_IncrRefCount( objarg );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	    rsc->rivet_child_exit_script = objarg;
	} else {
	    objarg = rsc->rivet_child_exit_script;
	    Tcl_AppendToObj( objarg, string, -1 );
	    Tcl_AppendToObj( objarg, "\n", 1 );
	}
    } else if( STREQU( script, "BeforeScript" ) ) {
	rsc->rivet_before_script = ap_pstrcat(pool, string, "\n", NULL);
    } else if( STREQU( script, "AfterScript" ) ) {
	rsc->rivet_after_script = ap_pstrcat(pool, string, "\n", NULL);
    } else if( STREQU( script, "ErrorScript" ) ) {
	rsc->rivet_error_script = ap_pstrcat(pool, string, "\n", NULL);
    }

    if( !objarg ) return string;

    return Tcl_GetStringFromObj( objarg, NULL );
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_ServerConf --
 *
 *      Implements the RivetServerConf Apache Directive
 *
 *      Command Arguments:
 *
 *	     RivetServerConf GlobalInitScript <script>
 * 	     RivetServerConf ChildInitScript <script>
 * 	     RivetServerConf ChildExitScript <script>
 * 	     RivetServerConf BeforeScript <script>
 * 	     RivetServerConf AfterScript <script>
 * 	     RivetServerConf ErrorScript <script>
 * 	     RivetServerConf CacheSize <integer>
 * 	     RivetServerConf UploadDirectory <directory>
 * 	     RivetServerConf UploadMaxSize <integer>
 * 	     RivetServerConf UploadFilesToVar <yes|no>
 * 	     RivetServerConf SeparateVirtualInterps <yes|no>
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A field in the rivet_server_conf table gets set.
 *
 *----------------------------------------------------------------------
 */

static const char *
Rivet_ServerConf( cmd_parms *cmd, void *dummy, char *var, char *val )
{
    server_rec *s = cmd->server;
    rivet_server_conf *rsc = RIVET_SERVER_CONF(s->module_config);
    char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetServerConf requires two arguments";
    }

    if( STREQU( var, "CacheSize" ) ) {
	*(rsc->cache_size) = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadDirectory" ) ) {
	rsc->upload_dir = val;
    } else if( STREQU( var, "UploadMaxSize" ) ) {
	rsc->upload_max = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadFilesToVar" ) ) {
	Tcl_GetBoolean (NULL, val, &rsc->upload_files_to_var);
    } else if( STREQU( var, "SeparateVirtualInterps" ) ) {
	Tcl_GetBoolean (NULL, val, &rsc->separate_virtual_interps);
    } else {
	string = Rivet_SetScript( cmd->pool, rsc, var, val);
    }

    ap_table_set( rsc->rivet_server_vars, var, string );
    return( NULL );
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_DirConf --
 *
 *      Implements the RivetDirConf Apache Directive
 *
 *      Command Arguments:
 *
 * 	     RivetDirConf BeforeScript <script>
 * 	     RivetDirConf AfterScript <script>
 * 	     RivetDirConf ErrorScript <script>
 * 	     RivetDirConf UploadDirectory <directory>
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A field in the rivet_dir_vars table in the rivet_server_conf
 *      structure gets set.
 *
 *----------------------------------------------------------------------
 */
static const char *
Rivet_DirConf( cmd_parms *cmd, rivet_server_conf *rdc, char *var, char *val )
{
    char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetDirConf requires two arguments";
    }

    if( STREQU( var, "UploadDirectory" ) ) {
	rdc->upload_dir = val;
    } else {
	string = Rivet_SetScript( cmd->pool, rdc, var, val );
    }

    ap_table_set( rdc->rivet_dir_vars, var, string );
    return( NULL );
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_UserConf --
 *
 *      Implements the RivetUserConf Apache Directive
 *
 *      Command Arguments:
 * 	     RivetUserConf BeforeScript <script>
 * 	     RivetUserConf AfterScript <script>
 * 	     RivetUserConf ErrorScript <script>
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A field in the rivet_user_vars table in the rivet_server_conf
 *      structure gets set.
 *
 *----------------------------------------------------------------------
 */
static const char *
Rivet_UserConf( cmd_parms *cmd, rivet_server_conf *rdc, char *var, char *val )
{
    char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetUserConf requires two arguments";
    }
    /* We have modified these scripts. */
    /* This is less than ideal though, because it will get set to 1
     * every time - FIXME. */
    rdc->user_scripts_updated = 1;

    string = Rivet_SetScript( cmd->pool, rdc, var, val );
    /* XXX Need to figure out what to do about setting the table.  */
    ap_table_set( rdc->rivet_user_vars, var, string );
    return( NULL );
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_MergeDirConfigVars --
 *
 *      Given a base Rivet server config structure, a new Rivet server
 *      config structure, and a Rivet server config structure to add,
 *      merge the base and add structures to produce the new structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If there is an "add" BeforeScript, use that, otherwise use
 *      the base one.  Same for AfterScript, ErrorScript, UploadDir,
 *      etc.
 *
 *----------------------------------------------------------------------
 */
static void
Rivet_MergeDirConfigVars( pool *p, rivet_server_conf *new,
			  rivet_server_conf *base, rivet_server_conf *add )
{
    FILEDEBUGINFO;

    new->rivet_before_script = add->rivet_before_script ?
	add->rivet_before_script : base->rivet_before_script;
    new->rivet_after_script = add->rivet_after_script ?
	add->rivet_after_script : base->rivet_after_script;
    new->rivet_error_script = add->rivet_error_script ?
	add->rivet_error_script : base->rivet_error_script;

    new->user_scripts_updated = add->user_scripts_updated ?
	add->user_scripts_updated : base->user_scripts_updated;

    new->upload_dir = add->upload_dir ?
	add->upload_dir : base->upload_dir;

    /* Merge the tables of dir and user variables. */
    if (base->rivet_dir_vars && add->rivet_dir_vars) {
	new->rivet_dir_vars =
	    ap_overlay_tables( p, base->rivet_dir_vars, add->rivet_dir_vars );
    } else {
	new->rivet_dir_vars = base->rivet_dir_vars;
    }
    if (base->rivet_user_vars && add->rivet_user_vars) {
   	new->rivet_user_vars =
	    ap_overlay_tables( p, base->rivet_user_vars, add->rivet_user_vars );
    } else {
	new->rivet_user_vars = base->rivet_user_vars;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rivet_GetConf --
 *
 *      Given an Apache request record, dig the Rivet server config structure 
 *      out of the specified Apache request_rec structure.
 *
 *      If there is no per directory config, just return the server
 *      config.  Otherwise make a copy of the config and merge the 
 *      per-directory config vars.
 *
 * Results:
 *	A Rivet server conf structure set up appropriately for the
 *      specified Apache request.
 *
 * Side Effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
/* Function to get a config and merge the directory/server options  */
rivet_server_conf *
Rivet_GetConf( request_rec *r )
{
    rivet_server_conf *newconfig = NULL;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->server->module_config );
    rivet_server_conf *rdc;
    void *dconf = r->per_dir_config;

    FILEDEBUGINFO;

    /* If there is no per dir config, just return the server config */
    if (dconf == NULL) {
	return rsc;
    }

    rdc = RIVET_SERVER_CONF( dconf );

    newconfig = RIVET_NEW_CONF( r->pool );

    Rivet_CopyConfig( rsc, newconfig );

    Rivet_MergeDirConfigVars( r->pool, newconfig, rsc, rdc );

    return newconfig;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CopyConfig --
 *
 * 	Copy the contents of a rivet_server_conf struct into another one.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *      The new Rivet server conf is populated with data from the old one.
 *
 *-----------------------------------------------------------------------------
 */

static void
Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
{
    FILEDEBUGINFO;

    newrsc->server_interp = oldrsc->server_interp;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;

    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;

    newrsc->user_scripts_updated = oldrsc->user_scripts_updated;

    newrsc->rivet_default_error_script = oldrsc->rivet_default_error_script;

    /* These are pointers so that they can be passed around... */
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->upload_max = oldrsc->upload_max;
    newrsc->upload_files_to_var = oldrsc->upload_files_to_var;
    newrsc->separate_virtual_interps = oldrsc->separate_virtual_interps;
    newrsc->server_name = oldrsc->server_name;
    newrsc->upload_dir = oldrsc->upload_dir;
    newrsc->rivet_server_vars = oldrsc->rivet_server_vars;
    newrsc->rivet_dir_vars = oldrsc->rivet_dir_vars;
    newrsc->rivet_user_vars = oldrsc->rivet_user_vars;
    newrsc->objCacheList = oldrsc->objCacheList;
    newrsc->objCache = oldrsc->objCache;

    newrsc->outchannel = oldrsc->outchannel;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CreateConfig --
 *
 * 	Given an Apache pool and an Apache server record, create a new
 *      Rivet server config and initialize it.
 *
 * Results:
 *	The new Rivet server config is returned.
 *
 * Side Effects:
 *      The Rivet server config is allocated as part of the specified pool.
 *
 *-----------------------------------------------------------------------------
 */

static void *
Rivet_CreateConfig( pool *p, server_rec *s )
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    rsc->server_interp = NULL;
    rsc->rivet_global_init_script = NULL;
    rsc->rivet_child_init_script = NULL;
    rsc->rivet_child_exit_script = NULL;
    rsc->rivet_before_script = NULL;
    rsc->rivet_after_script = NULL;
    rsc->rivet_error_script = NULL;

    rsc->user_scripts_updated = 0;

    rsc->rivet_default_error_script = Tcl_NewStringObj("::Rivet::handle_error", -1);
    Tcl_IncrRefCount(rsc->rivet_default_error_script);

    /* these are pointers so that they can be passed around...  */
    rsc->cache_size = ap_pcalloc(p, sizeof(int));
    rsc->cache_free = ap_pcalloc(p, sizeof(int));
    *(rsc->cache_size) = -1;
    *(rsc->cache_free) = 0;
    rsc->upload_max = 0;
    rsc->upload_files_to_var = 0;
    rsc->separate_virtual_interps = 0;
    rsc->server_name = NULL;
    rsc->upload_dir = "/tmp";
    rsc->objCacheList = NULL;
    rsc->objCache = NULL;

    rsc->outchannel = NULL;

    rsc->rivet_server_vars = ap_make_table( p, 4 );
    rsc->rivet_dir_vars = ap_make_table( p, 4 );
    rsc->rivet_user_vars = ap_make_table( p, 4 );

    return rsc;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CreateDirConfig --
 *
 * 	Given an Apache pool and dir string, generate a Rivet server
 *      conf structure and create the server vars, dir vars and user
 *      vars tables within it.
 *
 *      FIXME "dir" isn't used?
 *
 * Results:
 *	The new Rivet server config is returned.
 *
 * Side Effects:
 *      The Rivet server config is allocated as part of the specified pool.
 *
 *-----------------------------------------------------------------------------
 */

void *
Rivet_CreateDirConfig(pool *p, char *dir)
{
    rivet_server_conf *rdc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    rdc->rivet_server_vars = ap_make_table( p, 4 );
    rdc->rivet_dir_vars = ap_make_table( p, 4 );
    rdc->rivet_user_vars = ap_make_table( p, 4 );

    return rdc;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_MergeDirConfig --
 *
 * 	Given an Apache pool, a base Rivet server conf and an add Rivet
 *      server conf, allocate a new Rivet server and merge the directory 
 *      config vars from the base and add configs into the new config.
 *
 * Results:
 *	The new Rivet server config containing merged dir configs is returned.
 *
 * Side Effects:
 *      The new Rivet server config is allocated as part of the specified pool.
 *
 *-----------------------------------------------------------------------------
 */

void *
Rivet_MergeDirConfig( pool *p, void *basev, void *addv )
{
    rivet_server_conf *base = (rivet_server_conf *)basev;
    rivet_server_conf *add  = (rivet_server_conf *)addv;
    rivet_server_conf *new  = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    Rivet_MergeDirConfigVars( p, new, base, add );

    return new;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_MergeConfig --
 *
 * 	This function is called when there is a config option set both
 * 	at the 'global' level, and for a virtual host.  It "resolves
 * 	the conflicts" so to speak, by creating a new configuration,
 * 	and this function is where we get to have our say about how to
 * 	go about doing that.  For most of the options, we override the
 * 	global option with the local one.
 *
 * Results:
 *	Returns a new server configuration.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

void *
Rivet_MergeConfig(pool *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);
    rivet_server_conf *base = (rivet_server_conf *) basev;
    rivet_server_conf *overrides = (rivet_server_conf *) overridesv;

    FILEDEBUGINFO;

    /* For completeness' sake, we list the fate of all the members of
     * the rivet_server_conf struct. */

    /* server_interp isn't set at this point. */
    /* rivet_global_init_script is global, not per server. */

    rsc->rivet_child_init_script = overrides->rivet_child_init_script ?
	overrides->rivet_child_init_script : base->rivet_child_init_script;

    rsc->rivet_child_exit_script = overrides->rivet_child_exit_script ?
	overrides->rivet_child_exit_script : base->rivet_child_exit_script;

    rsc->rivet_before_script = overrides->rivet_before_script ?
	overrides->rivet_before_script : base->rivet_before_script;

    rsc->rivet_after_script = overrides->rivet_after_script ?
	overrides->rivet_after_script : base->rivet_after_script;

    rsc->rivet_error_script = overrides->rivet_error_script ?
	overrides->rivet_error_script : base->rivet_error_script;

    rsc->rivet_default_error_script = overrides->rivet_default_error_script ?
	overrides->rivet_default_error_script : base->rivet_default_error_script;

    /* cache_size is global, and set up later. */
    /* cache_free is not set up at this point. */

    rsc->upload_max = overrides->upload_max ?
	overrides->upload_max : base->upload_max;

    rsc->separate_virtual_interps = base->separate_virtual_interps;

    /* server_name is set up later. */

    rsc->upload_dir = overrides->upload_dir ?
	overrides->upload_dir : base->upload_dir;

    rsc->rivet_server_vars = overrides->rivet_server_vars ?
	overrides->rivet_server_vars : base->rivet_server_vars;

    rsc->rivet_dir_vars = overrides->rivet_dir_vars ?
	overrides->rivet_dir_vars : base->rivet_dir_vars;

    rsc->rivet_user_vars = overrides->rivet_user_vars ?
	overrides->rivet_user_vars : base->rivet_user_vars;

    /* objCacheList is set up later. */
    /* objCache is set up later. */
    /* outchannel is set up later. */

    return rsc;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildHandlers --
 *
 * 	Handles, depending on the situation, the scripts for the init
 * 	and exit handlers.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Runs the rivet_child_init/exit_script scripts.
 *
 *-----------------------------------------------------------------------------
 */

void
Rivet_ChildHandlers(server_rec *s, pool *p, int init)
{
    server_rec *sr;
    rivet_server_conf *rsc;
    rivet_server_conf *top;
    void *function;
    void *parentfunction;
    char *errmsg;

    top = RIVET_SERVER_CONF(s->module_config);
    if (init == 1) {
	parentfunction = top->rivet_child_init_script;
	errmsg = ap_pstrdup(p, "Error in child init script: %s");
    } else {
	parentfunction = top->rivet_child_exit_script;
	errmsg = ap_pstrdup(p, "Error in child exit script: %s");
    }

    sr = s;
    while(sr)
    {
	rsc = RIVET_SERVER_CONF(sr->module_config);
	function = init ? rsc->rivet_child_init_script :
	    rsc->rivet_child_exit_script;

	/* Execute it if it exists and it's the top level, separate
	 * virtual interps are turned on, or it's different than the
	 * main script. */
	if(function &&
	    ( sr == s || rsc->separate_virtual_interps ||
	      function != parentfunction))
	{
	    if (Tcl_EvalObjEx(rsc->server_interp,
			      function, 0) != TCL_OK) {
		ap_log_error(APLOG_MARK, APLOG_ERR, s,
			     errmsg,
			     Tcl_GetString(function));
		ap_log_error(APLOG_MARK, APLOG_ERR, s, "%s",
			     Tcl_GetVar(rsc->server_interp, "errorInfo", 0));
	    }
	}
	sr = sr->next;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildInit --
 *
 * 	This function is run when each individual Apache child process
 * 	is created.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Calls Tcl initialization functions for the new child process and
 *      causes the ChildInitScript to be executed, if it is defined.
 *
 *-----------------------------------------------------------------------------
 */

void
Rivet_ChildInit(server_rec *s, pool *p)
{
    Rivet_InitTclStuff(s, p);
    Rivet_ChildHandlers(s, p, 1);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildExit --
 *
 * 	Perform Rivet end-of-child processing.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Runs the ChildExitScript, if defined, and executes Tcl_Finalize
 *      to perform Tcl's own cleanup housekeeping without having Tcl
 *      exit, as Tcl_Exit would do, so that Apache can do the rest of
 *      it's child completion thing.
 *
 *-----------------------------------------------------------------------------
 */

void
Rivet_ChildExit(server_rec *s, pool *p)
{
    Rivet_ChildHandlers(s, p, 0);
    Tcl_Finalize();
    return;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitHandler --
 *
 * 	Perform Rivet module initialization -- called by Apache
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *      Uses the ap_add_version_component API to put Rivet in the
 *      "Server" header line sent back with the webpage.
 *
 *-----------------------------------------------------------------------------
 */

MODULE_VAR_EXPORT void
Rivet_InitHandler(server_rec *s, pool *p)
{
#ifndef HIDE_RIVET_VERSION
    ap_add_version_component("Rivet / "VERSION);
#else
    ap_add_version_component("Rivet");
#endif /* !HIDE_RIVET_VERSION */
}

/* Define Rivet handlers -- two kinds, Rivet pages containing HTML and
 * Rivet code, and pure programs.
 */
const handler_rec rivet_handlers[] =
{
    {"application/x-httpd-rivet", Rivet_SendContent},
    {"application/x-rivet-tcl", Rivet_SendContent},
    {NULL}
};

/* Define Rivet commands to be provided to the Apache config file(s) */
const command_rec rivet_cmds[] =
{
    {"RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, TAKE2, NULL},
    {"RivetDirConf", Rivet_DirConf, NULL, ACCESS_CONF, TAKE2, NULL},
    {"RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO, TAKE2,
     "RivetUserConf key value: sets RivetUserConf(key) = value"},
    {NULL}
};

/* Define the Rivet module to Apache */
module MODULE_VAR_EXPORT rivet_module =
{
    STANDARD_MODULE_STUFF,
    Rivet_InitHandler,		/* initializer */
    Rivet_CreateDirConfig,	/* dir config creater */
    Rivet_MergeDirConfig,       /* dir merger --- default is to override */
    Rivet_CreateConfig,         /* server config */
    Rivet_MergeConfig,          /* merge server config */
    rivet_cmds,                 /* command table */
    rivet_handlers,		/* handlers */
    NULL,			/* filename translation */
    NULL,			/* check_user_id */
    NULL,			/* check auth */
    NULL,			/* check access */
    NULL,			/* type_checker */
    NULL,			/* fixups */
    NULL,			/* logger */
    NULL,			/* header parser */
    Rivet_ChildInit,            /* child_init */
    Rivet_ChildExit,            /* child_exit */
    NULL			/* post read-request */
};

/*
  Local Variables: ***
  compile-command: "./make.tcl" ***
  End: ***
*/
