#define STREQU(s1, s2) (s1[0] == s2[0] && strcmp(s1, s2) == 0)

#define TCL_CMD_HEADER(cmd)	\
static int cmd(\
    ClientData clientData,\
    Tcl_Interp *interp,\
    int objc,\
    Tcl_Obj *CONST objv[])

#define TCL_OBJ_CMD( name, func ) \
Tcl_CreateObjCommand( interp, /* Tcl interpreter */\
		      name,   /* Function name in Tcl */\
		      func,   /* C function name */\
		      NULL,   /* Client Data */\
		      (Tcl_CmdDeleteProc *)NULL /* Tcl Delete Prov */)

int Rivet_Init( Tcl_Interp *interp );
int Rivet_InitList( Tcl_Interp *interp );
int Rivet_InitCrypt( Tcl_Interp *interp );
int Rivet_InitWWW( Tcl_Interp *interp );
int Rivet_InitCore( Tcl_Interp *interp );
