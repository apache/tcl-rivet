# Makefile for creating libTclMagick and libTkMagick

# Copyright David N. Welton <davidw@dedasys.com>, 2004, with many
# thanks to Bob Friesenhahn for all the help.

# AM_CFLAGS = @TCL_INCLUDES@ @GM_CPPFLAGS@

lib_LTLIBRARIES = mod_rivet.la mod_rivet.la
mod_rivet_la_SOURCES = apache_multipart_buffer.c rivetList.c TclWeb.c apache_request.c rivetChannel.c rivetParser.c mod_rivet.c rivetCore.c rivetWWW.c rivetCrypt.c TclWebapache.c

mod_rivet_la_LDFLAGS = @TCL_LDFLAGS@ -no-undefined -module
mod_rivet_la_LIBADD = @TCL_LIBS@
mod_rivet_la_CPPFLAGS = @TCL_INCLUDES@ @TCL_CPPFLAGS@

