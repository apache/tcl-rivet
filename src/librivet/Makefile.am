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

# $Id$

INCLUDES = @apache_include@ -I../@apache_version_dir@ -I..

# ... and these two.  We want to put the libs in the package path,
# rather than the standard library location for the system.
lib_libexecdir = @RIVET_TCL_TARGET@
lib_libexec_LTLIBRARIES = librivetlib.la

#
# Rivet Library
#

librivetlib_la_SOURCES = rivetList.c rivetCrypt.c rivetWWW.c rivetPkgInit.c
librivetlib_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
librivetlib_la_LIBADD  = @APXS_LIBS@
librivetlib_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_INCLUDES@ @APR_CPPFLAGS@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS 

