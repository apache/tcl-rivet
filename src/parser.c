/* $Id$

   Rivet parser - doesn't really need any of the includes besides
   tcl.h.
  */
#include <tcl.h>
#include "httpd.h"
#include "apache_request.h"
#include "mod_rivet.h"

/* 
   accepts an 'outbuf' to be filled, and an open file descritptor

   returns 'inside', letting the caller know whether the parser was
   inside a block of Tcl or not when it stopped.
  */

int rivet_parser(Tcl_Obj *outbuf, FILE *openfile)
{
    const char *strstart = STARTING_SEQUENCE;
    const char *strend = ENDING_SEQUENCE;

    char c;
    int ch;
    int endseqlen = strlen(ENDING_SEQUENCE), startseqlen = strlen(STARTING_SEQUENCE), p = 0;
    int inside = 0;
    Tcl_DString dstr;
    Tcl_DString convdstr;

    Tcl_DStringInit(&dstr);

    while ((ch = getc(openfile)) != EOF)
    {
	if (ch == -1)
	    return -1;
	c = ch;
	if (!inside)
	{
	    /* OUTSIDE  */

#if USE_OLD_TAGS == 1
	    if (c == '<')
	    {
		int nextchar = getc(openfile);
		if (nextchar == '+')
		{
		    Tcl_DStringAppend(&dstr, "\"\n", 2);
		    inside = 1;
		    p = 0;
		    continue;
		} else {
		    ungetc(nextchar, openfile);
		}
	    }
#endif

	    if (c == strstart[p])
	    {
		if ((++p) == endseqlen)
		{
		    /* ok, we have matched the whole ending sequence - do something  */
		    Tcl_DStringAppend(&dstr, "\"\n", 2);
		    inside = 1;
		    p = 0;
		    continue;
		}
	    } else {
		if (p > 0)
		    Tcl_DStringAppend(&dstr, (char *)strstart, p);
		/* or else just put the char in outbuf  */
		switch (c)
		{
		case '{':
		    Tcl_DStringAppend(&dstr, "\\{", -1);
		    break;
		case '}':
		    Tcl_DStringAppend(&dstr, "\\}", -1);
		    break;
		case '$':
		    Tcl_DStringAppend(&dstr, "\\$", -1);
		    break;
		case '[':
		    Tcl_DStringAppend(&dstr, "\\[", -1);
		    break;
		case ']':
		    Tcl_DStringAppend(&dstr, "\\]", -1);
		    break;
		case '"':
		    Tcl_DStringAppend(&dstr, "\\\"", -1);
		    break;
		case '\\':
		    Tcl_DStringAppend(&dstr, "\\\\", -1);
		    break;
		default:
		    Tcl_DStringAppend(&dstr, &c, 1);
		    break;
		}
		p = 0;
		continue;
	    }
	} else {
	    /* INSIDE  */

#if USE_OLD_TAGS == 1
	    if (c == '+')
	    {
		int nextchar = getc(openfile);
		if (nextchar == '>')
		{
		    Tcl_DStringAppend(&dstr, "\n hputs \"", -1);
		    inside = 0;
		    p = 0;
		    continue;
		} else {
		    ungetc(nextchar, openfile);
		}
	    }
#endif

	    if (c == strend[p])
	    {
		if ((++p) == startseqlen)
		{
		    Tcl_DStringAppend(&dstr, "\n hputs \"", -1);
		    inside = 0;
		    p = 0;
		    continue;
		}
	    }
	    else
	    {
		/*  plop stuff into outbuf, which we will then eval   */
		if (p > 0)
		    Tcl_DStringAppend(&dstr, (char *)strend, p);
		Tcl_DStringAppend(&dstr, &c, 1);
		p = 0;
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
