/*
 * parserPkgInit.c - This code provides a wrapper around
 * rivetParser.c, in order to expose it to Tcl scripts not running in
 * Rivet.
 */

/* $Id$ */

#include <tcl.h>
#include "rivetParser.h"

/*
 *-----------------------------------------------------------------------------
 *
 * Parse_Rivet --
 *
 * Provides glue between Rivet_GetRivetFile and tcl command
 * rivet::parserivet.
 *
 *-----------------------------------------------------------------------------
 */

static int
Parse_Rivet(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    Tcl_Obj *outbuf;

    outbuf = Tcl_NewObj();
    Tcl_IncrRefCount(outbuf);

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }

    if (Rivet_GetRivetFile(Tcl_GetString(objv[1]),
			   1, outbuf, interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, outbuf);
    Tcl_DecrRefCount(outbuf);
    return TCL_OK;
}


int
Rivetparser_Init( Tcl_Interp *interp )
{
    Tcl_CreateObjCommand(interp,
			 "rivet::parserivet",
			 Parse_Rivet,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    return Tcl_PkgProvide( interp, "rivetparser", "0.1" );
}
