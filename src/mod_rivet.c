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
#include "parser.h"
#include "channel.h"
#include "apache_request.h"
#include "mod_rivet.h"
#include "rivet.h"

module MODULE_VAR_EXPORT rivet_module;

/* Need some arbitrary non-NULL pointer which can't also be a request_rec */
#define NESTED_INCLUDE_MAGIC	(&rivet_module)

static void Rivet_InitTclStuff(server_rec *s, pool *p);
static void Rivet_CopyConfig( rivet_server_conf *oldrsc,
				rivet_server_conf *newrsc);
static int Rivet_GetRivetFile(request_rec *r, rivet_server_conf *rsc,
			 Tcl_Interp *interp, char *filename, int toplevel,
			 Tcl_Obj *outbuf);
static int Rivet_SendContent(request_rec *);
static int Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf,
				request_rec *r);

/* Set up the content type header */

int
Rivet_SetHeaderType(request_rec *r, char *header)
{
    rivet_server_conf *rsc = Rivet_GetConf(r);

    if( *(rsc->headers_set) ) return 0;

    r->content_type = header;
    *(rsc->headers_set) = 1;
    return 1;
}

/* Printer headers if they haven't been printed yet */
int
Rivet_PrintHeaders(request_rec *r)
{
    rivet_server_conf *rsc = Rivet_GetConf(r);

    if( *(rsc->headers_printed) ) return 0;

    if (*(rsc->headers_set) == 0)
	Rivet_SetHeaderType(r, DEFAULT_HEADER_TYPE);

    ap_send_http_header(r);
    *(rsc->headers_printed) = 1;
    return 1;
}

/* Print nice HTML formatted errors */
int
Rivet_PrintError(request_rec *r, int htmlflag, char *errstr)
{
    Rivet_SetHeaderType(r, DEFAULT_HEADER_TYPE);
    Rivet_PrintHeaders(r);

    if (htmlflag != 1)
	ap_rputs(ER1, r);

    if (errstr != NULL)
    {
	if (htmlflag != 1)
	{
	    ap_rputs(ap_escape_html(r->pool, errstr), r);
	} else {
	    ap_rputs(errstr, r);
	}
    }
    if (htmlflag != 1)
	ap_rputs(ER2, r);

    return 0;
}

/* Function to convert strings to UTF encoding */
char *
Rivet_StringToUtf(char *input, ap_pool *pool)
{
    char *temp;
    Tcl_DString dstr;
    Tcl_DStringInit(&dstr);
    Tcl_ExternalToUtfDString(NULL, input, (signed)strlen(input), &dstr);

    temp = ap_pstrdup(pool, Tcl_DStringValue(&dstr));
    Tcl_DStringFree(&dstr);
    return temp;
}

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

/* Load, cache and eval a Tcl file  */

