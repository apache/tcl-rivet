#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.
#
# RCS: @(#) $Id$

#-----------------------------------------------------------------------
# Sample configure.in for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.
#
# This initializes the environment with PACKAGE_NAME and PACKAGE_VERSION
# set as provided.  These will also be added as -D defs in your Makefile
# so you can encode the package version directly into the source files.
#-----------------------------------------------------------------------

AC_INIT([Rivet], [0.5.0])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT([3.1])

AC_CONFIG_AUX_DIR(tclconfig)

#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

#--------------------------------------------------------------------
# Load the tkConfig.sh file if necessary (Tk extension)
#--------------------------------------------------------------------

#TEA_PATH_TKCONFIG
#TEA_LOAD_TKCONFIG

#-----------------------------------------------------------------------
# Handle the --prefix=... option by defaulting to what Tcl gave.
# Must be called after TEA_LOAD_TCLCONFIG and before TEA_SETUP_COMPILER.
#-----------------------------------------------------------------------

TEA_PREFIX

#-----------------------------------------------------------------------
# Standard compiler checks.
# This sets up CC by using the CC env var, or looks for gcc otherwise.
# This also calls AC_PROG_CC, AC_PROG_INSTALL and a few others to create
# the basic setup necessary to compile executables.
#-----------------------------------------------------------------------

TEA_SETUP_COMPILER

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------

TEA_ADD_SOURCES([TclWeb.c TclWebapache.c TclWebcgi.c apache_multipart_buffer.c apache_request.c mod_rivet.c parserPkgInit.c rivetChannel.c rivetCore.c rivetCrypt.c rivetList.c rivetParser.c rivetPkgInit.c rivetWWW.c testing.c nmakehlp.c])
TEA_ADD_HEADERS([src/TclWeb.h src/apache_multipart_buffer.h src/apache_request.h src/mod_rivet.h src/rivet.h src/rivetChannel.h src/rivetParser.h])
TEA_ADD_INCLUDES([])
TEA_ADD_LIBS([])
TEA_ADD_CFLAGS([])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([])

#--------------------------------------------------------------------
# __CHANGE__
# A few miscellaneous platform-specific items:
#
# Define a special symbol for Windows (BUILD_sample in this case) so
# that we create the export library with the dll.
#
# Windows creates a few extra files that need to be cleaned up.
# You can add more files to clean if your extension creates any extra
# files.
#
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    AC_DEFINE(BUILD_sample)
    CLEANFILES="pkgIndex.tcl *.lib *.dll *.exp *.ilk *.pdb vc*.pch"
    #TEA_ADD_SOURCES([win/winFile.c])
    #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
else
    CLEANFILES="pkgIndex.tcl"
    #TEA_ADD_SOURCES([unix/unixFile.c])
    #TEA_ADD_LIBS([-lsuperfly])
fi
AC_SUBST(CLEANFILES)

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This MUST be called after TEA_LOAD_TCLCONFIG / TEA_LOAD_TKCONFIG
#--------------------------------------------------------------------

TEA_PUBLIC_TCL_HEADERS
#TEA_PRIVATE_TCL_HEADERS

#TEA_PUBLIC_TK_HEADERS
#TEA_PRIVATE_TK_HEADERS
#TEA_PATH_X

#--------------------------------------------------------------------
# Check whether --enable-threads or --disable-threads was given.
# This auto-enables if Tcl was compiled threaded.
#--------------------------------------------------------------------

TEA_ENABLE_THREADS

#--------------------------------------------------------------------
# The statement below defines a collection of symbols related to
# building as a shared library instead of a static library.
#--------------------------------------------------------------------

TEA_ENABLE_SHARED

#--------------------------------------------------------------------
# This macro figures out what flags to use with the compiler/linker
# when building shared/static debug/optimized objects.  This information
# can be taken from the tclConfig.sh file, but this figures it all out.
#--------------------------------------------------------------------

TEA_CONFIG_CFLAGS

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# Everyone should be linking against the Tcl stub library.  If you
# can't for some reason, remove this definition.  If you aren't using
# stubs, you also need to modify the SHLIB_LD_LIBS setting below to
# link against the non-stubbed Tcl library.  Add Tk too if necessary.
#--------------------------------------------------------------------

AC_DEFINE(USE_TCL_STUBS)
#AC_DEFINE(USE_TK_STUBS)

#--------------------------------------------------------------------
# This macro generates a line to use when building a library.  It
# depends on values set by the TEA_ENABLE_SHARED, TEA_ENABLE_SYMBOLS,
# and TEA_LOAD_TCLCONFIG macros above.
#--------------------------------------------------------------------

TEA_MAKE_LIB

#--------------------------------------------------------------------
# Find tclsh so that we can run pkg_mkIndex to generate the pkgIndex.tcl
# file during the install process.  Don't run the TCLSH_PROG through
# ${CYGPATH} because it's being used directly by make.
# Require that we use a tclsh shell version 8.2 or later since earlier
# versions have bugs in the pkg_mkIndex routine.
# Add WISH as well if this is a Tk extension.
#--------------------------------------------------------------------

TEA_PROG_TCLSH
#TEA_PROG_WISH

dnl This is required, because we need this information!
AC_ARG_WITH(apxs, [  --with-apxs              location of Apache's apxs tool], with_apxs=${withval})
AC_MSG_CHECKING([for apxs])

dnl First check to see if --with-magick-config was specified.
if test x"${with_apxs}" != x ; then
    if test -x "${with_apxs}" ; then
	dnl It actually calls another script, so the PATH needs to be set.
	export PATH=$PATH:`dirname ${with_apxs}`
	AC_MSG_RESULT("Using ${with_apxs}")
	APXS_CPPFLAGS=`${with_apxs} -q CFLAGS`
	AC_SUBST(APXS_CPPFLAGS)
	APXS_LDFLAGS=`${with_apxs} -q LDFLAGS_SHLIB`
	AC_SUBST(APXS_LDFLAGS)
	APXS_LIBS=`${with_apxs} -q LIBS_SHLIB`
	AC_SUBST(APXS_LIBS)
	APXS_INCLUDES=-I`${with_apxs} -q INCLUDEDIR`
	AC_SUBST(APXS_INCLUDES)

    else
	AC_MSG_ERROR([${with_apxs} is not an executable file])
    fi
else
    AC_MSG_ERROR([You must specify the location of the apxs script --with-apxs!])
fi


AC_ARG_WITH(tclsh, [  --with-tclsh		location of the tclsh executable that corresponds to the --with-tcl directory used], with_tclsh=${withval})
AC_MSG_CHECKING([for tclsh])

if test x"${with_tclsh}" != x ; then
    if test -x "${with_tclsh}" ; then
       NAMEOFEXECUTABLE=-DNAMEOFEXECUTABLE=\\\"${with_tclsh}\\\"
       AC_SUBST(NAMEOFEXECUTABLE)
    else
	AC_MSG_ERROR([${with_tclsh} is not an executable file])
    fi
else
    AC_MSG_ERROR([You must specify the location of the tclsh corresponding to the dir used with --with-tcl using --with-tclsh!])
fi

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------

AC_OUTPUT([Makefile src/Makefile])

