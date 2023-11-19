/* rivetChannel.c -- mod_rivet Tcl output channel. */

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
#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"

#include <tcl.h>
#include <errno.h>

#include "mod_rivet.h"
#include "TclWeb.h"

static int
RivetChan_InputProc(ClientData instancedata, char *buf, int toRead, int *errorCodePtr)
{
    return EINVAL;
}

/* This is the output 'method' for the Memory Buffer Tcl 'File'
   Channel that we create to divert stdout to. */

static int
RivetChan_OutputProc(ClientData instancedata, const char *buf, int toWrite, int *errorCodePtr)
{
    apr_threadkey_t*        rivet_thread_key = (apr_threadkey_t*) instancedata;
    rivet_thread_private*   private;

    apr_threadkey_private_get ((void **)&private,rivet_thread_key);

    TclWeb_PrintHeaders(private->req);
    if (private->req->content_sent == 0)
    {
        ap_rwrite(buf, toWrite, private->r);
        ap_rflush(private->r);
    }

    return toWrite;
}

static int
RivetChan_CloseProc(ClientData instancedata, Tcl_Interp *interp)
{
    return 0;
}

static int
RivetChan_Close2Proc(ClientData instancedata, Tcl_Interp *interp, int flags)
{
    return 0;
}


static int
RivetChan_SetOptionProc(ClientData instancedata,Tcl_Interp *interp,
	      const char *optionname,const char *value)
{
    return TCL_OK;
}

static void
RivetChan_WatchProc(ClientData instancedata, int mask)
{
    /* not much to do here */
    return;
}

static int
RivetChan_GetHandleProc(ClientData instancedata, int direction, ClientData *handlePtr)
{
    return TCL_ERROR;
}

Tcl_ChannelType RivetChan = {
    "apache_channel",           /* typeName */
    TCL_CHANNEL_VERSION_5,      /* channel type version */
    RivetChan_CloseProc,        /* close proc */
    RivetChan_InputProc,        /* input proc */
    RivetChan_OutputProc,       /* output proc */
    NULL,                       /* seek proc - can be null */
    RivetChan_SetOptionProc,    /* set option proc - can be null */
    NULL,                       /* get option proc - can be null */
    RivetChan_WatchProc,        /* watch proc */
    RivetChan_GetHandleProc,    /* get handle proc */
    RivetChan_Close2Proc,       /* close 2 proc - can be null */
    NULL,                       /* block mode proc - can be null */
    NULL,                       /* flush proc - can be null */
    NULL,                       /* handler proc - can be null */
    NULL,                       /* wide seek proc - can be null if seekproc is*/
    NULL,                       /* thread action proc - can be null */
    NULL                        /* truncate proc */
};
