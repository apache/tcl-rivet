#define STREQU(s1, s2) (s1[0] == s2[0] && strcmp(s1, s2) == 0)

int Rivet_Init( Tcl_Interp *interp );
int Rivet_InitList( Tcl_Interp *interp );
int Rivet_InitCore( Tcl_Interp *interp );
