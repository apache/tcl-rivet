# Makefile for Rivet

# Copyright 2004-2005 The Apache Software Foundation

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


# The weird and wild magic of auto* associates these two.
##apxs_libexecdir = @APXS_LIBEXECDIR@
##apxs_libexec_LTLIBRARIES = mod_rivet.la

INCLUDES = -I@apache_include@ 

# ... and these two.  We want to put the libs in the package path,
# rather than the standard library location for the system.
lib_libexecdir = @RIVET_TCL_TARGET@
lib_libexec_LTLIBRARIES = librivet.la librivetparser.la

#
# Apache Rivet Module, mod_rivet.so / mod_rivet.dll
#
##mod_rivet_la_SOURCES = @apache_version_dir@/apache_request.c \
##	rivetChannel.c \
##	rivetChannel.h \
##	rivetParser.c \
##	rivetParser.h \
##	TclWeb.h \
##	rivet.h

##mod_rivet_la_SOURCES = mod_rivet.c mod_rivet.h

##mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
##mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
##mod_rivet_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

# APXS_CPPFLAGS_SHLIB

#
# Rivet Library
#
librivet_la_SOURCES = rivetList.c rivetCrypt.c rivetWWW.c rivetPkgInit.c
librivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
librivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
librivet_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

#
# Rivet Parser Library
#
librivetparser_la_SOURCES = rivetParser.c parserPkgInit.c
librivetparser_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
librivetparser_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
librivetparser_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

SUBDIRS = @apache_version_dir@
