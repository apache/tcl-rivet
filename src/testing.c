/* Testing code for C routines. */
/* $Id$ */

/* Use this as a place to more directly test C routines, without going
 * to the trouble of installing/compiling the whole thin in mod_rivet,
 * and using with Apache. */


#include <tcl.h>
#include "httpd.h"

int
Rivet_Parser_Test
(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    Tcl_Obj *outbuf = NULL;
    FILE *testfile = NULL;

    if (objc != 2)
    {
	Tcl_WrongNumArgs(interp, 1, objv, "filename");
	return TCL_ERROR;
    }

    testfile = fopen(Tcl_GetString(objv[1]), "r");
    if (testfile == NULL)
    {
	Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
	return TCL_ERROR;
    }
    outbuf = Tcl_NewStringObj("", 0);
    Rivet_Parser(outbuf, testfile);
    return TCL_OK;
}

int
Cleanup
(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    printf("cleaning up\n");
    return TCL_OK;
}

int
Testing_Init(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "parsertest", Rivet_Parser_Test,
			 (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "cleanup", Cleanup,
			 (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_PkgProvide(interp, "rivettesting", "1.0");
    return TCL_OK;
}
