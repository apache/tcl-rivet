/* $Id$

   Rivet parser - doesn't really need any of the includes besides
   tcl.h.
  */
#include <tcl.h>
#include "httpd.h"
#include "apache_request.h"
#include "mod_rivet.h"

/*
   Accepts an 'outbuf' to be filled, and an open file descriptor

   Returns 'inside', letting the caller know whether the parser was
   inside a block of Tcl or not when it stopped.
  */

int
Rivet_Parser( Tcl_Obj *outbuf, FILE *openfile )
{
    const char *strstart = STARTING_SEQUENCE;
    const char *strend = ENDING_SEQUENCE;

    char c;
    int ch;
    int endseqlen = strlen(ENDING_SEQUENCE);
    int startseqlen = strlen(STARTING_SEQUENCE);
    int inside = 0, p = 0;
    Tcl_DString dstr;
    Tcl_DString convdstr;

    Tcl_DStringInit(&dstr);

    while ((ch = getc(openfile)) != EOF)
    {
	if (ch == -1)
	    return -1;
	c = (char)ch;
	if (!inside)
	{
	    /* Outside the delimiting tags. */
	    if (c == strstart[p])
	    {
		if ((++p) == endseqlen)
		{
		    /* We have matched the whole ending sequence.
		     */
		    Tcl_DStringAppend(&dstr, "\"\n", 2);
		    inside = 1;
		    p = 0;
		    continue;
		}
	    } else {
		if (p > 0) {
		    Tcl_DStringAppend(&dstr, (char *)strstart, p);
		    p = 0;
		}
		/* or else just put the char in outbuf  */
		switch (c)
		{
		case '{':
		    Tcl_DStringAppend(&dstr, "\\{", 2);
		    break;
		case '}':
		    Tcl_DStringAppend(&dstr, "\\}", 2);
		    break;
		case '$':
		    Tcl_DStringAppend(&dstr, "\\$", 2);
		    break;
		case '[':
		    Tcl_DStringAppend(&dstr, "\\[", 2);
		    break;
		case ']':
		    Tcl_DStringAppend(&dstr, "\\]", 2);
		    break;
		case '"':
		    Tcl_DStringAppend(&dstr, "\\\"", 2);
		    break;
		case '\\':
		    Tcl_DStringAppend(&dstr, "\\\\", 2);
		    break;
		default:
		    Tcl_DStringAppend(&dstr, &c, 1);
		    break;
		}
		continue;
	    }
	} else {
	    /* Inside the delimiting tags. */

	    if (c == strend[p])
	    {
		if ((++p) == startseqlen)
		{
		    Tcl_DStringAppend(&dstr, "\n puts \"", -1);
		    inside = 0;
		    p = 0;
		    continue;
		}
	    } else {
		/*  plop stuff into outbuf, which we will then eval   */
		if (p > 0) {
		    Tcl_DStringAppend(&dstr, (char *)strend, p);
		    p = 0;
		}
		Tcl_DStringAppend(&dstr, &c, 1);
	    }
	}
    }

    Tcl_ExternalToUtfDString(NULL,
			     Tcl_DStringValue(&dstr),
			     Tcl_DStringLength(&dstr),
			     &convdstr);

    Tcl_AppendToObj(outbuf, Tcl_DStringValue(&convdstr),
		    Tcl_DStringLength(&convdstr));
    Tcl_DStringFree(&dstr);
    Tcl_DStringFree(&convdstr);
    return inside;
}
