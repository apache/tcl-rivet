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

#
# mod_rivet
#
apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

mod_rivet_la_SOURCES =  mod_rivet.c                                         \
                        ../@apache_request@/apache_multipart_buffer.c 	    \
                        ../@apache_request@/apache_request.c                \
                        ../@rivet_core_cmds@/rivetCore.c                    \
                        ../@rivet_core_cmds@/rivetInspect.c                 \
                        ../@rivet_channel@/rivetChannel.c                   \
                        ../common/TclWebapache.c                            \
                        ../common/apache_config.c                           \
                        ../common/mod_rivet_common.c                        \
                        ../parser/rivetParser.c

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
mod_rivet_la_CPPFLAGS = -I@rivet_core@ -I../@rivet_channel@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@/parser -I../@apache_request@ @apache_include@ -I@RIVET_BASE_INCLUDE@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'
