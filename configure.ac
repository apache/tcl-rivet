#!/bin/sh 
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run when building Rivet
dnl	to configure the system for the local environment.
dnl
dnl You need at least autoconf 2.59
dnl
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

AC_INIT([Rivet],[0.6.0])
AM_CONFIG_HEADER(config.h)

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT([3.1])

AC_CONFIG_AUX_DIR(tclconfig)

#
# Init automake stuff
#
AC_DISABLE_STATIC
AM_INIT_AUTOMAKE([foreign])

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

#
# For automake to be happy, we also need AC_PROG_LIBTOOL
#
# the LIBTOOL_DEPS macro sets up for automatic updating of the
# libtool script if it becomes out of date, as per libtool docs
#
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------

# Unused: src/TclWeb.c

#TEA_ADD_SOURCES([src/TclWebcgi.c src/TclWebapache.c src/apache_multipart_buffer.c src/apache_request.c src/mod_rivet.c src/parserPkgInit.c src/rivetChannel.c src/rivetCore.c src/rivetCrypt.c src/rivetList.c src/rivetParser.c src/rivetPkgInit.c src/rivetWWW.c win/nmakehlp.c src/testing.c])
#TEA_ADD_HEADERS([src/TclWeb.h src/apache_multipart_buffer.h src/apache_request.h src/mod_rivet.h src/rivet.h src/rivetChannel.h src/rivetParser.h])
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
    AC_DEFINE(BUILD_rivet,1,[Define to define the BUILD_rivet define (?)])
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

#
# stubs are causing segmentation violations when mod_rivet is getting
# loaded into apache.  it would be nice if we did no stubs for this and
# still did stubs for librivet and librivetparser, which seem to be OK.
#
#AC_DEFINE(USE_TCL_STUBS,1,[Define to link against the Tcl stub library])
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

# We have to either find a working tclsh or be told where it is 
# using --with-tclsh
AC_ARG_WITH(tclsh, [  --with-tclsh=FILE       location of a working tclsh executable], with_tclsh=${withval})

#
# allow them to override the tclsh we think we found
#
if test x"${with_tclsh}" != x ; then
    AC_MSG_CHECKING([for tclsh])
    if test -f "${with_tclsh}" ; then
        TCLSH_PROG=${with_tclsh}
        AC_MSG_RESULT([manually set by --with-tclsh=$TCLSH_PROG])
    else
        AC_MSG_ERROR([No tclsh at place specified by --with-tclsh (${with_tclsh})])
    fi
    AC_SUBST(TCLSH_PROG)
else
TEA_PROG_TCLSH
fi

#TEA_PROG_WISH


# apache base directory

AC_DEFUN([APACHE],[
    AC_ARG_WITH(
        apache_base,
        [  --with-apache[=DIR]    Apache server base directory],
        [with_apache="/usr/local/apache2"]
    )

    AC_MSG_CHECKING(for Apache base directory)
	if test $with_apache_version = "1"; then
		if test -e $with_apache/bin/apachectl ; then
			with_apxs_found=$with_apache/bin/apxs
			apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
		elif test -e $with_apache/sbin/apachectl; then
			with_apxs_found=$with_apache/sbin/apxs
				apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
		elif test -e $with_apache/sbin/httpsdctl; then
			with_apxs_found=$with_apache/sbin/apxs
				apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
    	else
        AC_MSG_ERROR(Apache base directory not found. Specify --with-apache)
    	fi
    elif   test $with_apache_version = "2"; then
		# OpenBSD apache2 layout
		if test -e $with_apache/sbin/apachectl2; then
			with_apxs_found=$with_apache/sbin/apxs2
			apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
		# Debian folks calling it apache2ctl
		elif test -e $with_apache/sbin/apache2ctl ; then
			with_apxs_found=$with_apache/bin/apxs2
			apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
		#so called "generic" unix
		elif test -e $with_apache/bin/apachectl; then
			with_apxs_found=$with_apache/bin/apxs
			apache_base=$with_apache
			AC_MSG_RESULT(Apache base directory found)
    	else
       	 AC_MSG_ERROR(Apache base directory not found. Specify --with-apache)
    	fi
	fi
])

