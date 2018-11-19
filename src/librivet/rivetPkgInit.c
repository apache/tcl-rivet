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
/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivet.h"
//#include "mod_rivet.h"

/*-----------------------------------------------------------------------------
 * Rivet_GetNamespace --
 *
 *   Get the rivet namespace pointer. The procedure attempts to retrieve a
 *  pointer to the Tcl_Namespace structure for the ::rivet namespace. This
 *  pointer is stored in the interpreter's associated data (pointing to a
 *  rivet_interp_globals structure) if the interpreter is passed by mod_rivet.
 *  Otherwise a new ::rivet namespace is created and its Tcl_Namespace
 *  pointer is returned 
 *
 * Parameters:
 *
 *   o interp - Interpreter to add commands to.
 *
 * Returned value:
 *
 *   o rivet_ns - Tcl_Namespace* pointer to the RIVET_NS (::rivet) namespace
 *
 *-----------------------------------------------------------------------------
 */

#if RIVET_NAMESPACE_EXPORT == 1

Tcl_Namespace* 
Rivet_GetNamespace( Tcl_Interp* interp)
{
    Tcl_Namespace *rivet_ns;

    rivet_ns = Tcl_FindNamespace(interp,RIVET_NS,NULL,TCL_GLOBAL_ONLY);
    if (rivet_ns == NULL) 
    {
        rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,
                                        (Tcl_NamespaceDeleteProc *)NULL);
    }

    return rivet_ns;
}
#endif

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

DLLEXPORT int
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
 *
 *   Install the commands provided by librivet that are believed to be
 *   safe for use in safe interpreters, into a safe interpreter.
 *
 * Parameters:
 *
 *   o interp - Interpreter to add commands to.
 *
 *-----------------------------------------------------------------------------
 */

DLLEXPORT int
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
