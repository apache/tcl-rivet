/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000, 2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "mod_rivet"
 *    or "rivet", nor may "rivet" appear in their name, without prior
 *    written permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.  */

/* $Id$  */

/* mod_rivet.c by David Welton <davidw@apache.org>
 *            and Damon Courtney <damon@unreality.com>
 * Originally based off code from mod_include.
 */

/* See http://tcl.apache.org/mod_rivet/credits.rvt for additional credits. */

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
#include <string.h>

/* Rivet includes */
#include "mod_rivet.h"
#include "rivet.h"
#include "rivetParser.h"
#include "rivetChannel.h"

module MODULE_VAR_EXPORT rivet_module;

/* Need some arbitrary non-NULL pointer which can't also be a request_rec */
#define NESTED_INCLUDE_MAGIC	(&rivet_module)

static Tcl_Condition *sendMutex;

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


/* Calls Tcl_EvalObjEx() and checks for errors
 * Prints the error buffer if any.
 */

static int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *r)
{
    rivet_server_conf *conf = Rivet_GetConf(r);
    rivet_interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if( Tcl_EvalObjEx(interp, outbuf, 0) == TCL_ERROR ) {
	Tcl_Obj *errscript;

	Tcl_SetVar( interp, "errorOutbuf",
			Tcl_GetStringFromObj( outbuf, NULL ),
			TCL_GLOBAL_ONLY );

	/* If we don't have an error script, use the default error handler. */
	if( !conf->rivet_error_script ) {
	    errscript = conf->rivet_default_error_script;
	} else {
	    errscript = Tcl_NewStringObj(conf->rivet_error_script, -1);
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
    TclWeb_PrintHeaders(globals->req);
    Tcl_Flush(*(conf->outchannel));

    return TCL_OK;
}

/* This is a seperate function so that it may be called from 'Parse' */
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

    /* If toplevel is 0, we are being called from Parse, which means
       we need to get the information about the file ourselves. */
    if (toplevel == 0)
    {
	struct stat stat;
	if( Tcl_Stat(filename, &stat) < 0 ) return TCL_ERROR;
	ctime = stat.st_ctime;
	mtime = stat.st_mtime;
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

    /* We don't have a compiled version.  Let's create one */
    if (isNew || *(rsc->cache_size) == 0)
    {
	outbuf = Tcl_NewObj();
	if (toplevel && rsc->rivet_before_script) {
	    Tcl_AppendObjToObj(outbuf, Tcl_NewStringObj(rsc->rivet_before_script, -1));
	}
	if( STREQU( req->req->content_type, "application/x-httpd-rivet")
	    || !toplevel )
	{
	    /* toplevel == 0 means we are being called from the parse
	     * command, which only works on Rivet .rvt files. */
	    result = Rivet_GetRivetFile(filename, toplevel, outbuf, req);
	} else {
	    /* It's a plain Tcl file */
	    result = Rivet_GetTclFile(filename, outbuf, req);
	}
	if (result != TCL_OK)
	{
	    return result;
	}
	if (toplevel && rsc->rivet_after_script) {
	    Tcl_AppendObjToObj(outbuf, Tcl_NewStringObj(rsc->rivet_after_script, -1));
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
	    delEntry =
		Tcl_FindHashEntry(rsc->objCache,
				  rsc->objCacheList[*(rsc->cache_size)-1]);
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
    }

    return Rivet_ExecuteAndCheck(interp, outbuf, req->req);
}

static void
Rivet_CleanupRequest( request_rec *r )
{
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
static int
Rivet_SendContent(request_rec *r)
{
    char error[MAX_STRING_LEN];
    char timefmt[MAX_STRING_LEN];
    int errstatus;
    int retval;

    Tcl_Interp	*interp;
    Tcl_Obj	*request_init;
    Tcl_Obj	*request_cleanup;

    rivet_interp_globals *globals = NULL;
    rivet_server_conf *rsc = NULL;
    rivet_server_conf *rdc;

    Tcl_MutexLock(sendMutex);

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

    Rivet_PropagatePerDirConfArrays( interp, rdc );

    request_init = Tcl_NewStringObj("::Rivet::initialize_request\n",-1);
    if (Tcl_EvalObjEx(interp, request_init, TCL_EVAL_DIRECT) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
			"Could not create request namespace\n");
	retval = HTTP_BAD_REQUEST;
	goto sendcleanup;
    }


    {
	Tcl_Obj *infoscript[3];
	infoscript[0] = Tcl_NewStringObj("info", -1);
	infoscript[1] = Tcl_NewStringObj("script", -1);
	infoscript[2] = Tcl_NewStringObj(r->filename, -1);
	Tcl_EvalObjv(interp, 3, infoscript, 0);
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

    if ((errstatus = ApacheRequest___parse(globals->req->apachereq)) != OK) {
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

    request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
    if(Tcl_EvalObjEx(interp, request_cleanup, TCL_EVAL_DIRECT) == TCL_ERROR) {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "%s",
		     Tcl_GetVar(interp, "errorInfo", 0));
    }

    /* Reset globals */


    Rivet_CleanupRequest( r );

    retval = OK;
sendcleanup:
    globals->req->content_sent = 0;

    Tcl_MutexUnlock(sendMutex);

    return retval;
}

/*
 * Setup an array in each interpreter to tell us things about Apache.
 * This saves us from having to do any real call to load an entire
 * environment.  This routine only gets called once, when the child process
 * is created.
 *
 * SERVER_ROOT - Apache's root location
 * SERVER_CONF - Apache's configuration file
 * RIVET_DIR   - Rivet's Tcl source directory
 * RIVET_INIT  - Rivet's init.tcl file
 */
static void
Rivet_InitServerVariables( Tcl_Interp *interp, pool *p )
{
    Tcl_Obj *obj;

    obj = Tcl_NewStringObj(ap_server_root, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		  "server",
		  "SERVER_ROOT",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p, ap_server_confname), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		 "server",
		 "SERVER_CONF",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_DIR), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		  "server",
		  "RIVET_DIR",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(p, RIVET_INIT), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
		 "server",
		 "RIVET_INIT",
		  obj,
		  TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
}

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

static void
Rivet_InitTclStuff(server_rec *s, pool *p)
{
    int rslt;
    Tcl_Interp *interp;
    rivet_interp_globals *globals = NULL;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    server_rec *sr;

    /* Apache actually loads all the modules twice, just to see if it
     * can. This is a pain, because things don't seem to get
     * completely cleaned up on the Tcl side. So this little hack
     * should make us *really* load only the second time around. */

    if (getenv("RIVET_INIT") == NULL) {
	setenv("RIVET_INIT", "1", 0);
	return;
    }

    /* Initialize TCL stuff  */
    Tcl_FindExecutable(NULL);
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

    rsc->server_interp = interp; /* root interpreter */

    /* Create TCL commands to deal with Apache's BUFFs. */
    *(rsc->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rsc,
					   TCL_WRITABLE);

    Tcl_SetStdChannel(*(rsc->outchannel), TCL_STDOUT);
    Tcl_SetChannelOption(interp, *(rsc->outchannel), "-buffersize", "1000000");
    Tcl_RegisterChannel(interp, *(rsc->outchannel));

    /* Initialize the interpreter with Rivet's Tcl commands */
    Rivet_InitCore( interp );

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables( interp, p );

    Rivet_PropagateServerConfArray( interp, rsc );

    /* Set up interpreter associated data */
    globals = ap_pcalloc(p, sizeof(rivet_interp_globals));
    Tcl_SetAssocData(interp, "rivet", NULL, globals);

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */
    if( Tcl_EvalFile( interp, ap_server_root_relative(p, RIVET_INIT) )
	== TCL_ERROR ) {
	ap_log_error( APLOG_MARK, APLOG_ERR, s, "init.tcl must be installed correctly for Apache Rivet to function: %s", Tcl_GetStringResult(interp) );
	exit(1);
    }

    if (rsc->rivet_global_init_script != NULL)
    {
	rslt = Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0);
	if (rslt != TCL_OK)
	{
	    ap_log_error(APLOG_MARK, APLOG_ERR, s, "%s",
			 Tcl_GetVar(interp, "errorInfo", 0));
	}
    }

    /* If the user didn't set a cache size in their configuration, we
     * will assume an arbitrary size for them.
     *
     * If the cache size is 0, the user has requested not to cache documents.
     */
    if(*(rsc->cache_size) < 0)
    {
	if (ap_max_requests_per_child != 0) {
	    *(rsc->cache_size) = ap_max_requests_per_child / 5;
	} else {
	    *(rsc->cache_size) = 10; /* FIXME: Arbitrary number */
	}
	*(rsc->cache_free) = *(rsc->cache_size);
    } else if (*(rsc->cache_size) > 0) {
	*(rsc->cache_free) = *(rsc->cache_size);
    }

    /* Initializing cache structures */
    rsc->objCacheList = ap_pcalloc(p,
				(signed)(*(rsc->cache_size) * sizeof(char *)));
    Tcl_InitHashTable(rsc->objCache, TCL_STRING_KEYS);

    sr = s;
    while (sr)
    {
	rivet_server_conf *myrsc = NULL;
	/* This should set up slave interpreters for other virtual hosts */
	if (sr != s) /* not the first one  */
	{
	    myrsc = RIVET_NEW_CONF(p);
	    ap_set_module_config(sr->module_config, &rivet_module, myrsc);
	    Rivet_CopyConfig( rsc, myrsc );
	    if (rsc->seperate_virtual_interps != 0) {
		myrsc->server_interp = NULL;
	    }
	} else {
	    myrsc = RIVET_SERVER_CONF( sr->module_config );
	}
	if (!myrsc->server_interp)
	{
	    myrsc->server_interp = Tcl_CreateSlave(interp,
						    sr->server_hostname, 0);
	    Rivet_InitCore( myrsc->server_interp );
	    Tcl_SetChannelOption(myrsc->server_interp, *(rsc->outchannel),
				    "-buffering", "none");
	    Tcl_RegisterChannel(myrsc->server_interp, *(rsc->outchannel));
	    globals = ap_pcalloc(p, sizeof(rivet_interp_globals));
	    Tcl_SetAssocData(myrsc->server_interp, "rivet", NULL, globals);
	}

	myrsc->server_name = ap_pstrdup(p, sr->server_hostname);
	sr = sr->next;
    }
}

static char *
Rivet_AppendToScript( ap_pool *pool, rivet_server_conf *rsc, char *script, char *string )
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
	if( rsc->rivet_before_script == NULL ) {
	    rsc->rivet_before_script = ap_pstrcat(pool, string, "\n", NULL);
	} else {
	    rsc->rivet_before_script = ap_pstrcat(pool, rsc->rivet_before_script,
						  string, "\n", NULL);
	}
    } else if( STREQU( script, "AfterScript" ) ) {
	if( rsc->rivet_after_script == NULL ) {
	    rsc->rivet_after_script = ap_pstrcat(pool, string, "\n", NULL);
	} else {
	    rsc->rivet_after_script = ap_pstrcat(pool, rsc->rivet_after_script,
						  string, "\n", NULL);
	}
    } else if( STREQU( script, "ErrorScript" ) ) {
	if( rsc->rivet_error_script == NULL ) {
	    rsc->rivet_error_script = ap_pstrcat(pool, string, "\n", NULL);
	} else {
	    rsc->rivet_error_script = ap_pstrcat(pool, rsc->rivet_error_script,
						 string, "\n", NULL);
	}
    }

    if( !objarg ) return string;

    return Tcl_GetStringFromObj( objarg, NULL );
}

