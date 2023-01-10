/*
 * Rivet parser.
 *
 * Contains functions for loading up Tcl scripts either in flat Tcl
 * files, or in Rivet .rvt files.
 *
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

/* $Id$ */

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

#include <string.h>
#include <tcl.h>

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivetParser.h"

int Rivet_Parser(Tcl_Obj *outbuf, Tcl_Obj *inbuf);

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
Rivet_GetTclFile(char *filename, Tcl_Obj *outbuf, Tcl_Interp *interp)
{
    int result = 0;

    /* Taken, in part, from tclIOUtil.c out of the Tcl distribution,
     * and modified.
     */

    Tcl_Channel chan = Tcl_OpenFileChannel(interp, filename, "r", 0644);
    if (chan == (Tcl_Channel) NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "couldn't read file \"", filename,
                         "\": ", Tcl_PosixError(interp), (char *) NULL);
        return TCL_ERROR;
    }
    result = Tcl_ReadChars(chan, outbuf, -1, 1);
    if (result < 0)
    {
        Tcl_Close(interp, chan);
        Tcl_AppendResult(interp, "couldn't read file \"", filename,
                         "\": ", Tcl_PosixError(interp), (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_Close(interp, chan) != TCL_OK)
        return TCL_ERROR;

    return TCL_OK;
}

#if RIVET_CORE == mod_rivet_ng
/*
 *-----------------------------------------------------------------------------
 * Rivet_GetRivetFileNG --
 *
 * The mod_rivet_ng core doesn't assume the parsed script to be
 * enclosed in the ::request namespace. The whole ::request lifecycle is
 * devolved to the Rivet::request_handling procedure
 *
 *-----------------------------------------------------------------------------
 */

int
Rivet_GetRivetFile(char *filename, Tcl_Obj *outbuf, Tcl_Interp *interp)
{
    int sz = 0;
    Tcl_Obj *inbuf;
    Tcl_Channel rivetfile;

    /*
     * we call Tcl to read this file but the caveat exposed in
     * in Rivet_GetRivetFile still holds true (TODO)
     */

    rivetfile = Tcl_OpenFileChannel(interp, filename, "r", 0664);
    if (rivetfile == NULL) {
        /* Don't need to adderrorinfo - Tcl_OpenFileChannel takes care
           of that for us. */
        return TCL_ERROR;
    }

    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);
    inbuf = Tcl_NewObj();
    Tcl_IncrRefCount(inbuf);
    sz = Tcl_ReadChars(rivetfile, inbuf, -1, 0);

    Tcl_Close(interp, rivetfile);
    if (sz == -1)
    {
        Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
        Tcl_DecrRefCount(inbuf);
        return TCL_ERROR;
    }

    /* If we are not inside a <? ?> section, add the closing ". */
    if (Rivet_Parser(outbuf, inbuf) == 0)
    {
        Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    Tcl_DecrRefCount(inbuf);
    /* END PARSER  */
    return TCL_OK;
}

#else /* traditional rivet file processing with enclosion within the ::request namespace */

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
        Tcl_Obj *outbuf, Tcl_Interp *interp)
{
    int sz = 0;
    Tcl_Obj *inbuf;
    Tcl_Channel rivetfile;

    /*
     * TODO There is much switching between Tcl and APR for calling
     * utility routines. We should make up our minds and keep
     * a coherent attitude deciding when Tcl should be called upon
     * and when APR should be invoked instead for a certain class of
     * tasks
     */

    rivetfile = Tcl_OpenFileChannel(interp, filename, "r", 0664);
    if (rivetfile == NULL) {
        /* Don't need to adderrorinfo - Tcl_OpenFileChannel takes care
           of that for us. */
        return TCL_ERROR;
    }

    if (toplevel) {
        Tcl_AppendToObj(outbuf, "namespace eval request {\n", -1);
    } else {
        Tcl_SetStringObj(outbuf, "", -1);
    }
    Tcl_AppendToObj(outbuf, "puts -nonewline \"", -1);

    inbuf = Tcl_NewObj();
    Tcl_IncrRefCount(inbuf);
    sz = Tcl_ReadChars(rivetfile, inbuf, -1, 0);

    Tcl_Close(interp, rivetfile);
    if (sz == -1)
    {
        Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
        Tcl_DecrRefCount(inbuf);
        return TCL_ERROR;
    }

    /* If we are not inside a <? ?> section, add the closing ". */
    if (Rivet_Parser(outbuf, inbuf) == 0)
    {
        Tcl_AppendToObj(outbuf, "\"\n", 2);
    }

    if (toplevel)
    {
        Tcl_AppendToObj(outbuf, "\n}", -1);
    }
    Tcl_AppendToObj(outbuf, "\n", -1);

    Tcl_DecrRefCount(inbuf);
    /* END PARSER  */
    return TCL_OK;
}

#endif

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

int
Rivet_Parser(Tcl_Obj *outbuf, Tcl_Obj *inbuf)
{
    char *next;
    char *cur;
    const char *strstart = START_TAG;
    const char *strend = END_TAG;

    int endseqlen 	= 	(int) strlen(END_TAG);
    int startseqlen = 	(int) strlen(START_TAG);
    int inside = 0, p = 0, check_echo = 0;
    int inLen = 0;

    next = Tcl_GetStringFromObj(inbuf, &inLen);

    if (inLen == 0)
        return 0;

    while (*next != 0)
    {
        cur = next;
        next = (char *)Tcl_UtfNext(cur);
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
                    check_echo = 1;
                    p = 0;
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
                        Tcl_AppendToObj(outbuf, cur, (int)(next - cur));
                        break;
                }
                continue;
            }
        } else {
            /* Inside the delimiting tags. */

            if (check_echo)
            {
                check_echo = 0;
                if (*cur == '=') {
                    Tcl_AppendToObj(outbuf, "\nputs -nonewline ", -1);
                    continue;
                }
            }

            if (*cur == strend[p])
            {
                if ((++p) == endseqlen)
                {
                    Tcl_AppendToObj(outbuf, "\nputs -nonewline \"", -1);
                    inside = 0;
                    p = 0;
                }
            } else {
                /* Plop stuff into outbuf, which we will then eval. */
                if (p > 0) {
                    Tcl_AppendToObj(outbuf, (char *)strend, p);
                    p = 0;
                }
                Tcl_AppendToObj(outbuf, cur, (int)(next - cur));
            }
        }
    }

    //fprintf (stderr, "content:\n%s\n", Tcl_GetString(outbuf));
    //fflush (stderr);

    return inside;
}

