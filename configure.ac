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

AC_DEFINE(USE_TCL_STUBS)
AC_DEFINE(USE_TK_STUBS)

TEA_MAKE_LIB

TEA_PROG_TCLSH



AC_OUTPUT([
Makefile
generic/Makefile
])