# We have to either find apxs or be told where it is using --with-apxs
AC_DEFUN([CHECK_APXS],[
    AC_ARG_WITH(
        apxs,
        [  --with-apxs=FILE        location of Apache's apxs tool], ,
	if test -e $apache_base/bin/apxs ; then
		with_apxs=$apache_base/bin/apxs
	fi
    )

dnl First check to see if --with-apxs was specified.
    if test x"${with_apxs}" = x ; then
        AC_PATH_PROGS(APXS, apxs)
        if test x"${APXS}" = x ; then
	        AC_MSG_ERROR([Could not find apxs.  apxs must be in your PATH or you must specify the location of the apxs script using --with-apxs])
        else
	        with_apxs=${APXS}
        fi
    else
        AC_MSG_CHECKING([for apxs])
        AC_MSG_RESULT([${with_apxs}])
    fi
    if test -x "${with_apxs}" ; then
    dnl It actually calls another script, so the PATH needs to be set.
        export PATH=$PATH:`dirname ${with_apxs}`
        APXS_CPPFLAGS=`${with_apxs} -q CFLAGS`
        AC_SUBST(APXS_CPPFLAGS)
        APXS_LDFLAGS=`${with_apxs} -q LDFLAGS_SHLIB`
        AC_SUBST(APXS_LDFLAGS)
        APXS_LIBS=`${with_apxs} -q LIBS_SHLIB`
        AC_SUBST(APXS_LIBS)
        APXS_INCLUDES=-I`${with_apxs} -q INCLUDEDIR`
        AC_SUBST(APXS_INCLUDES)
        APXS_CPPFLAGS_SHLIB=`${with_apxs} -q CFLAGS_SHLIB`
        AC_SUBST(APXS_CPPFLAGS_SHLIB)
        APXS_LD_SHLIB=`${with_apxs} -q LD_SHLIB`
        AC_SUBST(APXS_LD_SHLIB)
        APXS_LIBEXECDIR=`${with_apxs} -q LIBEXECDIR`
        AC_SUBST(APXS_LIBEXECDIR)
        APXS_SYSCONFDIR=`${with_apxs} -q SYSCONFDIR`
        AC_SUBST(APXS_SYSCONFDIR)
        APXS_PREFIX=`${with_apxs} -q PREFIX`
        AC_SUBST(APXS_PREFIX)
    else
        AC_MSG_ERROR([${with_apxs} is not an executable file])
    fi
])


AC_DEFUN([APACHE_INCLUDES],[

  AC_ARG_WITH(
    apache_include,
    [  --with-apache-include[=DIR]     Apache server directory],
    ,
    [with_apache_include=${apache_base}/include]
  )

  AC_MSG_CHECKING(for Apache includes directory)

  if test "$with_apache_include" = "no"; then
    AC_MSG_ERROR( Specify the apache using --with-apache-include)
  else
    # make sure that a well known include file exists
    if test -e $with_apache_include/httpd.h; then
      apache_include=$with_apache_include
      AC_MSG_RESULT(Apache includes found!)
    else
      AC_MSG_ERROR( $with_apache_include not found. )
    fi
  fi
])

AC_DEFUN([APR_HANDLING],[
  AC_ARG_WITH(
   apr_config,
   	[  --with-apr-config=FILE     apr portable apr-1-config path],
	,
	 if test -x ${apache_base}/bin/apr-1-config ; then
		with_apr_config=${apache_base}/bin/apr-1-config 
        AC_MSG_RESULT([${with_apr_config}])
	 else
    	AC_MSG_ERROR( Specify the apr-1-config path using --with-apr-config)
	 fi
	 
  )
     AC_MSG_CHECKING([for apr])
	 if test -x ${with_apr_config} ; then
        export PATH=$PATH:`dirname ${with_apr_config}`
        AC_MSG_RESULT([${with_apr_config}])
		APR_CPPFLAGS=`${with_apr_config} --cppflags`
        AC_SUBST(APR_CPPFLAGS)
		APR_INCLUDES=`${with_apr_config} --includes`
        AC_SUBST(APR_INCLUDES)
	 else
    	AC_MSG_ERROR( Specify the apr-1-config path using --with-apr-config)
	 fi
	 
])

AC_DEFUN([APACHE_VERSION],[
    AC_ARG_WITH(
        apache_version,
        [  --with-apache-version[=VER]  build for apache 1.x or 2.x],
        ,[apache_version="2"]
    )

    AC_MSG_CHECKING(for apache version)
    if test "$with_apache_version" = "1"; then
        AC_MSG_RESULT( building for apache 1.x )
	AC_CONFIG_FILES([src/apache-1/Makefile])   
	apache_version_dir="apache-1"
    else
        AC_MSG_RESULT( building for apache 2.x )
	AC_CONFIG_FILES([src/apache-2/Makefile])
	apache_version_dir="apache-2"
    fi
])

AC_DEFUN([HANDLE_TCL_PACKAGE_PATH],[
    # shamelessly stolen from TEA_LOAD_TCLCONFIG
    AC_MSG_CHECKING([Debian TCL_PACKAGE_PATH workaround])
        if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
           AC_MSG_RESULT([loading])
           . "${TCL_BIN_DIR}/tclConfig.sh"
        else
           AC_MSG_RESULT([could not find ${TCL_BIN_DIR}/tclConfig.sh])
        fi
    # If we have multiple pathes, pull the first one, add end slash if needed
	res=`echo ${TCL_PACKAGE_PATH} | \
             grep '[[^\]] ' | \
             sed -e 's/\([[^\]]\)\( \)\(.*\)$/\1/'  -e 's/\([[^\/]]\)$/\1\//' `
	if test x"${res}" != x; then
	    TCL_PACKAGE_PATH=${res}
            AC_SUBST(TCL_PACKAGE_PATH)
        fi
						     
])


AC_DEFUN([GET_RIVET_BASE], [
	RIVET_BASE_INCLUDE=`pwd`/src	
	AC_SUBST(RIVET_BASE_INCLUDE)
])

GET_RIVET_BASE
APACHE_VERSION
APACHE
CHECK_APXS
APACHE_INCLUDES
APR_HANDLING
AC_SUBST(apache_include)
AC_SUBST(apr_include)
AC_SUBST(apache_version_dir)


AC_DEFINE_UNQUOTED(NAMEOFEXECUTABLE,"${TCLSH_PROG}",[The path to a working tclsh executable])

# We need to use the package path for the installation procedure.  On
#Debian linux TCL_PACKAGE_PATH may have more than one path in the
#TCL_PACKAGE_PATH variable, so we have to handle this
HANDLE_TCL_PACKAGE_PATH
AC_SUBST(TCL_PACKAGE_PATH)


# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])

#case $apache_version_dir in
#    apache-1) AC_CONFIG_FILES([Makefile src/Makefile src/apache-1/Makefile doc/Makefile]) ;;
#    apache-2) AC_CONFIG_FILES([Makefile src/Makefile src/apache-2/Makefile doc/Makefile]) ;;
#esac
AC_OUTPUT