static int
Rivet_GetTclFile(request_rec *r, Tcl_Interp *interp,
		char *filename, Tcl_Obj *outbuf)
{
    int result = 0;

    /* Taken, in part, from tclIOUtil.c out of the Tcl distribution,
     * and modified.
     */

    /* Basically, what we are doing here is a Tcl_EvalFile but with the
     * addition of caching code.
     */
    Tcl_Channel chan = Tcl_OpenFileChannel(interp, r->filename, "r", 0644);
    if (chan == (Tcl_Channel) NULL)
    {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "couldn't read file \"", r->filename,
			 "\": ", Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    result = Tcl_ReadChars(chan, outbuf, (signed)r->finfo.st_size, 1);
    if (result < 0)
    {
	Tcl_Close(interp, chan);
	Tcl_AppendResult(interp, "couldn't read file \"", r->filename,
			 "\": ", Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    if (Tcl_Close(interp, chan) != TCL_OK)
	return TCL_ERROR;

    return TCL_OK;
}

/* Parse and execute a Rivet file */

static int
Rivet_GetRivetFile(request_rec *r, rivet_server_conf *rsc, Tcl_Interp *interp,
			 char *filename, int toplevel, Tcl_Obj *outbuf)
{
    /* BEGIN PARSER  */
    int inside = 0;	/* are we inside the starting/ending delimiters  */


    FILE *f = NULL;

    if (!(f = ap_pfopen(r->pool, filename, "r")))
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
		     "file permissions deny server access: %s", filename);
	return HTTP_FORBIDDEN;
    }

    if (toplevel)
    {
	Tcl_SetStringObj(outbuf, "namespace eval request {\n", -1);
	if (rsc->rivet_before_script) {
	    Tcl_AppendObjToObj(outbuf, rsc->rivet_before_script);
	}
	Tcl_AppendToObj(outbuf, "puts \"", -1);
    }
    else
	Tcl_SetStringObj(outbuf, "puts \"\n", -1);

    /* if inside < 0, it's an error  */
    inside = Rivet_Parser(outbuf, f);
    if (inside < 0)
    {
	if (ferror(f))
	{
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
			 "Encountered error in mod_rivet getchar routine "
			 "while reading %s",
			 r->uri);
	    ap_pfclose( r->pool, f);
	}
    }

    ap_pfclose(r->pool, f);

    if (inside == 0)
    {
	Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    if (toplevel)
    {
	if (rsc->rivet_after_script)
	    Tcl_AppendObjToObj(outbuf, rsc->rivet_after_script);

	Tcl_AppendToObj(outbuf, "\n}\n", -1);
    }
    else
	Tcl_AppendToObj(outbuf, "\n", -1);

    /* END PARSER  */
    return TCL_OK;
}

/* Calls Tcl_EvalObj() and checks for errors
 * Prints the error buffer if any.
 */

static int
Rivet_ExecuteAndCheck(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *r)
{
    char *errorinfo;
    rivet_server_conf *conf = NULL;

    conf = Rivet_GetConf(r);
    if (Tcl_EvalObj(interp, outbuf) == TCL_ERROR)
    {
	Tcl_Obj *errscript =
	    conf->rivet_error_script ? conf->rivet_error_script : NULL;

        Rivet_PrintHeaders(r);
	Tcl_Flush(*(conf->outchannel));
        if (errscript)
        {
	    if (Tcl_EvalObj(interp, errscript) == TCL_ERROR)
                Rivet_PrintError(r, 1, "<b>Tcl_ErrorScript failed!</b>");
        } else {
            /* default action  */
            errorinfo = Tcl_GetVar(interp, "errorInfo", 0);
            Rivet_PrintError(r, 0, errorinfo);
            Rivet_PrintError(r, 1, "<p><b>OUTPUT BUFFER:</b></p>");
            Rivet_PrintError(r, 0, Tcl_GetStringFromObj(outbuf, (int *)NULL));
        }
    } else {
        /* Make sure to flush the output if buffer_add was the only output */
        Rivet_PrintHeaders(r);
	Tcl_Flush(*(conf->outchannel));
    }
    return OK;
}

/* This is a seperate function so that it may be called from 'Parse' */
int
Rivet_ParseExecFile(request_rec *r, rivet_server_conf *rsc,
			char *filename, int toplevel)
{
    char *hashKey = NULL;
    int isNew = 0;
    int result = 0;

    Tcl_Obj *outbuf = NULL;
    Tcl_HashEntry *entry = NULL;
    Tcl_Interp *interp = rsc->server_interp;

    time_t ctime;
    time_t mtime;

    /* If toplevel is 0, we are being called from Parse, which means
       we need to get the information about the file ourselves. */
    if (toplevel == 0)
    {
	int ret = 0;
	struct stat stat;
	ret = Tcl_Stat(filename, &stat);
	if (ret < 0)
	    return TCL_ERROR;
	ctime = stat.st_ctime;
	mtime = stat.st_mtime;
    } else {
	ctime = r->finfo.st_ctime;
	mtime = r->finfo.st_mtime;
    }

    /* Look for the script's compiled version.  * If it's not found,
     * create it.
     */
    if (*(rsc->cache_size))
    {
	hashKey = ap_psprintf(r->pool, "%s%lx%lx%d", filename,
			      mtime, ctime, toplevel);
	entry = Tcl_CreateHashEntry(rsc->objCache, hashKey, &isNew);
    }

    /* We don't have a compiled version.  Let's create one */
    if (isNew || *(rsc->cache_size) == 0)
    {
	outbuf = Tcl_NewObj();
	Tcl_IncrRefCount(outbuf);

	if( STREQU( r->content_type, "application/x-httpd-rivet") || !toplevel )
	{
	    /* It's a Rivet file - which we always are if toplevel is 0,
	     * meaning we are in the Parse command.
	     */
	    result = Rivet_GetRivetFile(r,rsc,interp,filename,toplevel,outbuf);
	} else {
	    /* It's a plain Tcl file */
	    result = Rivet_GetTclFile(r, interp, filename, outbuf);
	}
	if (result != TCL_OK)
	    return result;

	if (*(rsc->cache_size))
	    Tcl_SetHashValue(entry, (ClientData)outbuf);

	if (*(rsc->cache_free)) {
	    rsc->objCacheList[-- *(rsc->cache_free) ] = strdup(hashKey);
	} else if (*(rsc->cache_size)) { /* If it's zero, we just skip this. */
	    Tcl_HashEntry *delEntry;
	    delEntry = Tcl_FindHashEntry(rsc->objCache,
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

    Rivet_ExecuteAndCheck(interp, outbuf, r);

    return TCL_OK;
}

static void
Rivet_PropagatePerDirConfArrays( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    table *t;
    array_header *arr;
    table_entry  *elts;
    int i, nelts;

    /* Make sure RivetDirConf doesn't exist from a previous request. */
    Tcl_UnsetVar( interp, "RivetDirConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the DirConf variables into an array. */
    t = rsc->rivet_dir_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;

    for( i = 0; i < nelts; ++i )
    {
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("RivetDirConf", -1),
		       Tcl_NewStringObj( elts[i].key, -1),
		       Tcl_NewStringObj( elts[i].val, -1),
		       TCL_GLOBAL_ONLY);
    }

    /* Make sure RivetUserConf doesn't exist from a previous request. */
    Tcl_UnsetVar( interp, "RivetUserConf", TCL_GLOBAL_ONLY );

    /* Propagate all of the UserConf variables into an array. */
    t = rsc->rivet_user_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;

    for( i = 0; i < nelts; ++i )
    {
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("RivetUserConf", -1),
		       Tcl_NewStringObj( elts[i].key, -1),
		       Tcl_NewStringObj( elts[i].val, -1),
		       TCL_GLOBAL_ONLY);
    }

}

/* Set things up to execute a file, then execute */
static int
Rivet_SendContent(request_rec *r)
{
    char error[MAX_STRING_LEN];
    char timefmt[MAX_STRING_LEN];
    int errstatus;

    Tcl_Interp *interp;

    rivet_interp_globals *globals = NULL;
    rivet_server_conf *rsc = NULL;
    rivet_server_conf *rdc;
    rsc = Rivet_GetConf(r);
    globals = ap_pcalloc(r->pool, sizeof(rivet_interp_globals));
    globals->r = r;
    interp = rsc->server_interp;
    Tcl_SetAssocData(interp, "rivet", NULL, globals);

    rdc = RIVET_SERVER_CONF( r->per_dir_config );

    r->allowed |= (1 << M_GET);
    r->allowed |= (1 << M_POST);
    if (r->method_number != M_GET && r->method_number != M_POST)
	return DECLINED;

    if (r->finfo.st_mode == 0)
    {
	ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r->server,
		     "File does not exist: %s",
		     (r->path_info
		      ? ap_pstrcat(r->pool, r->filename, r->path_info, NULL)
		      : r->filename));
	return HTTP_NOT_FOUND;
    }

    if ((errstatus = ap_meets_conditions(r)) != OK)
	return errstatus;

    if (r->header_only)
    {
	Rivet_SetHeaderType(r, DEFAULT_HEADER_TYPE);
	Rivet_PrintHeaders(r);
	return OK;
    }

    ap_cpystrn(error, DEFAULT_ERROR_MSG, sizeof(error));
    ap_cpystrn(timefmt, DEFAULT_TIME_FORMAT, sizeof(timefmt));
    ap_chdir_file(r->filename);

    Rivet_PropagatePerDirConfArrays( interp, rdc );

    if (Tcl_EvalObj(interp, rsc->request_init) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
			"Could not create request namespace\n");
	return HTTP_BAD_REQUEST;
    }

    /* Apache Request stuff */

    globals->req = ApacheRequest_new(r);

    ApacheRequest_set_post_max(globals->req, rsc->upload_max);
    ApacheRequest_set_temp_dir(globals->req, rsc->upload_dir);

#if 0
    if (upload_files_to_var)
    {
	globals->req->hook_data = interp;
	globals->req->upload_hook = Rivet_UploadHook;
    }
#endif

    if ((errstatus = ApacheRequest___parse(globals->req)) != OK)
	return errstatus;

    Rivet_ParseExecFile(r, rsc, r->filename, 1);

    if( Tcl_EvalObj( interp, rsc->request_cleanup ) == TCL_ERROR ) {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "%s",
		     Tcl_GetVar(interp, "errorInfo", 0));
    }

    /* Reset globals */
    *(rsc->headers_printed) = 0;
    *(rsc->headers_set) = 0;
    *(rsc->content_sent) = 0;

    return OK;
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
    Tcl_ObjSetVar2(interp,
		   Tcl_NewStringObj("server", -1),
		   Tcl_NewStringObj("SERVER_ROOT", -1),
		   Tcl_NewStringObj(ap_server_root, -1),
		   TCL_GLOBAL_ONLY);
    Tcl_ObjSetVar2(interp,
		   Tcl_NewStringObj("server", -1),
		   Tcl_NewStringObj("SERVER_CONF", -1),
		   Tcl_NewStringObj(
			ap_server_root_relative(p, ap_server_confname), -1),
		   TCL_GLOBAL_ONLY);
    Tcl_ObjSetVar2(interp,
		   Tcl_NewStringObj("server", -1),
		   Tcl_NewStringObj("RIVET_DIR", -1),
		   Tcl_NewStringObj(ap_server_root_relative(p, RIVET_DIR), -1),
		   TCL_GLOBAL_ONLY);
    Tcl_ObjSetVar2(interp,
		   Tcl_NewStringObj("server", -1),
		   Tcl_NewStringObj("RIVET_INIT", -1),
		   Tcl_NewStringObj(ap_server_root_relative(p, RIVET_INIT), -1),
		   TCL_GLOBAL_ONLY);
}

