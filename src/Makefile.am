# Makefile for creating libTclMagick and libTkMagick

# Copyright 2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# AM_CFLAGS = @TCL_INCLUDES@ @GM_CPPFLAGS@

apxs_libexecdir = @APXS_LIBEXECDIR@

lib_LTLIBRARIES = librivet.la librivetparser.la

apxs_libexec_LTLIBRARIES = mod_rivet.la

#
# Apache Rivet Module, mod_rivet.so / mod_rivet.dll
#
mod_rivet_la_SOURCES = apache_multipart_buffer.c apache_request.c rivetChannel.c rivetParser.c rivetCore.c mod_rivet.c TclWebapache.c mod_rivet.h

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ -no-undefined -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
mod_rivet_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

# APXS_CPPFLAGS_SHLIB


#
# Rivet Library
#
librivet_la_SOURCES = rivetList.c rivetCrypt.c rivetWWW.c rivetPkgInit.c
librivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ -no-undefined -module -avoid-version
librivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
librivet_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

#
# Rivet Parser Library
#
librivetparser_la_SOURCES = rivetParser.c parserPkgInit.c
librivetparser_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ -no-undefined -module -avoid-version
librivetparser_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
librivetparser_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'


# librivet.so
# librivetparser.so
# ld -shared -x -o librivetparser.so rivetParser.o parserPkgInit.o -L/usr/local/lib -ltcl84 -lm
# ld -shared -x -o librivet.so rivetList.o rivetCrypt.o rivetWWW.o rivetPkgInit.o -L/usr/local/lib -ltcl84 -lm -lcrypt


#