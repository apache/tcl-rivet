#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.
#
# RCS: @(#) $Id$

AC_INIT(Rivet, 0.5.0)
AM_INIT_AUTOMAKE(Rivet, 0.5.0)

AC_PROG_CC
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC([])

TEA_INIT([3.0])

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

TEA_PATH_TKCONFIG
TEA_LOAD_TKCONFIG

TEA_PREFIX

TEA_SETUP_COMPILER

TEA_ADD_HEADERS([])
TEA_ADD_INCLUDES([])
TEA_ADD_LIBS([])
TEA_ADD_CFLAGS([])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([])

TEA_PUBLIC_TCL_HEADERS
TEA_PUBLIC_TK_HEADERS

TEA_ENABLE_THREADS

TEA_ENABLE_SHARED

TEA_CONFIG_CFLAGS

TEA_MAKE_LIB

TEA_PROG_TCLSH

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



AC_OUTPUT([
Makefile
src/Makefile
])
