#include <tcl.h>
#include "rivet.h"

int
Rivet_Init( Tcl_Interp *interp )
{
    Rivet_InitCore( interp );

    Rivet_InitList( interp );

    return TCL_OK;
}
