/* Copyright 2002-2004 The Apache Software Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   	http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef RIVETPARSER_H
#define RIVETPARSER_H 1

#define START_TAG "<?"
#define END_TAG "?>"

/* This bit is for windows. */
#ifdef BUILD_rivet
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_rivet */

EXTERN int Rivet_GetRivetFile(char *filename, int toplevel,
		       Tcl_Obj *outbuf, Tcl_Interp *interp);

EXTERN int Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, Tcl_Interp *interp);

EXTERN int Rivet_Parser(Tcl_Obj *outbuf, Tcl_Obj *inbuf);


#endif /* RIVETPARSER_H */
