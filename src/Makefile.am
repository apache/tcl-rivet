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

#SUBDIRS = librivet @apache_version_dir@ 

lib_libexecdir = @RIVET_TCL_TARGET@
lib_libexec_LTLIBRARIES = librivetparser.la librivetlib.la 

#
# Rivet Parser Library
#
librivetparser_la_SOURCES = parser/rivetParser.c parser/parserPkgInit.c
librivetparser_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
librivetparser_la_LIBADD  = @APXS_LIBS@
librivetparser_la_CPPFLAGS = @apache_include@ -I@apache_version_dir@  @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS

#
# Rivet Library
#
librivetlib_la_SOURCES = librivet/rivetList.c librivet/rivetCrypt.c librivet/rivetWWW.c librivet/rivetPkgInit.c
librivetlib_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
librivetlib_la_LIBADD  = @APXS_LIBS@
librivetlib_la_CPPFLAGS = -I@apache_version_dir@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS 

#
# mod_rivet
#
apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
mod_rivet_la_CPPFLAGS = -I@apache_version_dir@ @apache_include@ -I@RIVET_BASE_INCLUDE@ -Iparser @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'


mod_rivet_la_SOURCES = 	@apache_version_dir@/apache_multipart_buffer.c 	\
	                    @apache_version_dir@/apache_request.c			\
	                    @apache_version_dir@/mod_rivet.c 				\
	                    @apache_version_dir@/TclWebapache.c 			\
                        @apache_version_dir@/apache_config.c            \
	                    rivetcmds/rivetCore.c                           \
                        rivetcmds/rivetInspect.c                        \
	                    rivetChannel.c                                  \
                        parser/rivetParser.c

# Removing libtool .la files from installation

install-data-hook:
	echo 'removing unwanted libtool .la files'
	rm -f $(DESTDIR)/$(lib_libexecdir)/*.la
	rm -f $(DESTDIR)/$(apxs_libexecdir)/*.la

