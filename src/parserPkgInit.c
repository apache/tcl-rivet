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

static int
Parse_RivetData(ClientData clientData, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[])
{
    int inside = 0;
    Tcl_Obj *outbuf;

    outbuf = Tcl_NewObj();

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "data");
	return TCL_ERROR;
    }
    Tcl_IncrRefCount(outbuf);

    Tcl_AppendToObj(outbuf, "namespace eval request {\n", -1);
    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    inside = Rivet_Parser(outbuf, objv[1]);

    if (inside == 0)
    {
	Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    Tcl_AppendToObj(outbuf, "\n}\n", -1);

    Tcl_SetObjResult(interp, outbuf);
    Tcl_DecrRefCount(outbuf);
    return TCL_OK;
}

EXTERN int
Rivetparser_Init( Tcl_Interp *interp )
{
    Tcl_CreateObjCommand(interp,
			 "rivet::parserivet",
			 Parse_Rivet,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			 "rivet::parserivetdata",
			 Parse_RivetData,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    return Tcl_PkgProvide( interp, "rivetparser", "0.2" );
}
