#   Makefile.am -
#
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.

# The weird and wild magic of auto* associates these two.

#apxs_libexecdir = @APXS_LIBEXECDIR@
#apxs_libexec_LTLIBRARIES = mod_rivet.la
#
#mod_rivet_la_SOURCES =  mod_rivet.c                                         \
#                        ../@apache_request@/apache_multipart_buffer.c 	    \
#                        ../@apache_request@/apache_request.c                \
#                        ../@rivet_commands@/rivetCore.c                     \
#                        ../@rivet_commands@/rivetInspect.c                  \
#                        ../@rivet_channel@/rivetChannel.c                   \
#                        ../parser/rivetParser.c                             \
#                        TclWebapache.c                                      \
#                        apache_config.c                                     \
#                        mod_rivet_common.c
#
#mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
#mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
#mod_rivet_la_CPPFLAGS = -I@rivet_core@ -I../@rivet_channel@ -I@RIVET_BASE_INCLUDE@/parser -I../@apache_request@ @apache_include@ -I@RIVET_BASE_INCLUDE@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'
#
#lib_libexecdir = @RIVET_TCL_TARGET@/mpm
#lib_libexec_LTLIBRARIES = rivet_worker_mpm.la rivet_prefork_mpm.la rivet_aprthread_mpm.la
#
#rivet_worker_mpm_la_SOURCES = rivet_worker_mpm.c
#rivet_worker_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
#rivet_worker_mpm_la_LIBADD  = @APXS_LIBS@
#rivet_worker_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I../@apache_request@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@
#
#rivet_prefork_mpm_la_SOURCES = rivet_prefork_mpm.c
#rivet_prefork_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
#rivet_prefork_mpm_la_LIBADD  = @APXS_LIBS@
#rivet_prefork_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I../@apache_request@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@
#
#rivet_aprthread_mpm_la_SOURCES = rivet_aprthread_mpm.c
#rivet_aprthread_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
#rivet_aprthread_mpm_la_LIBADD  = @APXS_LIBS@
#rivet_aprthread_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I../@apache_request@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@
#
