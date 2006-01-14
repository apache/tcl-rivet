/* rivetChannel.c -- describes the mod_rivet Tcl output channel. */

/* Copyright 2002-2004 The Apache Software Foundation

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

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"

#include <tcl.h>
#include <errno.h>

//#include "apache_request.h"
#include "mod_rivet.h"
#include "TclWeb.h"

static int
inputproc(ClientData instancedata, char *buf, int toRead, int *errorCodePtr)
{
    return EINVAL;
}

/* This is the output 'method' for the Memory Buffer Tcl 'File'
   Channel that we create to divert stdout to. */

static int
outputproc(ClientData instancedata, CONST84 char *buf,
	   int toWrite, int *errorCodePtr)
{
    rivet_server_conf *rsc = (rivet_server_conf *)instancedata;
    rivet_interp_globals *globals =
        Tcl_GetAssocData(rsc->server_interp, "rivet", NULL);

    TclWeb_PrintHeaders(globals->req);
    if (globals->req->content_sent == 0)
    {
        ap_rwrite(buf, toWrite, globals->r);
        ap_rflush(globals->r);
    }
    return toWrite;
}

static int
closeproc(ClientData instancedata, Tcl_Interp *interp)
{
    return 0;
}

static int
setoptionproc(ClientData instancedata, Tcl_Interp *interp,
	      CONST84 char *optionname, CONST84 char *value)
{
    return TCL_OK;
}

static void
watchproc(ClientData instancedata, int mask)
{
    /* not much to do here */
    return;
}

static int
gethandleproc(ClientData instancedata, int direction, ClientData *handlePtr)
{
    return TCL_ERROR;
}

Tcl_ChannelType RivetChan = {
    "apache_channel",
    NULL,
    closeproc,
    inputproc,
    outputproc,
    NULL,
    setoptionproc,
    NULL,
    watchproc,
    gethandleproc,
    NULL,
    NULL
};