/*
 * Implements the RivetServerConf Apache Directive
 *
 * Command Arguments:
 *	RivetServerConf GlobalInitScript <script>
 * 	RivetServerConf ChildInitScript <script>
 * 	RivetServerConf ChildExitScript <script>
 * 	RivetServerConf BeforeScript <script>
 * 	RivetServerConf AfterScript <script>
 * 	RivetServerConf ErrorScript <script>
 * 	RivetServerConf CacheSize <integer>
 * 	RivetServerConf UploadDirectory <directory>
 * 	RivetServerConf UploadMaxSize <integer>
 * 	RivetServerConf UploadFilesToVar <yes|no>
 * 	RivetServerConf SeparateVirtualInterps <yes|no>
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
	if( STREQU( val, "on" ) ) {
	    rsc->upload_files_to_var = 1;
	} else {
	    rsc->upload_files_to_var = 0;
	}
    } else if( STREQU( var, "SeparateVirtualInterps" ) ) {
	if( STREQU( val, "on" ) ) {
	    rsc->seperate_virtual_interps = 1;
	} else {
	    rsc->seperate_virtual_interps = 0;
	}
    } else {
	string = Rivet_AppendToScript( cmd->pool, rsc, var, val);
    }

    ap_table_set( rsc->rivet_server_vars, var, string );
    return( NULL );
}

/*
 * Implements the RivetDirConf Apache Directive
 *
 * Command Arguments:
 * 	RivetDirConf BeforeScript <script>
 * 	RivetDirConf AfterScript <script>
 * 	RivetDirConf ErrorScript <script>
 * 	RivetDirConf UploadDirectory <directory>
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
	string = Rivet_AppendToScript( cmd->pool, rdc, var, val );
    }

    ap_table_set( rdc->rivet_dir_vars, var, string );
    return( NULL );
}

/*
 * Implements the RivetUserConf Apache Directive
 *
 * Command Arguments:
 * 	RivetUserConf BeforeScript <script>
 * 	RivetUserConf AfterScript <script>
 * 	RivetUserConf ErrorScript <script>
*/
static const char *
Rivet_UserConf( cmd_parms *cmd, rivet_server_conf *rdc, char *var, char *val )
{
    char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetUserConf requires two arguments";
    }

    string = Rivet_AppendToScript( cmd->pool, rdc, var, val );

    ap_table_set( rdc->rivet_user_vars, var, string );
    return( NULL );
}

