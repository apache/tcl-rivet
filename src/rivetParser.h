#ifndef RIVETPARSER_H
#define RIVETPARSER_H 1

#define STARTING_SEQUENCE "<?"
#define ENDING_SEQUENCE "?>"

int Rivet_GetRivetFile(char *filename, int toplevel,
		       Tcl_Obj *outbuf, Tcl_Interp *interp);

int Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, Tcl_Interp *interp);

#endif /* RIVETPARSER_H */
