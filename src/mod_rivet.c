/* Copyright David Welton 1998, 1999 */

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

/* mod_rivet.c by David Welton <davidw@apache.org> - originally mod_include.  */
/* See http://tcl.apache.org/mod_rivet/credits.ttml for additional credits. */

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_main.h"
#include "util_script.h"
#include "http_conf_globals.h"

#include <tcl.h>
#include <string.h>

#include "tcl_commands.h"
#include "parser.h"
#include "channel.h"
#include "apache_request.h"
#include "mod_rivet.h"

module MODULE_VAR_EXPORT rivet_module;

static void tcl_init_stuff(server_rec *s, pool *p);
static void copy_rivet_config( rivet_server_conf *oldrsc, rivet_server_conf *newrsc);
static int get_ttml_file(request_rec *r, rivet_server_conf *rsc,
			 Tcl_Interp *interp, char *filename, int toplevel, Tcl_Obj *outbuf);
static int send_content(request_rec *);
static int execute_and_check(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *r);

/* just need some arbitrary non-NULL pointer which can't also be a request_rec */
#define NESTED_INCLUDE_MAGIC	(&rivet_module)

#define RIVET_SERVER_CONF(module)	(rivet_server_conf *)ap_get_module_config(module, &rivet_module)

/* Set up the content type header */

int
set_header_type(request_rec *r, char *header)
{
    rivet_server_conf *rsc = rivet_get_conf(r);
    if (*(rsc->headers_set) == 0)
    {
	r->content_type = header;
	*(rsc->headers_set) = 1;
	return 1;
    } else {
	return 0;
    }
}

/* Printer headers if they haven't been printed yet */
int
print_headers(request_rec *r)
{
    rivet_server_conf *rsc = rivet_get_conf(r);
    if (*(rsc->headers_printed) == 0)
    {
	if (*(rsc->headers_set) == 0)
	    set_header_type(r, DEFAULT_HEADER_TYPE);

	ap_send_http_header(r);
	*(rsc->headers_printed) = 1;
	return 1;
    } else {
	return 0;
    }
}

