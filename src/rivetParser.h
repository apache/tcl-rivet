#ifndef RIVETPARSER_H
#define RIVETPARSER_H 1


int Rivet_GetRivetFile(char *filename, int toplevel,
		   Tcl_Obj *outbuf, TclWebRequest *req);

int Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, TclWebRequest *req);

#endif /* RIVETPARSER_H */
