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
apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

INCLUDES = -I@apache_include@ -I@RIVET_BASE_INCLUDE@ 

# ... and these two.  We want to put the libs in the package path,
# rather than the standard library location for the system.
##lib_libexecdir = @TCL_PACKAGE_PATH@/rivet
##lib_libexec_LTLIBRARIES = librivet.la librivetparser.la

#
# Apache Rivet Module, mod_rivet.so / mod_rivet.dll
#
mod_rivet_la_SOURCES = 			\
	apache_multipart_buffer.c 	\
	apache_request.c			\
	mod_rivet.c 				\
	TclWebapache.c 				\
	rivetCore.c					\
	../rivetChannel.c 			\
	../rivetParser.c

##mod_rivet_la_SOURCES = mod_rivet.c mod_rivet.h

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@  @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@
mod_rivet_la_CPPFLAGS = @TCL_INCLUDES@  @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@  @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

# APXS_CPPFLAGS_SHLIB

# Removing libtool .la files from installation

install-data-hook:
	echo 'removing unwanted libtool .la files'
	rm -fv $(DESTDIR)/$(apxs_libexecdir)/*.la


