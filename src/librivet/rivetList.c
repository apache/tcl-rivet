/*
 * rivetList.c - Rivet commands that manipulate lists.
 */

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

/* Code originally from TclX by Karl Lehenbauer and others. */

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <tcl.h>
#include <string.h>
/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivet.h"

static Tcl_ObjCmdProc Rivet_LremoveObjCmd;
static Tcl_ObjCmdProc Rivet_CommaSplitObjCmd;
static Tcl_ObjCmdProc Rivet_CommaJoinObjCmd;
static Tcl_ObjCmdProc Rivet_LassignArrayObjCmd;

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
                         listObjv[i]) != TCL_OK) {
                goto errorExit;
            }
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
                        (memcmp(value, pattern, (unsigned)valueLen) == 0);
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

static void
Rivet_ListObjAppendString (interp, targetList, string, length)
    Tcl_Interp *interp;
    Tcl_Obj    *targetList;
    char       *string;
    int         length;
{
    Tcl_Obj    *elementObj;

    elementObj = Tcl_NewStringObj (string, length);
    Tcl_ListObjAppendElement (interp, targetList, elementObj);
    /* Tcl_DecrRefCount (elementObj); */
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CommaObjSplitCmd --
 *
 * Implements the `comma_split' Tcl command:
 *    comma_split $line
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_CommaSplitObjCmd (notUsed, interp, objc, objv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj   *CONST objv[];
{
    char        *first, *next;
    char         c;
    int          stringLength;
    Tcl_Obj     *resultList;

    /* ??? need a way to set this */
    /* true if two quotes ("") in the body of a field maps to one (") */
    int          quotequoteQuotesQuote = 1;

    /* true if quotes within strings not followed by a comma are allowed */
    int          quotePairsWithinStrings = 1;

    if( objc != 2 ) {
    Tcl_WrongNumArgs( interp, 1, objv, "string" );
    return TCL_ERROR;
    }

    /* get access to a textual representation of the object */
    first = Tcl_GetStringFromObj (objv [1], &stringLength);

    /* handle the trivial case... if the string is empty, so is the result */
    if (stringLength == 0) return TCL_OK;

    next = first;
    resultList = Tcl_GetObjResult (interp);

    /* this loop walks through the comma-separated string we've been passed */
    while (1) {

    /* grab the next character in the buffer */
        c = *next;

    /* if we've got a quote at this point, it is at the start
     * of a field, scan to the closing quote, make that a field, 
     * and update */

        if (c == '"') {
            next = ++first;
            while (1) {
                c = *next;
            /*
             * if we're at the end, we've got an unterminated quoted string
             */
                if (c == '\0') goto format_error;

                    /*
             * If we get a double quote, first see if it's a pair of double 
             * quotes, i.e. a quoted quote, and handle that.
             */
                if (c == '"') {
                /* if consecutive pairs of quotes as quotes of quotes
                 * is enabled and the following char is a double quote,
                 * turn the pair into a single by zooming on down */
                    if (quotequoteQuotesQuote && (*(next + 1) == '"')) {
                        next += 2;
                        continue;
                    }

                /* If double quotes within strings is enabled and the
                 * char following this quote is not a comma, scan forward
                 * for a quote */
                    if (quotePairsWithinStrings && (*(next + 1) != ',')) {
                        next++;
                        continue;
                    }
                /* It's a solo double-quote, not a pair of double-quotes, 
                 * so terminate the element
                 * at the current quote (the closing quote).
                 */
                    Rivet_ListObjAppendString (interp,resultList, first, next - first);

                /* skip the closing quote that we overwrote, and the
                 * following comma if there is one.
                 */

                    ++next;
                    c = *next;

                /* 
                 * if we get end-of-line here, it's fine... and we're done
                 */

                    if (c == '\0') return TCL_OK;

                /*
                 * It's not end-of-line.  If the next character is
                 * not a comma, it's an error.
                 */
                    if (c != ',') {
                      format_error:
                        Tcl_ResetResult (interp);
                        Tcl_AppendResult (interp,
                                  "format error in string: \"", 
                                   first, "\"", (char *) NULL);
                        return TCL_ERROR;
                    }

                /* We're done with that field.  The next one starts one
                 * character past the current one, which is (was) a
                 * comma */
                    first = ++next;
                    break;
                }
            /* It wasn't a quote, look at the next character. */
                next++;
            }
            continue;
        }

    /* If we get here, we're at the start of a field that didn't
     * start with a quote
     */

        next = first;
        while (1) {
            c = *next;

            /* If we reach end of the string, append the last element
             * and return to our caller. 
             */

            if (c == '\0') {
                Rivet_ListObjAppendString (interp, resultList, first, -1);
                return TCL_OK;
            }

            /* If we get a comma, that's the end of this piece,
             * stick it into the list.
             */
            if (c == ',') {
                Rivet_ListObjAppendString (interp,
                      resultList,
                      first, next - first);
                first = ++next;
                break;
            }
            next++;
        }
    }
    Rivet_ListObjAppendString (interp, resultList, first, -1);
    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_CommaJoinCmd --
 *
 * Implements the `comma_join' Tcl command:
 *    comma_join $list
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_CommaJoinObjCmd (notUsed, interp, objc, objv)
    ClientData   notUsed;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj   *CONST objv[];
{
    int         listObjc;
    Tcl_Obj   **listObjv;
    int         listIdx, didField;
    Tcl_Obj    *resultPtr;
    char       *walkPtr;
    char       *strPtr;
    int         stringLength;

    if( objc != 2 ) {
    Tcl_WrongNumArgs( interp, 1, objv,
            "list arrayName elementName ?elementName..?" );
        return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult (interp);

    if (Tcl_ListObjGetElements  (interp, 
                 objv[1], 
                 &listObjc, 
                 &listObjv) != TCL_OK) {
        return TCL_ERROR;
    }

    didField = 0;
    for (listIdx = 0; listIdx < listObjc; listIdx++) {
        /* If it's the first thing we've output, start it out
         * with a double quote.  If not, terminate the last
         * element with a double quote, then put out a comma,
         * then open the next element with a double quote
         */
        if (didField) {
            Tcl_AppendToObj (resultPtr, "\",\"", 3);
        } else {
            Tcl_AppendToObj (resultPtr, "\"", 1);
            didField = 1;
        }
        walkPtr = strPtr  = Tcl_GetStringFromObj (listObjv[listIdx], &stringLength);
        /* Walk the string of the list element that we're about to
         * append to the result object.
         *
         * For each character, if it isn't a double quote, move on to
         * the next character until the string is exhausted.
         */
        for (;stringLength; stringLength--) {
            if (*walkPtr++ != '"') continue;

            /* OK, we saw a double quote.  Emit everything up to and
             * including the double quote, then reset the string to
             * start at the same double quote (to issue it twice and
             * pick up where we left off.  Be sure to get the length
             * calculations right!
             */

             Tcl_AppendToObj (resultPtr, strPtr, walkPtr - strPtr);
             strPtr = walkPtr - 1;
        }
        Tcl_AppendToObj (resultPtr, strPtr, walkPtr - strPtr);
    }
    Tcl_AppendToObj (resultPtr, "\"", 1);
    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_LassignArrayCmd --
 *     Implements the TCL lassign_array command:
 *         lassign_array list arrayname elementname ?elementname...?
 *
 * Results:
 *      Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_LassignArrayObjCmd )
{
    int     listObjc, listIdx, idx;
    Tcl_Obj **listObjv;
    Tcl_Obj *varValue;

    if( objc < 4 ) {
    Tcl_WrongNumArgs( interp, 1, objv,
            "list arrayName elementName ?elementName..?");
        return TCL_ERROR;
    }

    if( Tcl_ListObjGetElements(interp, objv[1],
                               &listObjc, &listObjv) != TCL_OK)
        return TCL_ERROR;

    for (idx = 3, listIdx = 0; idx < objc; idx++, listIdx++) {
    varValue = (listIdx < listObjc) ?
        listObjv[listIdx] : Tcl_NewStringObj("", -1);

    if (Tcl_ObjSetVar2( interp, objv[2], objv[idx],
                varValue, TCL_LEAVE_ERR_MSG ) == NULL) {
            return TCL_ERROR;
        }
    }

    /* We have some left over items.  Return them in a list. */
    if( listIdx < listObjc ) {
        Tcl_Obj *list = Tcl_NewListObj( 0, NULL );
        int i;

        for( i = listIdx; i < listObjc; ++i )
        {
            if (Tcl_ListObjAppendElement(interp, list, listObjv[i]) != TCL_OK) {
            return TCL_ERROR;
            }
        }
        Tcl_SetObjResult( interp, list );
    }
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Rivet_initList --
 *
 *   Initialize the list commands in an interpreter.
 *
 *   These routines have been examined and are believed to be safe in a safe
 *   interpreter, as they only manipulate Tcl lists, strings, and arrays.
 *
 * Parameters:
 *   o interp - Interpreter to add commands to.
 *   o rivet_ns - Tcl_Namespace pointer to the RIVET_NS namespace.
 *
 *-----------------------------------------------------------------------------
 */

#if RIVET_NAMESPACE_EXPORT == 1
extern Tcl_Namespace* Rivet_GetNamespace( Tcl_Interp* interp);
#endif

int 
Rivet_InitList( Tcl_Interp *interp)
{
    RIVET_OBJ_CMD("lremove",Rivet_LremoveObjCmd,NULL);
    RIVET_OBJ_CMD("comma_split",Rivet_CommaSplitObjCmd,NULL);
    RIVET_OBJ_CMD("comma_join",Rivet_CommaJoinObjCmd,NULL);
    RIVET_OBJ_CMD("lassign_array",Rivet_LassignArrayObjCmd,NULL);

#if RIVET_NAMESPACE_EXPORT == 1
    {
        Tcl_Namespace* rivet_ns = Rivet_GetNamespace(interp);
        RIVET_EXPORT_CMD(interp,rivet_ns,"lremove");
        RIVET_EXPORT_CMD(interp,rivet_ns,"comma_split");
        RIVET_EXPORT_CMD(interp,rivet_ns,"comma_join");
        RIVET_EXPORT_CMD(interp,rivet_ns,"lassign_array");
    }
#endif

    return TCL_OK;
}
