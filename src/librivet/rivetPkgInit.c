/*
 * rivetPkgInit.c - Initialize all of the Rivet commands into a Tcl interp.
 */

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
#include <rivet_config.h>
#endif

#include <tcl.h>
#include "rivet.h"
#include "mod_rivet.h"

/*-----------------------------------------------------------------------------
 * Rivetlib_Init --
 *
 *   Install the commands provided by librivet into an interpreter.
 *
 * Parameters:
 *
 *   o interp - Interpreter to add commands to.
 *
 *-----------------------------------------------------------------------------
 */

int
Rivetlib_Init( Tcl_Interp *interp )
{

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) { 
#else
	if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) { 
#endif    
	    return TCL_ERROR;
    }

    Rivet_InitList (interp);
    Rivet_InitCrypt(interp);
    Rivet_InitWWW  (interp);

    return Tcl_PkgProvide( interp, RIVETLIB_TCL_PACKAGE, RIVET_VERSION );
}

/*-----------------------------------------------------------------------------
 * Rivetlib_SafeInit --
 *   Install the commands provided by librivet that are believed to be
 *   safe for use in safe interpreters, into a safe interpreter.
 *
 * Parameters:
 *
 *   o interp - Interpreter to add commands to.
 *
 *-----------------------------------------------------------------------------
 */

int
Rivetlib_SafeInit( Tcl_Interp *interp )
{

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) { 
#else
	if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) { 
#endif    
	    return TCL_ERROR;
    }

    Rivet_InitList(interp);
    Rivet_InitCrypt(interp);
    Rivet_InitWWW(interp);

    return Tcl_PkgProvide( interp, RIVETLIB_TCL_PACKAGE, RIVET_VERSION );
}
