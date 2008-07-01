
/* This is for windows. */
#ifdef BUILD_rivet
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_rivet */

#define STREQU(s1, s2)  (s1[0] == s2[0] && strcmp(s1, s2) == 0)
#define STRNEQU(s1, s2) (s1[0] == s2[0] && strncmp(s1, s2, strlen(s2)) == 0)

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

EXTERN int Rivet_Init( Tcl_Interp *interp );
EXTERN int Rivet_InitList( Tcl_Interp *interp );
EXTERN int Rivet_InitCrypt( Tcl_Interp *interp );
EXTERN int Rivet_InitWWW( Tcl_Interp *interp );
EXTERN int Rivet_InitCore( Tcl_Interp *interp );
