#include "httpd.h"
#include "http_config.h"
#include "http_request.h"

#include <tcl.h>
#include <errno.h>

#include "apache_request.h"
#include "mod_rivet.h"
#include "TclWeb.h"

/* This file describes the mod_rivet Tcl output channel. */

static int
inputproc(ClientData instancedata, char *buf, int toRead, int *errorCodePtr)
{
    return EINVAL;
}

/* This is the output 'method' for the Memory Buffer Tcl 'File'
   Channel that we create to divert stdout to. */

static int
outputproc(ClientData instancedata, char *buf, int toWrite, int *errorCodePtr)
{
    rivet_server_conf *rsc = (rivet_server_conf *)instancedata;
    rivet_interp_globals *globals =
	Tcl_GetAssocData(rsc->server_interp, "rivet", NULL);

    TclWeb_PrintHeaders(globals->req);
    if (*(rsc->content_sent) == 0)
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
		 char *optionname, char *value)
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
