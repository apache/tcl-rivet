/*
 * parserPkgInit.c - This code provides a wrapper around rivetParser.c,
 * in order to expose it to Tcl scripts not running in Rivet.
 */

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

/* $Id$ */

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <apr_tables.h>
#include <apr_file_io.h>
#include <tcl.h>

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivet.h"
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
    int     tclcode;

    outbuf = Tcl_NewObj();
    Tcl_IncrRefCount(outbuf);

    if (objc != 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

#if RIVET_CORE == mod_rivet_ng
    Tcl_AppendToObj(outbuf, "namespace eval request {\n", -1);
    tclcode = Rivet_GetRivetFile(Tcl_GetString(objv[1]),outbuf,interp);
    if (tclcode == TCL_ERROR) return TCL_ERROR;
    Tcl_AppendToObj(outbuf, "\n}\n", -1);
#else
    tclcode = Rivet_GetRivetFile(Tcl_GetString(objv[1]),1,outbuf,interp);
    if (tclcode == TCL_ERROR) return TCL_ERROR;
#endif

    Tcl_SetObjResult(interp, outbuf);
    Tcl_DecrRefCount(outbuf);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Parse_RivetData --
 *
 *      Takes a Rivet script as an argument, and returns the parsed
 *      tcl script version.
 *
 * Results:
 *      A normal Tcl result.
 *
 * Side Effects:
 *      None.
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

DLLEXPORT int
Rivetparser_Init( Tcl_Interp *interp )
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL)
#endif
    {
        return TCL_ERROR;
    }

    RIVET_OBJ_CMD("parserivet",Parse_Rivet,NULL);
    RIVET_OBJ_CMD("parserivetdata",Parse_RivetData,NULL);
    return Tcl_PkgProvide( interp, "rivetparser", "0.2" );
}

DLLEXPORT int
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

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL)
#endif
    {
        return TCL_ERROR;
    }

/*
    Tcl_CreateObjCommand(interp,
                         "rivet::parserivetdata",
                         Parse_RivetData,
                         NULL,
                         (Tcl_CmdDeleteProc *)NULL);
*/

    RIVET_OBJ_CMD("parserivetdata",Parse_RivetData,NULL);
    return Tcl_PkgProvide( interp, "rivetparser", "0.2" );
}