/* Print nice HTML formatted errors */
int
print_error(request_rec *r, int htmlflag, char *errstr)
{
    set_header_type(r, DEFAULT_HEADER_TYPE);
    print_headers(r);

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

/* Make sure that everything in the output buffer has been flushed. */
int
flush_output_buffer(request_rec *r)
{
    rivet_server_conf *rsc = rivet_get_conf(r);
    if (Tcl_DStringLength(rsc->buffer) != 0)
    {
	ap_rwrite(Tcl_DStringValue(rsc->buffer), Tcl_DStringLength(rsc->buffer), r);
	Tcl_DStringInit(rsc->buffer);
    }
    *(rsc->content_sent) = 1;
    return 0;
}

/* Function to convert strings to UTF encoding */
char *
StringToUtf(char *input, ap_pool *pool)
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
rivet_upload_hook(void *ptr, char *buf, int len, ApacheUpload *upload)
{
    Tcl_Interp *interp = ptr;
    static int usenum = 0;
    static int uploaded = 0;

    if (oldptr != upload)
    {
    } else {
    }

#if USE_ONLY_UPLOAD_COMMAND == 0

    Tcl_ObjSetVar2(interp,
		   Tcl_NewStringObj("::request::UPLOAD", -1),
		   Tcl_NewStringObj("data", -1),
		   Tcl_DuplicateObj(uploadstorage[usenum]),
		   0);
#endif /* USE_ONLY_UPLOAD_COMMAND  */
    return len;
}
#endif /* 0 */


/* Load, cache and eval a Tcl file  */

static int
get_tcl_file(request_rec *r, Tcl_Interp *interp,
		char *filename, Tcl_Obj *outbuf)
{
    int result = 0;
#if 1
    /* Taken, in part, from tclIOUtil.c out of the Tcl
       distribution, and modified */

    /* Basically, what we are doing here is a Tcl_EvalFile, but
       with the addition of caching code. */
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
#else
    Tcl_EvalFile(interp, r->filename);
#endif /* 1 */
}

/* Parse and execute a ttml file */

static int
get_ttml_file(request_rec *r, rivet_server_conf *rsc, Tcl_Interp *interp,
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
	Tcl_AppendToObj(outbuf, "buffer_add \"", -1);
    }
    else
	Tcl_SetStringObj(outbuf, "hputs \"\n", -1);

    /* if inside < 0, it's an error  */
    inside = rivet_parser(outbuf, f);
    if (inside < 0)
    {
	if (ferror(f))
	{
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
			 "Encountered error in mod_rivet getchar routine while reading %s",
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

/* 	Tcl_AppendToObj(outbuf, "\n}\nnamespace delete request\n", -1); seems redundant */
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
execute_and_check(Tcl_Interp *interp, Tcl_Obj *outbuf, request_rec *r)
{
    char *errorinfo;
    rivet_server_conf *conf = NULL;

    conf = rivet_get_conf(r);
    if (Tcl_EvalObj(interp, outbuf) == TCL_ERROR)
    {
	Tcl_Obj *errscript = conf->rivet_error_script ? conf->rivet_error_script :
	    conf->rivet_error_script ? conf->rivet_error_script : NULL;

        print_headers(r);
        flush_output_buffer(r);
        if (errscript)
        {
	    if (Tcl_EvalObj(interp, errscript) == TCL_ERROR)
                print_error(r, 1, "<b>Tcl_ErrorScript failed!</b>");
        } else {
            /* default action  */
            errorinfo = Tcl_GetVar(interp, "errorInfo", 0);
            print_error(r, 0, errorinfo);
            print_error(r, 1, "<p><b>OUTPUT BUFFER:</b></p>");
            print_error(r, 0, Tcl_GetStringFromObj(outbuf, (int *)NULL));
        }
/*                  "</pre><b>OUTPUT BUFFER</b><pre>\n",
                    Tcl_GetStringFromObj(outbuf, (int *)NULL));  */
    } else {
        /* We make sure to flush the output if buffer_add was the only output */
        print_headers(r);
        flush_output_buffer(r);
    }
    return OK;
}

/* This is a seperate function so that it may be called from 'Parse' */
int 
get_parse_exec_file(request_rec *r, rivet_server_conf *rsc,
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

    /* Look for the script's compiled version. If it's not found,
       create it. */
    if (*(rsc->cache_size))
    {
	hashKey = ap_psprintf(r->pool, "%s%lx%lx%d", filename,
			      mtime, ctime, toplevel);
	entry = Tcl_CreateHashEntry(rsc->objCache, hashKey, &isNew);
    }
    if (isNew || *(rsc->cache_size) == 0)
    {
	outbuf = Tcl_NewObj();
	Tcl_IncrRefCount(outbuf);

	if(!strcmp(r->content_type, "application/x-httpd-tcl") || toplevel == 0)
	{
	    /* It's a TTML file - which we always are if toplevel is
               0, meaning we are in the Parse command */
	    result = get_ttml_file(r, rsc, interp, filename, toplevel, outbuf);
	} else {
	    /* It's a plain Tcl file */
	    result = get_tcl_file(r, interp, filename, outbuf);
	}
	if (result != TCL_OK)
	    return result;

	if (*(rsc->cache_size))
	    Tcl_SetHashValue(entry, (ClientData)outbuf);

	if (*(rsc->cache_free)) {
	    rsc->objCacheList[-- *(rsc->cache_free) ] = strdup(hashKey);
	} else if (*(rsc->cache_size)) { /* if it's zero, we just skip this... */
	    Tcl_HashEntry *delEntry;
	    delEntry = Tcl_FindHashEntry(rsc->objCache,
					 rsc->objCacheList[*(rsc->cache_size) - 1]);
	    Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
	    Tcl_DeleteHashEntry(delEntry);
	    free(rsc->objCacheList[*(rsc->cache_size) - 1]);
	    memmove((rsc->objCacheList) + 1, rsc->objCacheList,
		    sizeof(char *) * (*(rsc->cache_size) -1));
	    rsc->objCacheList[0] = strdup(hashKey);
	}
    } else {
	outbuf = (Tcl_Obj *)Tcl_GetHashValue(entry);
    }
    execute_and_check(interp, outbuf, r);
    return TCL_OK;
}

/* Set things up to execute a file, then execute */
static int
send_content(request_rec *r)
{
    char error[MAX_STRING_LEN];
    char timefmt[MAX_STRING_LEN];

    int errstatus;

    Tcl_Interp *interp;

    rivet_interp_globals *globals = NULL;
    rivet_server_conf *rsc = NULL;
    rsc = rivet_get_conf(r);
    globals = ap_pcalloc(r->pool, sizeof(rivet_interp_globals));
    globals->r = r;
    interp = rsc->server_interp;
    Tcl_SetAssocData(interp, "rivet", NULL, globals);

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

    /* We need to send it as html */
    /*     r->content_type = DEFAULT_HEADER_TYPE;  */

    if (r->header_only)
    {
	set_header_type(r, DEFAULT_HEADER_TYPE);
	print_headers(r);

	return OK;
    }

    ap_cpystrn(error, DEFAULT_ERROR_MSG, sizeof(error));
    ap_cpystrn(timefmt, DEFAULT_TIME_FORMAT, sizeof(timefmt));
    ap_chdir_file(r->filename);

    if (Tcl_EvalObj(interp, rsc->namespacePrologue) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "Could not create request namespace\n");
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
	globals->req->upload_hook = rivet_upload_hook;
    }
#endif

    if ((errstatus = ApacheRequest___parse(globals->req)) != OK)
	return errstatus;

    /* take results and create tcl variables from them */
#if USE_ONLY_VAR_COMMAND == 0
    if (globals->req->parms)
    {
	int i;
	array_header *parmsarray = ap_table_elts(globals->req->parms);
	table_entry *parms = (table_entry *)parmsarray->elts;
	Tcl_Obj *varsobj = Tcl_NewStringObj("::request::VARS", -1);
	for (i = 0; i < parmsarray->nelts; ++i)
	{
	    if (!parms[i].key)
		continue;
	    else {
		/* All this is so that a query like x=1&x=2&x=3 will
                   produce a variable that is a list */
		Tcl_Obj *newkey = STRING_TO_UTF_TO_OBJ(parms[i].key, r->pool);
		Tcl_Obj *newval = STRING_TO_UTF_TO_OBJ(parms[i].val, r->pool);
		Tcl_Obj *oldval = Tcl_ObjGetVar2(interp, varsobj, newkey, 0);

		if (oldval == NULL)
		{
		    Tcl_ObjSetVar2(interp, varsobj, newkey, newval, 0);
		} else {
		    Tcl_Obj *concat[2];
		    concat[0] = oldval;
		    concat[1] = newval;
		    Tcl_ObjSetVar2(interp, varsobj, newkey, Tcl_ConcatObj(2, concat), 0);
		}
	    }
	}

    }
#endif
#if USE_ONLY_UPLOAD_COMMAND == 1
    upload = req->upload;
    /* Loop through uploaded files */
    while (upload)
    {
	char *type = NULL;
	char *channelname = NULL;
	Tcl_Channel chan;

	/* The name of the file uploaded  */
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("::request::UPLOAD", -1),
		       Tcl_NewStringObj("filename", -1),
		       Tcl_NewStringObj(upload->filename, -1),
		       TCL_LIST_ELEMENT|TCL_APPEND_VALUE);

	/* The variable name of the file upload */
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("::request::UPLOAD", -1),
		       Tcl_NewStringObj("name", -1),
		       Tcl_NewStringObj(upload->name, -1),
		       TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
	Tcl_ObjSetVar2(interp,
		       Tcl_NewStringObj("::request::UPLOAD", -1),
		       Tcl_NewStringObj("size", -1),
		       Tcl_NewIntObj(upload->size),
		       TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
	type = (char *)ap_table_get(upload->info, "Content-type");
	if (type)
	{
	    Tcl_ObjSetVar2(interp,
			   Tcl_NewStringObj("::request::UPLOAD", -1),
			   Tcl_NewStringObj("type", -1),
			   Tcl_NewStringObj(type, -1), /* kill end of line */
			   TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
	}
	if (!upload_files_to_var)
	{
	    if (upload->fp != NULL)
	    {
		chan = Tcl_MakeFileChannel((ClientData)fileno(upload->fp), TCL_READABLE);
		Tcl_RegisterChannel(interp, chan);
		channelname = Tcl_GetChannelName(chan);
		Tcl_ObjSetVar2(interp,
			       Tcl_NewStringObj("::request::UPLOAD", -1),
			       Tcl_NewStringObj("channelname", -1),
			       Tcl_NewStringObj(channelname, -1), /* kill end of line */
			       TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
	    }
	}

	upload = upload->next;
    }
#endif /* USE_ONLY_UPLOAD_COMMAND == 1 */

    get_parse_exec_file(r, rsc, r->filename, 1);
    /* reset globals  */
    *(rsc->buffer_output) = 0;
    *(rsc->headers_printed) = 0;
    *(rsc->headers_set) = 0;
    *(rsc->content_sent) = 0;

    return OK;
}

static void
tcl_init_stuff(server_rec *s, pool *p)
{
    int rslt;
    Tcl_Interp *interp;
    rivet_server_conf *rsc = (rivet_server_conf *)
	ap_get_module_config(s->module_config, &rivet_module);
    server_rec *sr;
    /* Initialize TCL stuff  */

    Tcl_FindExecutable(NULL);
    interp = Tcl_CreateInterp();
    rsc->server_interp = interp; /* root interpreter */

    /* Create TCL commands to deal with Apache's BUFFs. */
    *(rsc->outchannel) = Tcl_CreateChannel(&ApacheChan, "apacheout", rsc, TCL_WRITABLE);

    Tcl_SetStdChannel(*(rsc->outchannel), TCL_STDOUT);
    Tcl_SetChannelOption(interp, *(rsc->outchannel), "-buffering", "none");

    Tcl_RegisterChannel(interp, *(rsc->outchannel));
    if (interp == NULL)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, s, "Error in Tcl_CreateInterp, aborting\n");
	exit(1);
    }
    if (Tcl_Init(interp) == TCL_ERROR)
    {
	ap_log_error(APLOG_MARK, APLOG_ERR, s, Tcl_GetStringResult(interp));
	exit(1);
    }
    Rivet_init( interp );
    rsc->namespacePrologue = Tcl_NewStringObj(
	"catch { namespace delete request }\n"
	"namespace eval request { }\n"
	"proc ::request::global { args } { foreach arg $args { uplevel \"::global ::request::$arg\" } }\n", -1);
    Tcl_IncrRefCount(rsc->namespacePrologue);

#if DBG
    ap_log_error(APLOG_MARK, APLOG_ERR, s, "Config string = \"%s\"",
		 Tcl_GetStringFromObj(rsc->rivet_global_init_script, NULL));  /* XXX */
    ap_log_error(APLOG_MARK, APLOG_ERR, s, "Cache size = \"%d\"", *(rsc->cache_size));  /* XXX */
#endif

    if (rsc->rivet_global_init_script != NULL)
    {
	rslt = Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0);
	if (rslt != TCL_OK)
	{
	    ap_log_error(APLOG_MARK, APLOG_ERR, s, "%s",
			 Tcl_GetVar(interp, "errorInfo", 0));
	}
    }

    /* This is what happens if it is not set by the user */
    if(*(rsc->cache_size) < 0)
    {
	if (ap_max_requests_per_child != 0)
	    *(rsc->cache_size) = ap_max_requests_per_child / 2;
	else
	    *(rsc->cache_size) = 10; /* Arbitrary number FIXME */
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
	/* This should set up slave interpreters for other virtual
           hosts */
	if (sr != s) /* not the first one  */
	{
	    myrsc = ap_pcalloc(p, sizeof(rivet_server_conf));
	    ap_set_module_config(sr->module_config, &rivet_module, myrsc);
	    copy_rivet_config( rsc, myrsc );
	    if (rsc->seperate_virtual_interps != 0)
		myrsc->server_interp = NULL;
	} else {
	    myrsc = (rivet_server_conf *) ap_get_module_config(sr->module_config, &rivet_module);
	}
	if (!myrsc->server_interp)
	{
	    myrsc->server_interp = Tcl_CreateSlave(interp, sr->server_hostname, 0);
	    Rivet_init( myrsc->server_interp );
	    Tcl_SetChannelOption(myrsc->server_interp, *(rsc->outchannel), "-buffering", "none");
	    Tcl_RegisterChannel(myrsc->server_interp, *(rsc->outchannel));
	}

	myrsc->server_name = ap_pstrdup(p, sr->server_hostname);
	sr = sr->next;
    }
}

MODULE_VAR_EXPORT void
rivet_init_handler(server_rec *s, pool *p)
{
#if THREADED_TCL == 0
    tcl_init_stuff(s, p);
#endif
#ifndef HIDE_RIVET_VERSION
    ap_add_version_component("mod_rivet/"RIVET_VERSION);
#else
    ap_add_version_component("mod_rivet");
#endif /* !HIDE_RIVET_VERSION */
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

    if ( var == NULL || val == NULL) {
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

    return( NULL );
}

static const char *
Rivet_DirConf( cmd_parms *cmd, rivet_server_conf *rdc, char *var, char *val )
{
    Tcl_Obj *objarg;

    if ( var == NULL || val == NULL) {
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

static const char *
Rivet_UserConf( cmd_parms *cmd, rivet_server_conf *rdc, char *var, char *val )
{
    if ( var == NULL || val == NULL) {
	return "Rivet Error: RivetUserConf requires two arguments";
    }

    ap_table_set( rdc->rivet_user_vars, var, val );
    return( NULL );
}

/* Function to get a config and merge the directory/server options  */
rivet_server_conf *
rivet_get_conf( request_rec *r )
{
    rivet_server_conf *newconfig = NULL;
    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->server->module_config );
    rivet_server_conf *rdc;
    void *dconf = r->per_dir_config;


    /* If there is no per dir config, just return the server config */
    if( dconf == NULL ) return rsc;

    rdc = RIVET_SERVER_CONF( dconf );

    newconfig = (rivet_server_conf *) ap_pcalloc(r->pool,
						    sizeof(rivet_server_conf));
    newconfig->server_interp = rsc->server_interp;

    copy_rivet_config( rsc, newconfig );

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
copy_rivet_config( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
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
    newrsc->namespacePrologue = oldrsc->namespacePrologue;

    newrsc->buffer_output = oldrsc->buffer_output;
    newrsc->headers_printed = oldrsc->headers_printed;
    newrsc->headers_set = oldrsc->headers_set;
    newrsc->content_sent = oldrsc->content_sent;
    newrsc->buffer = oldrsc->buffer;
    newrsc->outchannel = oldrsc->outchannel;
}

static void *
create_rivet_config( pool *p, server_rec *s )
{
    rivet_server_conf *rsc =
	(rivet_server_conf *) ap_pcalloc(p, sizeof(rivet_server_conf));

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
    rsc->namespacePrologue = NULL;

    rsc->buffer_output = ap_pcalloc(p, sizeof(int));
    rsc->headers_printed = ap_pcalloc(p, sizeof(int));
    rsc->headers_set = ap_pcalloc(p, sizeof(int));
    rsc->content_sent = ap_pcalloc(p, sizeof(int));
    *(rsc->buffer_output) = 0;
    *(rsc->headers_printed) = 0;
    *(rsc->headers_set) = 0;
    *(rsc->content_sent) = 0;
    rsc->buffer = ap_pcalloc(p, sizeof(Tcl_DString));
    Tcl_DStringInit(rsc->buffer);
    rsc->outchannel = ap_pcalloc(p, sizeof(Tcl_Channel));
    return rsc;
}

void *
create_rivet_dir_config(pool *p, char *dir)
{
    rivet_server_conf *rdc =
	(rivet_server_conf *) ap_pcalloc(p, sizeof(rivet_server_conf));
    return rdc;
}

void *
merge_rivet_config(pool *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = 
	(rivet_server_conf *) ap_pcalloc(p, sizeof(rivet_server_conf));
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
rivet_child_init(server_rec *s, pool *p)
{
    server_rec *sr;
    rivet_server_conf *rsc;

#if THREADED_TCL == 1
    tcl_init_stuff(s, p);
#endif

    sr = s;
    while(sr)
    {
	rsc = RIVET_SERVER_CONF(sr->module_config);
	if( rsc->rivet_child_init_script != NULL ) 
	{
	    if (Tcl_EvalObjEx(rsc->server_interp, rsc->rivet_child_init_script, 0)
		!= TCL_OK) {
		ap_log_error(APLOG_MARK, APLOG_ERR, s,
			     "Problem running child init script: %s",
			     Tcl_GetString(rsc->rivet_child_init_script));
	    }
	}
	sr = sr->next;
    }
}

void
rivet_child_exit(server_rec *s, pool *p)
{
    rivet_server_conf *rsc = (rivet_server_conf *)
	ap_get_module_config(s->module_config, &rivet_module);

    if( rsc->rivet_child_exit_script == NULL ) return;

    if ( Tcl_EvalObjEx(rsc->server_interp, rsc->rivet_child_exit_script, 0)
	!= TCL_OK) {
	ap_log_error(APLOG_MARK, APLOG_ERR, s,
		     "Problem running child exit script: %s",
		     Tcl_GetStringFromObj(rsc->rivet_child_exit_script, NULL));
    }
}

const handler_rec rivet_handlers[] =
{
    {"application/x-httpd-tcl", send_content},
    {"application/x-rivet-tcl", send_content},
    {NULL}
};

const command_rec rivet_cmds[] =
{
    {"RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, TAKE2, NULL},
    {"RivetDirConf", Rivet_DirConf, NULL, RSRC_CONF, TAKE2, NULL},
    {"RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO, TAKE2,
     "RivetUserConf key value: sets RivetUserConf(key) = value"},
    {NULL}
};

module MODULE_VAR_EXPORT rivet_module =
{
    STANDARD_MODULE_STUFF,
    rivet_init_handler,		/* initializer */
    create_rivet_dir_config,	/* dir config creater */
    NULL,                       /* dir merger --- default is to override */
    create_rivet_config,         /* server config */
    merge_rivet_config,          /* merge server config */
    rivet_cmds,                  /* command table */
    rivet_handlers,		/* handlers */
    NULL,			/* filename translation */
    NULL,			/* check_user_id */
    NULL,			/* check auth */
    NULL,			/* check access */
    NULL,			/* type_checker */
    NULL,			/* fixups */
    NULL,			/* logger */
    NULL,			/* header parser */
    rivet_child_init,            /* child_init */
    rivet_child_exit,            /* child_exit */
    NULL			/* post read-request */
};

/*
  Local Variables: ***
  compile-command: "./make.tcl" ***
  End: ***
*/