/*
 * Merge the per-directory configuration options into a new configuration.
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

static void
Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
{
    FILEDEBUGINFO;

    newrsc->server_interp = oldrsc->server_interp;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;
    newrsc->rivet_child_init_script = oldrsc->rivet_child_init_script;
    newrsc->rivet_child_exit_script = oldrsc->rivet_child_exit_script;
    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;

    newrsc->rivet_default_error_script = oldrsc->rivet_default_error_script;

    /* these are pointers so that they can be passed around...  */
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->upload_max = oldrsc->upload_max;
    newrsc->upload_files_to_var = oldrsc->upload_files_to_var;
    newrsc->seperate_virtual_interps = oldrsc->seperate_virtual_interps;
    newrsc->server_name = oldrsc->server_name;
    newrsc->upload_dir = oldrsc->upload_dir;
    newrsc->objCacheList = oldrsc->objCacheList;
    newrsc->objCache = oldrsc->objCache;

    newrsc->outchannel = oldrsc->outchannel;
}

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

    rsc->rivet_default_error_script = Tcl_NewStringObj("::Rivet::handle_error", -1);
    Tcl_IncrRefCount(rsc->rivet_default_error_script);

    /* these are pointers so that they can be passed around...  */
    rsc->cache_size = ap_pcalloc(p, sizeof(int));
    rsc->cache_free = ap_pcalloc(p, sizeof(int));
    *(rsc->cache_size) = -1;
    *(rsc->cache_free) = 0;
    rsc->upload_max = 0;
    rsc->upload_files_to_var = 0;
    rsc->seperate_virtual_interps = 0;
    rsc->server_name = NULL;
    rsc->upload_dir = "/tmp";
    rsc->objCacheList = NULL;
    rsc->objCache = ap_pcalloc(p, sizeof(Tcl_HashTable));

    rsc->outchannel = ap_pcalloc(p, sizeof(Tcl_Channel));

    rsc->rivet_server_vars = ap_make_table( p, 4 );
    rsc->rivet_dir_vars = ap_make_table( p, 4 );
    rsc->rivet_user_vars = ap_make_table( p, 4 );

    return rsc;
}

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