static void
Rivet_PropagateServerConfArray( Tcl_Interp *interp, rivet_server_conf *rsc )
{
    table *t;
    array_header *arr;
    table_entry  *elts;
    int i, nelts;

    /* Propagate all of the ServerConf variables into an array. */
    t = rsc->rivet_server_vars;
    arr   = ap_table_elts( t );
    elts  = (table_entry *)arr->elts;
    nelts = arr->nelts;

    for( i = 0; i < nelts; ++i )
    {
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("RivetServerConf", -1),
		       Tcl_NewStringObj( elts[i].key, -1),
		       Tcl_NewStringObj( elts[i].val, -1),
		       TCL_GLOBAL_ONLY);
    }
}

static void
Rivet_InitTclStuff(server_rec *s, pool *p)
{
    int rslt;
    Tcl_Interp *interp;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );
    server_rec *sr;

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
    Rivet_init( interp );

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables( interp, p );

    Rivet_PropagateServerConfArray( interp, rsc );

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */
    if( Tcl_EvalFile( interp, ap_server_root_relative(p, RIVET_INIT) )
	== TCL_ERROR ) {
	ap_log_error( APLOG_MARK, APLOG_ERR, s, Tcl_GetStringResult(interp) );
	exit(1);
    }

    rsc->request_init = Tcl_NewStringObj("::Rivet::initialize_request\n",-1);
    Tcl_IncrRefCount(rsc->request_init);

    rsc->request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n",-1);
    Tcl_IncrRefCount(rsc->request_cleanup);

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
	if (ap_max_requests_per_child != 0)
	    *(rsc->cache_size) = ap_max_requests_per_child / 2;
	else
	    *(rsc->cache_size) = 10; /* FIXME: Arbitrary number */
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
	    if (rsc->seperate_virtual_interps != 0)
		myrsc->server_interp = NULL;
	} else {
	    myrsc = RIVET_SERVER_CONF( sr->module_config );
	}
	if (!myrsc->server_interp)
	{
	    myrsc->server_interp = Tcl_CreateSlave(interp,
						    sr->server_hostname, 0);
	    Rivet_init( myrsc->server_interp );
	    Tcl_SetChannelOption(myrsc->server_interp, *(rsc->outchannel),
				    "-buffering", "none");
	    Tcl_RegisterChannel(myrsc->server_interp, *(rsc->outchannel));
	}

	myrsc->server_name = ap_pstrdup(p, sr->server_hostname);
	sr = sr->next;
    }
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
    Tcl_Obj *objarg;
    server_rec *s = cmd->server;
    rivet_server_conf *rsc = RIVET_SERVER_CONF(s->module_config);

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetServerConf requires two arguments";
    }

    if( STREQU( var, "GlobalInitScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_global_init_script = objarg;
    } else if( STREQU( var, "ChildInitScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_child_init_script = objarg;
    } else if( STREQU( var, "ChildExitScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_child_exit_script = objarg;
    } else if( STREQU( var, "BeforeScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_before_script = objarg;
    } else if( STREQU( var, "AfterScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_after_script = objarg;
    } else if( STREQU( var, "ErrorScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rsc->rivet_error_script = objarg;
    } else if( STREQU( var, "CacheSize" ) ) {
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
    }

    ap_table_set( rsc->rivet_server_vars, var, val );

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
    Tcl_Obj *objarg;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetDirConf requires two arguments";
    }

    if( STREQU( var, "BeforeScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_before_script = objarg;
    } else if( STREQU( var, "AfterScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_after_script = objarg;
    } else if( STREQU( var, "ErrorScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_error_script = objarg;
    } else if( STREQU( var, "UploadDirectory" ) ) {
	rdc->upload_dir = val;
    }

    ap_table_set( rdc->rivet_dir_vars, var, val );
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
    Tcl_Obj *objarg;

    if ( var == NULL || val == NULL ) {
	return "Rivet Error: RivetUserConf requires two arguments";
    }

    if( STREQU( var, "BeforeScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_before_script = objarg;
    } else if( STREQU( var, "AfterScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_after_script = objarg;
    } else if( STREQU( var, "ErrorScript" ) ) {
	objarg = Tcl_NewStringObj(val, -1);
	Tcl_IncrRefCount(objarg);
	Tcl_AppendToObj(objarg, "\n", 1);
	rdc->rivet_error_script = objarg;
    }

    ap_table_set( rdc->rivet_user_vars, var, val );
    return( NULL );
}

/* Function to get a config and merge the directory/server options  */
rivet_server_conf *
Rivet_GetConf( request_rec *r )
{
    rivet_server_conf *newconfig = NULL;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->server->module_config );
    rivet_server_conf *rdc;
    void *dconf = r->per_dir_config;


    /* If there is no per dir config, just return the server config */
    if( dconf == NULL ) return rsc;

    rdc = RIVET_SERVER_CONF( dconf );

    newconfig = RIVET_NEW_CONF( r->pool );
    newconfig->server_interp = rsc->server_interp;

    Rivet_CopyConfig( rsc, newconfig );

    /* List here things that can be per-directory. */

    newconfig->rivet_before_script = rdc->rivet_before_script ?
	rdc->rivet_before_script : rsc->rivet_before_script;

    newconfig->rivet_after_script = rdc->rivet_after_script ?
	rdc->rivet_after_script : rsc->rivet_after_script;

    newconfig->rivet_error_script = rdc->rivet_error_script ?
	rdc->rivet_error_script : rsc->rivet_error_script;

    return newconfig;
}

static void
Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
{
    newrsc->server_interp = oldrsc->server_interp;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;
    newrsc->rivet_child_init_script = oldrsc->rivet_child_init_script;
    newrsc->rivet_child_exit_script = oldrsc->rivet_child_exit_script;
    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;

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
    newrsc->request_init = oldrsc->request_init;
    newrsc->request_cleanup = oldrsc->request_cleanup;

    newrsc->headers_printed = oldrsc->headers_printed;
    newrsc->headers_set = oldrsc->headers_set;
    newrsc->content_sent = oldrsc->content_sent;
    newrsc->outchannel = oldrsc->outchannel;
}

static void *
Rivet_CreateConfig( pool *p, server_rec *s )
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);

    rsc->server_interp = NULL;
    rsc->rivet_global_init_script = NULL;
    rsc->rivet_child_init_script = NULL;
    rsc->rivet_child_exit_script = NULL;
    rsc->rivet_before_script = NULL;
    rsc->rivet_after_script = NULL;
    rsc->rivet_error_script = NULL;

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
    rsc->request_init = NULL;
    rsc->request_cleanup = NULL;

    rsc->headers_printed = ap_pcalloc(p, sizeof(int));
    rsc->headers_set = ap_pcalloc(p, sizeof(int));
    rsc->content_sent = ap_pcalloc(p, sizeof(int));
    *(rsc->headers_printed) = 0;
    *(rsc->headers_set) = 0;
    *(rsc->content_sent) = 0;
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

    /* Merge the allowed directory options. */
    new->rivet_before_script = add->rivet_before_script ?
	add->rivet_before_script : base->rivet_before_script;
    new->rivet_after_script = add->rivet_after_script ?
	add->rivet_after_script : base->rivet_after_script;
    new->rivet_error_script = add->rivet_error_script ?
	add->rivet_error_script : base->rivet_error_script;

    /* Merge the tables of dir and user variables. */
    new->rivet_dir_vars =
	ap_overlay_tables( p, base->rivet_dir_vars, add->rivet_dir_vars );
    new->rivet_user_vars =
	ap_overlay_tables( p, base->rivet_user_vars, add->rivet_user_vars );

    return new;
}

void *
Rivet_MergeConfig(pool *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);
    rivet_server_conf *base = (rivet_server_conf *) basev;
    rivet_server_conf *overrides = (rivet_server_conf *) overridesv;

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

    if( rsc->rivet_child_exit_script == NULL ) return;

    if ( Tcl_EvalObjEx(rsc->server_interp, rsc->rivet_child_exit_script, 0)
	!= TCL_OK) {
	ap_log_error(APLOG_MARK, APLOG_ERR, s,
		     "Problem running child exit script: %s",
		     Tcl_GetStringFromObj(rsc->rivet_child_exit_script, NULL));
    }
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
