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

static Tcl_Namespace* 
Rivet_GetNamespace( Tcl_Interp* interp)
{
    rivet_interp_globals *globals; 
    Tcl_Namespace *rivet_ns;

    globals = Tcl_GetAssocData(interp, "rivet", NULL);
    if (globals != NULL)
    {
//      fprintf(stderr,"Associated data found, getting Rivet ns from mod_rivet\n");
        rivet_ns = globals->rivet_ns;
    }
    else
    {
//      fprintf(stderr,"no Associated data found, running standalone\n");
        rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,(Tcl_NamespaceDeleteProc *)NULL);
    }

    return rivet_ns;
}


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
    Tcl_Namespace *rivet_ns = Rivet_GetNamespace(interp);

    Rivet_InitList ( interp, rivet_ns );
    Rivet_InitCrypt( interp, rivet_ns );
    Rivet_InitWWW  ( interp, rivet_ns );

    return Tcl_PkgProvide( interp, "RivetLib", "1.2" );
}


/*-----------------------------------------------------------------------------
 * Rivetlib_SafeInit --
 *   Install the commands provided by librivet that are believed to be
 *   safe for use in safe interpreters, into a safe interpreter.
 *
 * Parameters:
 *   o interp - Interpreter to add commands to.
 *-----------------------------------------------------------------------------
 */

int
Rivetlib_SafeInit( Tcl_Interp *interp )
{
    Tcl_Namespace *rivet_ns = Rivet_GetNamespace(interp);

    Rivet_InitList( interp, rivet_ns );
    Rivet_InitCrypt( interp, rivet_ns );
    Rivet_InitWWW( interp, rivet_ns );

    return Tcl_PkgProvide( interp, "RivetLib", "1.2" );
}

