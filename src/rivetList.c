#include <tcl.h>
#include "rivet.h"


/*-----------------------------------------------------------------------------
 * Rivet_LremoveObjCmd --
 *   Implements the lremove command:
 *       lremove ?-exact|-glob|-regexp? -all list pattern ..?pattern?..?pattern?
 *-----------------------------------------------------------------------------
 */
static int
Rivet_LremoveObjCmd( clientData, interp, objc, objv )
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj    *CONST objv[];
{
#define EXACT   0
#define GLOB    1
#define REGEXP  2
    int listObjc, i, j, match, mode, patternLen, valueLen;
    int list, all, done, append;
    char *modeStr, *pattern, *value;
    Tcl_Obj **listObjv, *matchedListPtr = NULL;

    if( objc < 3 ) {
	Tcl_WrongNumArgs( interp, 1, objv,
	    "?mode? ?-all? list ?pattern?.. ?pattern?..");
	return TCL_ERROR;
    }

    list = 1;
    all = 0;
    mode = GLOB;

    /* Check arguments to see if they are switches.
     * Switches: -glob, -regexp, -exact, -all
     */
    for( i = 1; i < objc; ++i )
    {
	modeStr = Tcl_GetStringFromObj(objv[i], NULL);
	if( modeStr[0] != '-' ) break;

	if( STREQU(modeStr, "-exact") ) {
	    mode = EXACT;
	    list++;
	} else if( STREQU(modeStr, "-glob") ) {
	    mode = GLOB;
	    list++;
	} else if( STREQU(modeStr, "-regexp") ) {
	    mode = REGEXP;
	    list++;
	} else if( STREQU(modeStr, "-all") ) {
	    all = 1;
	    list++;
	} else if( STREQU(modeStr, "--") ) {
	    list++;
	    break;
	} else {
	    Tcl_AppendResult( interp, "bad switch \"", modeStr,
				"\": must be -exact, -glob, -regexp or -all",
				(char *)NULL);
	    return TCL_ERROR;
	}
	    
    }

    if( list >= objc - 1 )  {
	Tcl_WrongNumArgs(interp, 1, objv, "?mod? ?-all? list pattern");
	return TCL_ERROR;
    }

    if( Tcl_ListObjGetElements(interp, objv[list],
                               &listObjc, &listObjv) != TCL_OK)
        return TCL_ERROR;

    done = 0;
    for(i = 0; i < listObjc; i++)
    {
        match = 0;
	value = Tcl_GetStringFromObj(listObjv[i], &valueLen);

	/* We're done.  Append the rest of the elements and return */
	if( done ) {
	    if (matchedListPtr == NULL) {
		matchedListPtr = Tcl_NewListObj(0, NULL);
	    }
	    if (Tcl_ListObjAppendElement(interp, matchedListPtr,
					 listObjv[i]) != TCL_OK)
		goto errorExit;
	    
	    continue;
	}

	append = list + 1;
	for( j = list + 1; j < objc; ++j )
	{
	    pattern = Tcl_GetStringFromObj(objv[j], &patternLen);
	    if( (mode != EXACT) && (strlen(pattern) != (size_t)patternLen) ) {
		goto binData;
	    }

	    switch(mode) {
	      case EXACT:
		match = (valueLen == patternLen) &&
		    (memcmp(value, pattern, valueLen) == 0);
		break;

	      case GLOB:
		if( strlen(value) != (size_t)valueLen ) {
		    goto binData;
		}
		match = Tcl_StringMatch(value, pattern);
		break;

	      case REGEXP:
		if( strlen(value) != (size_t)valueLen ) {
		    goto binData;
		}
		match = Tcl_RegExpMatch(interp, value, pattern);
		if( match < 0 ) {
		    goto errorExit;
		}
		break;
	    }
	    /* It's not in the pattern we're looking for.
	     * Check the next pattern.
	     */
	    if( !match ) {
		append++;
		continue;
	    }

	    /* We found a match, and we're not looking for anymore */
	    if( !all ) {
		done = 1;
		break;
	    }
	}

	/* We're done.  Append the rest of the elements and return */
	if( done ) continue;

	/* If append is equal to j, the value made it through all the patterns
	 * without matching, so we append it to our return list.
	 */
	if( append == j ) {
	    if (matchedListPtr == NULL) {
		matchedListPtr = Tcl_NewListObj(0, NULL);
	    }
	    if (Tcl_ListObjAppendElement(interp, matchedListPtr,
					 listObjv[i]) != TCL_OK)
		goto errorExit;
	}
    }

    if( matchedListPtr != NULL ) {
        Tcl_SetObjResult(interp, matchedListPtr);
    }
    return TCL_OK;
    
  errorExit:
    if(matchedListPtr != NULL)
        Tcl_DecrRefCount(matchedListPtr);
    return TCL_ERROR;

  binData:
    Tcl_AppendResult(interp, "Binary data is not supported in this mode.",
				(char *) NULL);
    return TCL_ERROR;
}


/*-----------------------------------------------------------------------------
 * Rivet_initList --
 *   Initialize the list commands in an interpreter.
 *
 * Parameters:
 *   o interp - Interpreter to add commands to.
 *-----------------------------------------------------------------------------
 */
int
Rivet_InitList( interp )
    Tcl_Interp *interp;
{
    Tcl_CreateObjCommand(interp,
			 "lremove",
			 Rivet_LremoveObjCmd, 
                         (ClientData) NULL,
			 (Tcl_CmdDeleteProc*) NULL);
    return TCL_OK;
}
