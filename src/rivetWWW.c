/*
 * rivetWWW.c - Rivet commands designed for use with the world wide web.
 */

#include <tcl.h>
#include <ctype.h>
#include "rivet.h"

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_HexToDigit --
 *     Helper function to convert a hex character into the equivalent integer.
 *
 * Results:
 *     The integer, or -1 if an illegal hex character is encountered.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_HexToDigit(int c) {

    if (c >= 'a' && c <= 'f') {
	return (c - 'a' + 10);
    }

    if (c >= 'A' && c <= 'F') {
	return (c - 'A' + 10);
    }

    if (c >= '0' && c <= '9') {
	return (c - '0');
    }
    return (-1);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_UnescapeStringCmd --
 *     Implements the TCL unescape_string command:
 *         unescape_string string
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_UnescapeStringCmd )
{
    char *origString, *newString, *origStringP, *newStringP;
    int  origLength;
    int digit1, digit2;

    if ( objc != 2 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "string" );
	return TCL_ERROR;
    }

    origString = Tcl_GetStringFromObj( objv[1], &origLength );
    newString = Tcl_Alloc( (unsigned)origLength + 1);

    /* for all the characters in the source string */
    for (origStringP = origString, newStringP = newString;
	 *origStringP != '\0';
	 origStringP++) {
	 char c = *origStringP;
	 char c2;

         /* map plus to space */
	 if (c == '+') {
	     *newStringP++ = ' ';
	     continue;
	 }

         /* if there's a percent sign, decode the two-character
	  * hex sequence that follows and copy it to the target
	  * string */
	 if (c == '%') {
	     digit1 = Rivet_HexToDigit(c = *++origStringP);
	     digit2 = Rivet_HexToDigit(c2 = *++origStringP);

	    if (digit1 == -1 || digit2 == -1) {
		char buf[2];
		snprintf( buf, 2, "%c%c", c, c2 );
		Tcl_AppendResult( interp,
			Tcl_GetStringFromObj( objv[0], NULL ),
			": bad char in hex sequence %", buf, (char *)NULL );
		return TCL_ERROR;
	     }

	     *newStringP++ = (digit1 * 16 + digit2);
	     continue;
	 }

         /* it wasn't a plus or percent, just copy the char across */
	 *newStringP++ = c;
    }
    /* Don't forget to null-terminate the target string */
    *newStringP = '\0';

    Tcl_SetObjResult( interp, Tcl_NewStringObj( newString, -1 ) );
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_DigitToHex
 *     Helper function to convert a number 0 - 15 into the equivalent hex
 *     character.
 *
 * Results:
 *     The integer, or -1 if an illegal hex character is encountered.
 *
 *-----------------------------------------------------------------------------
 */
