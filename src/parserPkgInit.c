/*
 * parserPkgInit.c - This code provides a wrapper around
 * rivetParser.c, in order to expose it to Tcl scripts not running in
 * Rivet.
 */

/* Copyright 2003-2004 The Apache Software Foundation

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

/*
 *-----------------------------------------------------------------------------
 *
 * Parse_RivetData --
 *
 * 	Takes a Rivet script as an argument, and returns the parsed
 * 	tcl script version.
 *
 * Results:
 *	A normal Tcl result.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

static int
Parse_RivetData(ClientData clientData, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[])
{
    Tcl_Obj *outbuf;

    outbuf = Tcl_NewObj();

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "data");
	return TCL_ERROR;
    }
    Tcl_IncrRefCount(outbuf);

    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    if (Rivet_Parser(outbuf, objv[1]) == 0)
    {
	Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    Tcl_SetObjResult(interp, outbuf);
    Tcl_DecrRefCount(outbuf);
    return TCL_OK;
}

/* Package init for standalone parser package.  */

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

EXTERN int
Rivetparser_SafeInit( Tcl_Interp *interp )
{
    /* rivet::parserivet is DEFINITELY unsafe -- it takes a filename,
     * and code in safe interpreters could possibly manipulate it
     * to read files */

    /* but after inspect we find that rivet::parserivetdata appears
     * to be safe -- it reads a string and returns a new string derived
     * from the string it read, using Tcl C calls to construct the
     * target string, which should by design prevent buffer overflow
     * attacks, etc.
     */
    Tcl_CreateObjCommand(interp,
			 "rivet::parserivetdata",
			 Parse_RivetData,
			 NULL,
			 (Tcl_CmdDeleteProc *)NULL);

    return Tcl_PkgProvide( interp, "rivetparser", "0.2" );
}
