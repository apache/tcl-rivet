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

AM_CPPFLAGS = @apache_include@ -I../@apache_version_dir@ -I..

# ... and these two.  We want to put the libs in the package path,
# rather than the standard library location for the system.
lib_libexecdir = @RIVET_TCL_TARGET@
lib_libexec_LTLIBRARIES = librivetparser.la

#
# Rivet Parser Library
#
librivetparser_la_SOURCES = rivetParser.c parserPkgInit.c
librivetparser_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
librivetparser_la_LIBADD  = @APXS_LIBS@
librivetparser_la_CPPFLAGS = @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"' -DUSE_TCL_STUBS

