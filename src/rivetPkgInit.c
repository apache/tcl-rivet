/*
 * rivetPkgInit.c - Initialize all of the Rivet commands into a Tcl interp.
 */

#include <tcl.h>
#include "rivet.h"

int
Rivet_Init( Tcl_Interp *interp )
{
    Rivet_InitList( interp );

    Rivet_InitCrypt( interp );

    Rivet_InitWWW( interp );

    return Tcl_PkgProvide( interp, "Rivet", "1.1" );
}