void *
Rivet_MergeConfig(pool *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);
    rivet_server_conf *base = (rivet_server_conf *) basev;
    rivet_server_conf *overrides = (rivet_server_conf *) overridesv;

    FILEDEBUGINFO;

    rsc->server_interp = overrides->server_interp ?
	overrides->server_interp : base->server_interp;

    rsc->rivet_before_script = overrides->rivet_before_script ?
	overrides->rivet_before_script : base->rivet_before_script;

    rsc->rivet_after_script = overrides->rivet_after_script ?
	overrides->rivet_after_script : base->rivet_after_script;

    rsc->rivet_error_script = overrides->rivet_error_script ?
	overrides->rivet_error_script : base->rivet_error_script;

    rsc->upload_max = overrides->upload_max ?
	overrides->upload_max : base->upload_max;

    rsc->server_name = overrides->server_name ?
	overrides->server_name : base->server_name;
    rsc->upload_dir = overrides->upload_dir ?
	overrides->upload_dir : base->upload_dir;

    return rsc;
}

void
Rivet_ChildInit(server_rec *s, pool *p)
{
    server_rec *sr;
    rivet_server_conf *rsc;

#if THREADED_TCL == 1
    Rivet_InitTclStuff(s, p);
#endif

    sr = s;
    while(sr)
    {
	rsc = RIVET_SERVER_CONF(sr->module_config);
	if( rsc->rivet_child_init_script != NULL )
	{
	    if (Tcl_EvalObjEx(rsc->server_interp,
			      rsc->rivet_child_init_script, 0) != TCL_OK) {
		ap_log_error(APLOG_MARK, APLOG_ERR, s,
			     "Problem running child init script: %s",
			     Tcl_GetString(rsc->rivet_child_init_script));
	    }
	}
	sr = sr->next;
    }
}

