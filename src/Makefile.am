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
librivetparser_la_SOURCES = parser/rivetParser.c parser/parserPkgInit.c parser/rivetParser.h
librivetparser_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
librivetparser_la_LIBADD  = @APXS_LIBS@
librivetparser_la_CPPFLAGS = @apache_include@ -I$(srcdir)/@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS

#
# Rivet Library
#
librivetlib_la_SOURCES = librivet/rivetList.c librivet/rivetCrypt.c librivet/rivetWWW.c librivet/rivetPkgInit.c
librivetlib_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ @APU_LDFLAGS@ -module -avoid-version
librivetlib_la_LIBADD  = @APXS_LIBS@
librivetlib_la_CPPFLAGS = -I$(srcdir)/@rivet_core@ -I$(srcdir)/@apache_request@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APU_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS 

apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

mod_rivet_la_SOURCES =  @rivet_core@/mod_rivet.c                        \
                        @apache_request@/apache_multipart_buffer.c 	    \
                        @apache_request@/apache_multipart_buffer.h 	    \
                        @apache_request@/apache_request.c               \
                        @apache_request@/apache_request.h               \
                        @rivet_commands@/rivetCore.c                    \
                        @rivet_commands@/rivetInspect.c                 \
                        @rivet_channel@/rivetChannel.c                  \
                        parser/rivetParser.c                            \
                        @rivet_core@/TclWebapache.c                     \
                        @rivet_core@/apache_config.c                    \
                        @rivet_core@/mod_rivet_common.c                 \
                        @rivet_core@/mod_rivet_generator.c              \
                        @rivet_core@/mod_rivet_cache.c                  \
                        @rivet_core@/apache_config.h                    \
                        @rivet_core@/mod_rivet_cache.h                  \
                        @rivet_core@/mod_rivet_common.h                 \
                        @rivet_core@/mod_rivet_generator.h              \
                        @rivet_core@/mod_rivet.h                        \
                        @rivet_core@/rivetChannel.h                     \
                        @rivet_core@/rivetCore.h                        \
                        @rivet_core@/rivet_types.h                      \
                        @rivet_core@/worker_prefork_common.h            \
                        rivet.h                                         \
                        TclWeb.h

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
mod_rivet_la_CPPFLAGS = -I$(srcdir) \
                        -I$(srcdir)/@rivet_core@ \
                        -I$(srcdir)/@rivet_channel@ \
                        -I$(srcdir)/parser \
                        -I$(srcdir)/@apache_request@ \
                         @apache_include@ -I. @TCL_INCLUDES@ \
                         @APXS_CPPFLAGS@ @APXS_INCLUDES@ \
                         @APR_CPPFLAGS@ @APR_INCLUDES@ \
                         @APU_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

lib_libmpmdir = @RIVET_TCL_TARGET@/mpm
lib_libmpm_LTLIBRARIES = rivet_worker_mpm.la rivet_prefork_mpm.la rivet_lazy_mpm.la

rivet_worker_mpm_la_SOURCES = @rivet_core@/rivet_worker_mpm.c \
                              @rivet_core@/worker_prefork_common.c
rivet_worker_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_worker_mpm_la_LIBADD  = @APXS_LIBS@
rivet_worker_mpm_la_CPPFLAGS = @apache_include@ @TCL_INCLUDES@ \
                               @APXS_CPPFLAGS@ @APXS_INCLUDES@ \
                               @APR_CPPFLAGS@ @APR_INCLUDES@ \
                               @APU_INCLUDES@ \
                              -I$(srcdir) \
                              -I$(srcdir)/@apache_request@ \
                              -I$(srcdir)/@rivet_core@
 
rivet_prefork_mpm_la_SOURCES = @rivet_core@/rivet_prefork_mpm.c \
                               @rivet_core@/worker_prefork_common.c
rivet_prefork_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_prefork_mpm_la_LIBADD  = @APXS_LIBS@
rivet_prefork_mpm_la_CPPFLAGS = @apache_include@ @TCL_INCLUDES@ \
                                @APXS_CPPFLAGS@ @APXS_INCLUDES@ \
                                @APR_CPPFLAGS@ @APR_INCLUDES@ \
                                @APU_INCLUDES@ \
                               -I$(srcdir) \
                               -I$(srcdir)/@apache_request@ \
                               -I$(srcdir)/@rivet_core@

rivet_lazy_mpm_la_SOURCES = @rivet_core@/rivet_lazy_mpm.c
rivet_lazy_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_lazy_mpm_la_LIBADD  = @APXS_LIBS@
rivet_lazy_mpm_la_CPPFLAGS = @apache_include@ @TCL_INCLUDES@ \
                             @APXS_CPPFLAGS@ @APXS_INCLUDES@ \
                             @APR_CPPFLAGS@ @APR_INCLUDES@ \
                             @APU_INCLUDES@ \
                            -I$(srcdir) \
                            -I$(srcdir)/@apache_request@ \
                            -I$(srcdir)/@rivet_core@

# Removing libtool .la files from installation

install-data-hook:
	echo 'target install-data-hook: removing unwanted libtool .la files from $(lib_libexecdir) $(apxs_libexecdir) $(lib_libmpmdir)'
	rm -f $(DESTDIR)$(lib_libexecdir)/*.la
	rm -f $(DESTDIR)$(apxs_libexecdir)/*.la
	rm -f $(DESTDIR)$(lib_libmpmdir)/*.la

uninstall:
	echo 'target uninstall: removing shared libraries from $(lib_libexecdir) $(apxs_libexecdir) $(lib_libmpmdir)'
	rm -f $(DESTDIR)$(lib_libmpmdir)/*.so
	rm -f $(DESTDIR)$(lib_libexecdir)/*.so
	rm -f $(DESTDIR)$(apxs_libexecdir)/mod_rivet.so

