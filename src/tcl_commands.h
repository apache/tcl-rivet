int MakeURL(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Parse(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Include(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Buffer_Add(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Hputs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Headers(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Buffered(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int HFlush(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int HGetVars(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Var(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Upload(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int Rivet_Info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

int No_Body(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

