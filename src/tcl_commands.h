int Rivet_MakeURL(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Parse(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Include(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_BufferAdd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Hputs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Headers(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Buffered(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Hflush(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_HGetVars(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Var(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Upload(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_NoBody(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_init( Tcl_Interp *interp );