void
Rivet_ChildExit(server_rec *s, pool *p)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );

    if( rsc->rivet_child_exit_script != NULL ) {
	if ( Tcl_EvalObjEx(rsc->server_interp, rsc->rivet_child_exit_script, 0)
	     != TCL_OK) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, s,
			 "Problem running child exit script: %s",
			 Tcl_GetStringFromObj(rsc->rivet_child_exit_script, NULL));
	}
    }
    Tcl_Finalize();
    return;
}

MODULE_VAR_EXPORT void
Rivet_InitHandler(server_rec *s, pool *p)
{
#if THREADED_TCL == 0
    Rivet_InitTclStuff(s, p);
#endif

#ifndef HIDE_RIVET_VERSION
    ap_add_version_component("Rivet / "RIVET_VERSION);
#else
    ap_add_version_component("Rivet");
#endif /* !HIDE_RIVET_VERSION */
}

const handler_rec rivet_handlers[] =
{
    {"application/x-httpd-rivet", Rivet_SendContent},
    {"application/x-rivet-tcl", Rivet_SendContent},
    {NULL}
};

const command_rec rivet_cmds[] =
{
    {"RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, TAKE2, NULL},
    {"RivetDirConf", Rivet_DirConf, NULL, ACCESS_CONF, TAKE2, NULL},
    {"RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO, TAKE2,
     "RivetUserConf key value: sets RivetUserConf(key) = value"},
    {NULL}
};

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