static int
Rivet_DigitToHex(int c) {

    if (c < 10) {
        return c + '0';
    }
    return c - 10 + 'a';
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EscapeStringCmd --
 *     Implements the TCL escape_string command:
 *         escape_string string
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_EscapeStringCmd )
{
    char *origString, *newString, *origStringP, *newStringP;
    int origLength;

    if ( objc != 2 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "string" );
	return TCL_ERROR;
    }

    origString = Tcl_GetStringFromObj( objv[1], &origLength );

    /* If they sent us an empty string, we're done */
    if (origLength == 0) return TCL_OK;

    newString = (char *)Tcl_Alloc( (unsigned)origLength * 3 + 1 );

    /* for all the characters in the source string */
    for (origStringP = origString, newStringP = newString;
	*origStringP != '\0';
	origStringP++) {
	char c = *origStringP;

        if (isalnum ((int)c)) {
	    *newStringP++ = c;
	} else {
	    *newStringP++ = '%';
	    *newStringP++ = Rivet_DigitToHex((c >> 4) & 0x0f);
	    *newStringP++ = Rivet_DigitToHex(c & 0x0f);
	}
    }
    /* Don't forget to null-terminate the target string */
    *newStringP = '\0';

    Tcl_SetObjResult( interp, Tcl_NewStringObj( newString, -1 ) );
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EscapeSgmlCharsCmd --
 *     Implements the TCL escape_sgml_chars command:
 *         escape_sgml_chars string
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_EscapeSgmlCharsCmd )
{
    char *origString, *newString, *origStringP, *newStringP;
    int origLength;

    if( objc != 2 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "string" );
	return TCL_ERROR;
    }

    origString = Tcl_GetStringFromObj( objv[1], &origLength );

    /* If they sent us an empty string, we're done */
    if (origLength == 0) return TCL_OK;

    newString = (char *)Tcl_Alloc( (unsigned)origLength * 3 + 1 );

    /* for all the characters in the source string */
    for (origStringP = origString, newStringP = newString;
	*origStringP != '\0';
	origStringP++) {
	char c = *origStringP;

	switch(c) {
	    case '&':
		*newStringP++ = '&';
		*newStringP++ = 'a';
		*newStringP++ = 'm';
		*newStringP++ = 'p';
		*newStringP++ = ';';
		break;
	    case '<':
		*newStringP++ = '&';
		*newStringP++ = 'l';
		*newStringP++ = 't';
		*newStringP++ = ';';
		break;
	    case '>':
		*newStringP++ = '&';
		*newStringP++ = 'g';
		*newStringP++ = 't';
		*newStringP++ = ';';
		break;
	    case '\'':
		*newStringP++ = '&';
		*newStringP++ = '#';
		*newStringP++ = '3';
		*newStringP++ = '9';
		*newStringP++ = ';';
		break;
	    case '"':
		*newStringP++ = '&';
		*newStringP++ = 'q';
		*newStringP++ = 'u';
		*newStringP++ = 'o';
		*newStringP++ = 't';
		*newStringP++ = ';';
		break;
	    default:
		*newStringP++ = c;
		break;
	}
    }
    /* Don't forget to null-terminate the target string */
    *newStringP = '\0';

    Tcl_SetObjResult( interp, Tcl_NewStringObj( newString, -1 ) );
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EscapeShellCommandCmd --
 *     Implements the TCL www_escape_shell_command command:
 *         www_escape_shell_command string
 *
 * Results:
 *     Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_EscapeShellCommandCmd )
{
    char *origString, *newString, *origStringP, *newStringP, *checkP;
    int  origLength;

    if( objc != 2) {
	Tcl_WrongNumArgs( interp, 1, objv, "string" );
	return TCL_ERROR;
    }

    origString = Tcl_GetStringFromObj( objv[1], &origLength );

    newString = Tcl_Alloc( (unsigned)origLength * 2 + 1 );

    /* for all the characters in the source string */
    for (origStringP = origString, newStringP = newString;
	*origStringP != '\0';
	origStringP++) {
	char c = *origStringP;

	/* if the character is a shell metacharacter, quote it */
	for (checkP = "&;`'|*?-~<>^()[]{}$\\"; *checkP != '\0'; checkP++) {
	    if (c == *checkP) {
		*newStringP++ = '\\';
		break;
	    }
	}

	*newStringP++ = c;
    }
    /* Don't forget to null-terminate the target string */
    *newStringP = '\0';

    Tcl_SetObjResult( interp, Tcl_NewStringObj( newString, -1 ) );
    return TCL_OK;
}

/*     
 *-----------------------------------------------------------------------------
 * Rivet_InitWWW --
 *     
 *   Initialize the WWW functions.
 *-----------------------------------------------------------------------------
 */  
int
Rivet_InitWWW( interp )
    Tcl_Interp *interp;
{
    Tcl_CreateObjCommand(interp,
			"unescape_string",
			Rivet_UnescapeStringCmd,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			"escape_string",
			Rivet_EscapeStringCmd,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			"escape_sgml_chars",
			Rivet_EscapeSgmlCharsCmd,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp,
			"escape_shell_command",
			Rivet_EscapeShellCommandCmd,
			NULL,
			(Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
