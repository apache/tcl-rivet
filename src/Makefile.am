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

#SUBDIRS = @rivet_core@

lib_libexecdir = @RIVET_TCL_TARGET@
lib_libexec_LTLIBRARIES = librivetparser.la librivetlib.la 

#
# Rivet Parser Library
#
librivetparser_la_SOURCES = parser/rivetParser.c parser/parserPkgInit.c
librivetparser_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
librivetparser_la_LIBADD  = @APXS_LIBS@
librivetparser_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS

#
# Rivet Library
#
librivetlib_la_SOURCES = librivet/rivetList.c librivet/rivetCrypt.c librivet/rivetWWW.c librivet/rivetPkgInit.c
librivetlib_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
librivetlib_la_LIBADD  = @APXS_LIBS@
librivetlib_la_CPPFLAGS = -I@rivet_core@ -I@apache_request@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS 

apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

mod_rivet_la_SOURCES =  @rivet_core@/mod_rivet.c                            \
                        @apache_request@/apache_multipart_buffer.c 	        \
                        @apache_request@/apache_request.c                   \
                        @rivet_commands@/rivetCore.c                        \
                        @rivet_commands@/rivetInspect.c                     \
                        @rivet_channel@/rivetChannel.c                      \
                        parser/rivetParser.c                                \
                        @rivet_core@/TclWebapache.c                         \
                        @rivet_core@/apache_config.c                        \
                        @rivet_core@/mod_rivet_common.c

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
mod_rivet_la_CPPFLAGS = -I@rivet_core@ -I@rivet_channel@ -I@RIVET_BASE_INCLUDE@/parser -I@apache_request@ @apache_include@ -I@RIVET_BASE_INCLUDE@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

lib_libmpmdir = @RIVET_TCL_TARGET@/mpm
lib_libmpm_LTLIBRARIES = rivet_worker_mpm.la rivet_prefork_mpm.la rivet_aprthread_mpm.la

rivet_worker_mpm_la_SOURCES = experimental/rivet_worker_mpm.c
rivet_worker_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_worker_mpm_la_LIBADD  = @APXS_LIBS@
rivet_worker_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I@apache_request@ -I@RIVET_BASE_INCLUDE@

rivet_prefork_mpm_la_SOURCES = experimental/rivet_prefork_mpm.c
rivet_prefork_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_prefork_mpm_la_LIBADD  = @APXS_LIBS@
rivet_prefork_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I@apache_request@ -I@RIVET_BASE_INCLUDE@

rivet_aprthread_mpm_la_SOURCES = experimental/rivet_aprthread_mpm.c
rivet_aprthread_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_aprthread_mpm_la_LIBADD  = @APXS_LIBS@
rivet_aprthread_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I@apache_request@ -I@RIVET_BASE_INCLUDE@

# Removing libtool .la files from installation

install-data-hook:
	echo 'removing unwanted libtool .la files'
	rm -f $(DESTDIR)/$(lib_libexecdir)/*.la
	rm -f $(DESTDIR)/$(apxs_libexecdir)/*.la

