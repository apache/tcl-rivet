#ifndef RIVETPARSER_H
#define RIVETPARSER_H 1

/* This bit is for windows. */
#ifdef BUILD_rivet
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_rivet */

#define STARTING_SEQUENCE "<?"
#define ENDING_SEQUENCE "?>"

EXTERN int Rivet_GetRivetFile(char *filename, int toplevel,
		       Tcl_Obj *outbuf, Tcl_Interp *interp);

EXTERN int Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, Tcl_Interp *interp);

#endif /* RIVETPARSER_H */
