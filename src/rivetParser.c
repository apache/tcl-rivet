/*
 * Rivet parser.
 *
 * Contains functions for loading up Tcl scripts either in flat Tcl
 * files, or in Rivet .rvt files.
 *
 */

/* $Id$ */

#include <tcl.h>
#include "mod_rivet.h"
#include "rivetParser.h"

static int Rivet_Parser(Tcl_Obj *outbuf, Tcl_Obj *inbuf);

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_GetTclFile --
 *
 * Takes a filename, an outbuf to fill in with a Tcl script, and a
 * TclWebRequest.  Fills in outbuf with a Tcl script.
 *
 *-----------------------------------------------------------------------------
 */

int
Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, TclWebRequest *req)
{
    int result = 0;

    /* Taken, in part, from tclIOUtil.c out of the Tcl distribution,
     * and modified.
     */

    Tcl_Channel chan = Tcl_OpenFileChannel(req->interp, filename, "r", 0644);
    if (chan == (Tcl_Channel) NULL)
    {
	Tcl_ResetResult(req->interp);
	Tcl_AppendResult(req->interp, "couldn't read file \"", filename,
			 "\": ", Tcl_PosixError(req->interp), (char *) NULL);
	return TCL_ERROR;
    }
    result = Tcl_ReadChars(chan, outbuf, -1, 1);
    if (result < 0)
    {
	Tcl_Close(req->interp, chan);
	Tcl_AppendResult(req->interp, "couldn't read file \"", filename,
			 "\": ", Tcl_PosixError(req->interp), (char *) NULL);
	return TCL_ERROR;
    }

    if (Tcl_Close(req->interp, chan) != TCL_OK)
	return TCL_ERROR;

    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_GetRivetFile --
 *
 * Takes a filename, a flag to indicate whether we are operating at
 * the top level (not from within the "parse" command), a buffer to
 * fill in, and a TclWebRequest.  Fills in outbuf with a parsed Rivet
 * .rvt file, creating a Tcl script ready for execution.
 *
 *-----------------------------------------------------------------------------
 */

int
Rivet_GetRivetFile(char *filename, int toplevel,
		   Tcl_Obj *outbuf, TclWebRequest *req)
{
    int inside = 0;	/* are we inside the starting/ending delimiters  */
    int sz = 0;
    Tcl_Obj *inbuf;
    Tcl_Channel rivetfile;

    rivetfile = Tcl_OpenFileChannel(req->interp, filename, "r", 0664);
    if (rivetfile == NULL) {
	Tcl_AddErrorInfo(req->interp, Tcl_PosixError(req->interp));
        return TCL_ERROR;
    }

    if (toplevel) {
	Tcl_AppendToObj(outbuf, "namespace eval request {\n", -1);
    } else {
	Tcl_SetStringObj(outbuf, "", -1);
    }
    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    inbuf = Tcl_NewObj();
    sz = Tcl_ReadChars(rivetfile, inbuf, -1, 0);

    Tcl_Close(req->interp, rivetfile);
    if (sz == -1)
    {
	Tcl_AddErrorInfo(req->interp, Tcl_PosixError(req->interp));
	return TCL_ERROR;
    }

    inside = Rivet_Parser(outbuf, inbuf);

    if (inside == 0)
    {
	Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    if (toplevel)
    {
	Tcl_AppendToObj(outbuf, "\n}", -1);
    }
    Tcl_AppendToObj(outbuf, "\n", -1);

    /* END PARSER  */
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Parser --
 *
 * Parses data (from .rvt file) in inbuf and creates resulting script
 * in outbuf.
 *
 * Results:
 *
 * Returns 'inside' - whether we were still inside a block of Tcl code
 * or not, when the parser hit the end of the data.
 *
 *-----------------------------------------------------------------------------
 */

static int
Rivet_Parser(Tcl_Obj *outbuf, Tcl_Obj *inbuf)
{
    char *next;
    char *cur;
    const char *strstart = STARTING_SEQUENCE;
    const char *strend = ENDING_SEQUENCE;


    int endseqlen = strlen(ENDING_SEQUENCE);
    int startseqlen = strlen(STARTING_SEQUENCE);
    int inside = 0, p = 0;
    int inLen = 0;

    next = Tcl_GetStringFromObj(inbuf, &inLen);

    if (inLen == 0)
	return 0;

    while (*next != 0)
    {
	cur = next;
	next = Tcl_UtfNext(cur);
	if (!inside)
	{
	    /* Outside the delimiting tags. */
	    if (*cur == strstart[p])
	    {
		if ((++p) == startseqlen)
		{
		    /* We have matched the whole ending sequence. */
		    Tcl_AppendToObj(outbuf, "\"\n", 2);
		    inside = 1;
		    p = 0;
		    cur = Tcl_UtfNext(cur);
		    continue;
		}
	    } else {
		if (p > 0) {
		    Tcl_AppendToObj(outbuf, (char *)strstart, p);
		    p = 0;
		}
		/* or else just put the char in outbuf  */
		switch (*cur)
		{
		case '{':
		    Tcl_AppendToObj(outbuf, "\\{", 2);
		    break;
		case '}':
		    Tcl_AppendToObj(outbuf, "\\}", 2);
		    break;
		case '$':
		    Tcl_AppendToObj(outbuf, "\\$", 2);
		    break;
		case '[':
		    Tcl_AppendToObj(outbuf, "\\[", 2);
		    break;
		case ']':
		    Tcl_AppendToObj(outbuf, "\\]", 2);
		    break;
		case '"':
		    Tcl_AppendToObj(outbuf, "\\\"", 2);
		    break;
		case '\\':
		    Tcl_AppendToObj(outbuf, "\\\\", 2);
		    break;
		default:
		    Tcl_AppendToObj(outbuf, cur, next - cur);
		    break;
		}
		continue;
	    }
	} else {
	    /* Inside the delimiting tags. */

	    if (*cur == strend[p])
	    {
		if ((++p) == endseqlen)
		{
		    Tcl_AppendToObj(outbuf, "\n puts -nonewline \"", -1);
		    inside = 0;
		    p = 0;
		}
	    } else {
		/* Plop stuff into outbuf, which we will then eval. */
		if (p > 0) {
		    Tcl_AppendToObj(outbuf, (char *)strend, p);
		    p = 0;
		}
		Tcl_AppendToObj(outbuf, cur, next - cur);
	    }
	}
    }

    return inside;
}
