#!/bin/sh 
dnl This file is an input file used by the GNU "autoconf" program to
dnl generate the file "configure", which is run when building Rivet
dnl to configure the system for the local environment.
dnl
dnl You need at least autoconf 2.59
dnl
#
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

m4_define([rivet_VERSION],m4_esyscmd([cat VERSION | tr -d '\n']))

AC_INIT([Rivet],m4_defn([rivet_VERSION]),[rivet-dev@tcl.apache.org],[rivet-]m4_defn([rivet_VERSION]))
TEA_INIT([3.9])

# we are storing here the configure command line, as recursive
# invocations change the value of the shell variables $0 $*

CONFIGURE_CMD="$0 $*"

AC_CONFIG_AUX_DIR(tclconfig)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# AX_PREFIX_CONFIG_H is a macro taken from the autotools macro archive.
# The typical configure generated 'config.h' can be renamed and 
# preprocessor symbols in it can be prefixed in order to avoid clashes 
# with Apache's config.h
# 
# As of today (27 Sept 2008) this macro is not part of autotools and
# it's in the 'm4' directory
#

AX_PREFIX_CONFIG_H([rivet_config.h],[rivet])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

#
# Init automake stuff
#
AC_DISABLE_STATIC
AM_INIT_AUTOMAKE([foreign subdir-objects])

# establishing a landmark in the directory hierarchy as per 
# autotools design. This is what AC_CONFIG_SRCDIR is meant to do
# I arbitrarily choose src/rivet.h as landmark, moving this file
# to a different directory or deleting it implies this line has to
# change

AC_CONFIG_SRCDIR([src/rivet.h])

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
# For automake to be happy, we also need LT_INIT
#
# the LIBTOOL_DEPS macro sets up for automatic updating of the
# libtool script if it becomes out of date, as per libtool docs
#
LT_INIT
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

# removed AC_DEFINE(BUILD_rivet....) as per TEA 3.9 (Massimo Manghi 20100825)

if test "${TEA_PLATFORM}" = "windows" ; then
#   AC_DEFINE(BUILD_rivet,1,[Define to define the BUILD_rivet define (?)])
    CLEANFILES="pkgIndex.tcl *.lib *.dll *.exp *.ilk *.pdb vc*.pch"
#   #TEA_ADD_SOURCES([win/winFile.c])
#   #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
else
    CLEANFILES="pkgIndex.tcl"
#   #TEA_ADD_SOURCES([unix/unixFile.c])
#   #TEA_ADD_LIBS([-lsuperfly])
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
AC_ARG_WITH(tclsh, [  --with-tclsh=FILE location of a working tclsh executable], with_tclsh=${withval})

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
    AC_MSG_CHECKING(for Apache base)
    AC_ARG_WITH(
        apache,
        [  --with-apache[=DIR] Apache server base directory],, 
        [with_apache="/usr/local/apache2"]
    )
    apache_base="${with_apache}"
    AC_MSG_RESULT([$apache_base])
])

#
# We have to either find apxs or be told where it is using --with-apxs 
#
AC_DEFUN([CHECK_APXS],[
    AC_MSG_CHECKING(for Apache apxs)
    AC_ARG_WITH(
        apxs,
        [  --with-apxs=FILE        location of Apache apxs tool],  
        [if test -x "${with_apxs}" ; then
            apxs_found="${with_apxs}"
            AC_MSG_RESULT([$apxs_found])
         fi]
        ,
        [ 
            APXS_NAMES=apxs
            for apxs_file in ${APXS_NAMES}; do 
                for apxs_path in bin sbin; do 
                     if test -x "${apache_base}"/"${apxs_path}"/"${apxs_file}"; then
                            apxs_found="${apache_base}"/"${apxs_path}"/"${apxs_file}" 
                            break 2
                      fi
                done
            done
            if test x"$apxs_found" = x; then
                AC_PATH_PROGS(with_apxs, apxs)
                if test "${with_apxs+set}" = set ; then
                    apxs_found="${with_apxs}"
                fi
            fi
            if test x"${apxs_found}" = x; then
                AC_MSG_ERROR([Could not find apxs.  apxs must be in your PATH or you must specify the location of the apxs script using --with-apxs])
            else
                AC_MSG_RESULT([$apxs_found])
            fi
        ]
    )
    if test "${apxs_found+set}" = set ; then
        dnl At this point we already have apr sorted out
        export PATH=$PATH:`dirname $apxs_found`
        APXS_CPPFLAGS=`${apxs_found} -q CFLAGS`
        AC_SUBST(APXS_CPPFLAGS)
        APXS_LDFLAGS=`${apxs_found} -q LDFLAGS_SHLIB`
        AC_SUBST(APXS_LDFLAGS)
        APXS_LIBS=`${apxs_found} -q LIBS_SHLIB`
        AC_SUBST(APXS_LIBS)
        APXS_INCLUDES=-I`${apxs_found} -q INCLUDEDIR`
        AC_SUBST(APXS_INCLUDES)
        APXS_CPPFLAGS_SHLIB=`${apxs_found} -q CFLAGS_SHLIB`
        AC_SUBST(APXS_CPPFLAGS_SHLIB)
        APXS_LD_SHLIB=`${apxs_found} -q LD_SHLIB`
        AC_SUBST(APXS_LD_SHLIB)
        APXS_LIBEXECDIR=`${apxs_found} -q LIBEXECDIR`
        AC_SUBST(APXS_LIBEXECDIR)
        APXS_SYSCONFDIR=`${apxs_found} -q SYSCONFDIR`
        AC_SUBST(APXS_SYSCONFDIR)
        APXS_PREFIX=`${apxs_found} -q PREFIX`
        AC_SUBST(APXS_PREFIX)
    else 
        AC_MSG_ERROR([Could not find apxs.  apxs must be in your PATH or you must specify the location of the apxs script using --with-apxs])
    fi
])


AC_DEFUN([APACHE_INCLUDES],[
  AC_ARG_WITH(
    apache_include,
     [  --with-apache-include[=DIR]     Apache server directory],
     [apache_include="${with_apache_include}"] 
    ,[apache_include="${APXS_INCLUDES}"]
  )
    AC_SUBST(apache_include)
])

AC_DEFUN([APR_HANDLING],[
  AC_MSG_CHECKING(for Apache apr)
  AC_ARG_WITH(
   apr_config,
    [  --with-apr-config=FILE     apr portable apr-1-config path],
     [if test -x "${with_apr_config}"; then
         apr_found="${with_apr_config}"
     fi]
         ,
    [if test -x "${apache_base}/bin/apr-1-config" ; then
         apr_found="${apache_base}/bin/apr-1-config" 
    elif test -x "${apache_base}/sbin/apr-1-config" ; then
         apr_found="${apache_base}/sbin/apr-1-config" 
    elif test "${apache_base+set}" = set; then 
        AC_PATH_PROGS(with_apr_config, apr-1-config)
        if test "${with_apr_config+set}" = set ; then
            apr_found="${with_apr_config}"
        fi
    else 
        AC_MSG_ERROR( Specify the apr-1-config path using --with-apr-config, 1)
    fi]
)
    if test "${apr_found+set}" = set ; then
        AC_MSG_RESULT([$apr_found])
        dnl At this point we already have apr sorted out
        dnl It actually calls another script, so the PATH needs to be set.
        export PATH=$PATH:`dirname ${apr_found}`
        APR_CPPFLAGS=`${apr_found} --cppflags`
        AC_SUBST(APR_CPPFLAGS)
        APR_INCLUDES=`${apr_found} --includes`
        AC_SUBST(APR_INCLUDES)
        APR_LDLFAGS=`${apr_found} --link-libtool --libs`
        AC_SUBST(APR_LDFLAGS)
    else
        AC_MSG_ERROR([Could not find apr-1-config. apr-1-config must be in your PATH or you must specify the location of the apr script using --with-apr-config])
    fi
])

# -- APU_HANDLING
#
# calling apu-1-config to determine where apr-utils includes
# and library are
#

AC_DEFUN([APU_HANDLING],[
  AC_MSG_CHECKING(for Apache apu)
  AC_ARG_WITH(
   apu_config,
    [  --with-apu-config=FILE     apu portable apu-1-config path],
    [if "${with_apu_config}" --version; then
         apu_found="${with_apu_config}"
    fi]
     ,
    [if apu-1-config --version; then
         apu_found=apu-1-config
    elif test -x "${apache_base}/bin/apu-1-config" ; then
         apu_found="${apache_base}/bin/apu-1-config" 
    elif test -x "${apache_base}/sbin/apu-1-config" ; then
         apu_found="${apache_base}/sbin/apu-1-config" 
    elif test "${apache_base+set}" = set; then 
        AC_PATH_PROGS(with_apu_config, apu-1-config)
        if test "${with_apu_config+set}" = set ; then
            apu_found="${with_apu_config}"
        fi
    else 
        AC_MSG_ERROR( Specify the apu-1-config path or program name using --with-apu-config, 1)
    fi]
)
    if test "${apu_found+set}" = set ; then
        AC_MSG_RESULT([$apu_found_])
        dnl At this point we already have apu sorted out
        dnl It actually calls another script, so the PATH needs to be set.
        export PATH=$PATH:`dirname ${apu_found}`
        APU_INCLUDES=`${apu_found} --includes`
        AC_SUBST(APU_INCLUDES)
        APU_LDLFAGS=`${apu_found} --link-libtool --libs`
        AC_SUBST(APU_LDFLAGS)
    else
        AC_MSG_ERROR([Could not find apu-1-config. apu-1-config must be in your PATH or you must specify the location of the apu script using --with-apu-config])
    fi
])

#--------------------------------------------------------------------
# 6-Dec-2011: Introducing new ::rivet namespace. Some components 
# (namely RivetTcl, providing Tcl level initialization) are required 
# within mod_rivet.c. Unfortunalely Tcl < 8.5.10 shared
# libs exporting names could clash with Apache's binary own names. 
# Hence we have to make sure we are running a Tcl compatible version.
# (See Tcl bug #3216070). 
# 
# Effect: If you're running Tcl < 8.5.10 we refuse to build Rivet...

AX_COMPARE_VERSION([$TCL_VERSION$TCL_PATCH_LEVEL],[lt],[8.5.10],[
    AC_MSG_ERROR([Rivet m4_defn([rivet_VERSION]) requires Tcl > 8.5.10 (current is $TCL_VERSION$TCL_PATCH_LEVEL)])
] , [
    AC_MSG_NOTICE([we are building Rivet m4_defn([rivet_VERSION]) with Tcl $TCL_VERSION$TCL_PATCH_LEVEL])
])


print_fileevent_msg="0"
AX_COMPARE_VERSION([$TCL_VERSION$TCL_PATCH_LEVEL],[lt],[8.5.15],[
    print_fileevent_msg="1"
] , )


AX_COMPARE_VERSION([$TCL_VERSION],[eq],[8.6],[
    if test "$TCL_PATCH_LEVEL" = ".0" ; then
        print_fileevent_msg="1"
    fi
] , )

# MOD_RIVET_CORE
#
# Determining the path to the mod_rivet.* source files
#

AC_DEFUN([MOD_RIVET_CORE],[
    AC_ARG_WITH(
       rivet_core,
       [  --with-rivet-core[=path] mod_rivet core directory],,
       [with_rivet_core="mod_rivet_ng"]
    )

    AC_MSG_CHECKING([mod_rivet core])

    #if [[ -d "src/${with_rivet_core}" ]]; then
    #    rivet_core=$(pwd)/src/${with_rivet_core}
    #else
    #    rivet_core=${with_rivet_core}
    #fi
    
    rivet_core=${with_rivet_core}
   
    # by default we search the channel and core commands
    # implementation in the same directory. This can
    # be superseded by other macros

    default_rivet_commands=${rivet_core}
    default_rivet_channel=${rivet_core}

    #AC_CONFIG_FILES([src/${rivet_core}/Makefile])
    AC_SUBST(rivet_core)
    AC_MSG_RESULT([$rivet_core])
])

# 

AC_DEFUN([HANDLE_TCL_PACKAGE_PATH],[
    # shamelessly stolen from TEA_LOAD_TCLCONFIG
    AC_MSG_CHECKING([Debian TCL_PACKAGE_PATH workaround])
        if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
           AC_MSG_RESULT([loading])
           . "${TCL_BIN_DIR}/tclConfig.sh"
        else
           AC_MSG_RESULT([could not find ${TCL_BIN_DIR}/tclConfig.sh])
        fi
    # If we have multiple paths, pull the first one, add end slash if needed
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
    RIVET_SOURCE_BASE=`pwd`/src    
    AC_SUBST(RIVET_SOURCE_BASE)
])

# APACHE_REQUEST
#
# Choosing where is located the code for handling Apache request data (like
# form data etc). Default: 'request'  

AC_DEFUN([APACHE_REQUEST],[
    AC_ARG_WITH(
        apache_request,
        [  --with-apache-request=DIR path to rivet HTML form and urlencoded data handler],,
        [with_apache_request="request"]
    )

    AC_MSG_CHECKING([code for HTML form and urlencoded data handling])
    apache_request=${with_apache_request}
    AC_SUBST(apache_request)
    AC_MSG_RESULT([$apache_request])
])

# RIVET_CORE_CMDS
#
# selecting non default directory for Rivet code commands core
#

AC_DEFUN([RIVET_CORE_CMDS],[
    AC_ARG_WITH(
        rivet_commands,
        [  --with-rivet-commands[=dir] path to Rivet core commands code],,
        [with_rivet_commands=$default_rivet_commands]
    )

    AC_MSG_CHECKING([for Rivet core commands code])
    rivet_commands=${with_rivet_commands}
    AC_SUBST(rivet_commands)
    AC_MSG_RESULT([$rivet_commands])

])

# RIVET_CHANNEL
#
# selecting the Rivet channel code directory

AC_DEFUN([RIVET_CHANNEL],[
    AC_ARG_WITH(
        rivet_channel,
        [  --with-rivet-channel=DIR Rivet channel code path ],,
        [with_rivet_channel=$default_rivet_channel]
    )

    AC_MSG_CHECKING([for Rivet channel code path])
    rivet_channel=${with_rivet_channel}

    AC_SUBST(rivet_channel)
    AC_MSG_RESULT([$rivet_channel])
])

# RIVET_TCL_LIB
#
#

AC_DEFUN([RIVET_TCL_LIB],[
    AC_ARG_WITH(
        rivet_target_dir,
        [  --with-rivet-target-dir=DIR  Rivet tcl and binary libraries target dir ],,
        with_rivet_target_dir="${apache_base}/lib/rivet${PACKAGE_VERSION}"
    )
    AC_MSG_CHECKING([for Rivet libraries target])
    RIVET_TCL_TARGET=${with_rivet_target_dir}
    AC_SUBST(RIVET_TCL_TARGET)
    AC_MSG_RESULT([$with_rivet_target_dir])
])

# DISPLAY_RIVET_VERSION
#
# --enable-version-display=[yes|no] (default: no)
# switches on version number display in Apache's signature.
#

AC_DEFUN([DISPLAY_RIVET_VERSION],[
    AC_ARG_ENABLE(
        version_display,
    [  --enable-version-display  Display Rivet version in Apache signature],
    [ signature_version_display=$enable_version_display ],
    [ signature_version_display="no" ]
    )

    AC_MSG_CHECKING([whether rivet version is printed in Apache signature])
    if test "$signature_version_display" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(DISPLAY_VERSION,1,[Display Rivet version in Apache signature])
    else
        AC_MSG_RESULT([no, Rivet version will be hidden])
        AC_DEFINE(DISPLAY_VERSION,0,[Display Rivet version in Apache signature])
    fi
])

# UPLOAD_DIRECTORY
# 
# Adds '--with-upload-dir=DIR' to configure's arguments
#
# let's make configuration of the upload directory more flexible for windows builds
# (in case someone steps forward and takes up the Windows port)
# The upload directory can be changed in the configuration, but we give the 
# user/packager the chance to set a default at compile time.
# Default: /tmp

AC_DEFUN([UPLOAD_DIRECTORY],[
    AC_ARG_WITH(
    upload_dir,
    [  --with-upload-dir=DIR Default directory for uploads],,
        with_upload_dir="/tmp"
    )
    AC_MSG_CHECKING(where Rivet will save uploads)
    RIVET_UPLOAD_DIR=${with_upload_dir}
    AC_SUBST(RIVET_UPLOAD_DIR)
    AC_MSG_RESULT([$with_upload_dir])
])

# HONOR_HEAD_REQUESTS (--enable-head-requests)
# Rivet answers to HEAD requests with a standard response to avoid the whole workload
# of generating the content of a page with to the sole purpose of returning the HTTP 
# headers.To test the configuration or scripts that manipulate the headers it 
# can be useful to let Rivet honor a HEAD request as if the whole content was 
# generated so that the real headers are returned (this can be set in configuration 
# by adding "RivetServerConf HonorHeaderOnlyRequests yes" to the server configuration)
#
# Default: no

AC_DEFUN([HONOR_HEAD_REQUESTS],[
    AC_ARG_ENABLE(
    head_requests,
    [  --enable-head-requests Returns real headers in response to a HEAD request],
    [ honor_head_requests=$enable_head_requests ],
    [ honor_head_requests="yes"]
    )

    AC_MSG_CHECKING([if Rivet has to honor HEAD requests])
    if test "$honor_head_requests" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(HEAD_REQUESTS,1,[Honor HEAD requests])
    else
        AC_MSG_RESULT([no, Rivet will shunt HEAD requests])
        AC_DEFINE(HEAD_REQUESTS,0,[Honor HEAD requests])
    fi
])

# SINGLE_WORKER_TCL_THREAD 
#
# This options is used by the worker MPM bridge for debugging.
# When enabled rivet_worker_bridge will create a single Tcl interp thread instead 
# of the a whole pool of them
# 
# Defautl: no

AC_DEFUN([SINGLE_WORKER_TCL_THREAD],[
    AC_ARG_ENABLE(
    single_thread,
    [  --enable-single-thread forces the worker brigde to create a single thread (debug)],
    [ single_thread=$enable_single_thread ],
    [ single_thread="no"]
    )

    AC_MSG_CHECKING([if the worker MPM bridge will run in single thread mode])
    if test "$single_thread" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(MPM_SINGLE_TCL_THREAD,1,[yes, MPM worker single thread])
    else
        AC_MSG_RESULT([no, maximal number of Tcl threads])
    fi
])

# -- SERIALIZE_HTTP_REQUEST
#
# The macro defines the symbol SERIALIZE_HTTP_REQUESTS and cooperates 
# with macro HTTP_REQUESTS_PROC. This option is useful only 
# for debugging (avoids threads concurrency). See the code of the
# worker bridge to see the preprocessor macros cooperating to
# attain this feature
#
# Default: no

AC_DEFUN([HTTP_REQUESTS_MUTEX],[
    AC_ARG_ENABLE(
        requests_serialization,
        [  --enable-requests-serialization Forces HTTP requests serialization among threads(debug)],
        [ requests_serialization=$enable_requests_serialization ],
        [ requests_serialization="no" ]
    )

    AC_MSG_CHECKING([if we want the worker bridge to serialize HTTP requests])
    if test "$requests_serialization" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(SERIALIZE_HTTP_REQUESTS,1,[requests will be serialized])
    else
        AC_MSG_RESULT([no, HTTP requests will not be serialized])
    fi
])

# -- RIVET_DEBUG_BUILD
#
# Defining a symbol for conditional inclusion of code for debugging purposes
#
# Default: no
#

AC_DEFUN([BUILD_DEBUG_CODE],[
    AC_ARG_ENABLE(
        rivet_debug_build,
        [  --enable-rivet-debug-build Includes optional debugging code],
        [ rivet_debug_build=$enable_rivet_debug_build ],
        [ rivet_debug_build="no" ]
    )

    AC_MSG_CHECKING([if we are building mod_rivet for debugging])
    if test "$rivet_debug_build" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(DEBUG_BUILD,1,[we are building mod_rivet with optional code])
    else
        AC_MSG_RESULT([no, ordinary build])
    fi
])

# RIVET_COMMANDS_EXPORT (--enable-rivet-commands-export).
# Enable export of commands in Rivet's namespace. Definining this symbols
# sets the boolean configuration variable rivet_commands_export 
# Default: yes

AC_DEFUN([RIVET_COMMANDS_EXPORT],[
    AC_ARG_ENABLE(
        rivet-commands-export,
        [  --disable-rivet-commands-export to prevent export from ::rivet namespace],
        [ rivet_commands_export=$enable_rivet_commands_export ],
        [ rivet_commands_export="yes"]
    )

    AC_MSG_CHECKING([if the commands in ::rivet namespace will be exported])
    if test "$rivet_commands_export" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(NAMESPACE_EXPORT,1,[commands will be exported])
    else
        AC_MSG_RESULT([no])
        AC_DEFINE(NAMESPACE_EXPORT,0,[commands will not be exported])
    fi
])

# IMPORT_RIVET_COMMANDS (--enable-import-rivet-commands).
# For compatibility the module can be compiled and installed forcing rivet
# to import commands from ::rivet into the global namespace.
# Default: no

AC_DEFUN([IMPORT_RIVET_COMMANDS],[
    AC_ARG_ENABLE(
        import-rivet-commands,
        [  --enable-import-rivet-commands requires explicit namespace import],
        [ import_rivet_commands=$enable_import_rivet_commands],
        [ import_rivet_commands="no"]
    )

    AC_MSG_CHECKING([if ::rivet namespace will be automatically imported for compatibility])
    if test "$import_rivet_commands" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(NAMESPACE_IMPORT,1,[commands will be imported into the global namespace])
    else
        AC_MSG_RESULT([no])
        AC_DEFINE(NAMESPACE_IMPORT,0,[good, no automatic import will be done])
    fi
])


# SEPARATE_VIRTUAL_INTERPS (--enable-virtual-interps-separation)
# Virtual hosts get their own interpreter and configuration. Different
# applications running on different virtual hosts don't mix up variables
# and namespaces, avoiding conflicts.
# Default: no

AC_DEFUN([VIRTUAL_INTERPS_SEPARATION],[
    AC_ARG_ENABLE(
    virtual-interps-separation,
    [  --enable-virtual-interps-separation to turn on virtual host separation],
    [ separate_virtual_interps=$enable_virtual_interps_separation ],
    [ separate_virtual_interps="no"]
    )

    AC_MSG_CHECKING([whether Rivet will create an interpreter for each virtual host])
    if test "$separate_virtual_interps" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(SEPARATE_VIRTUAL_INTERPS,1,[virtual hosts will have their own interp])
    else
        AC_MSG_RESULT([no])
        AC_DEFINE(SEPARATE_VIRTUAL_INTERPS,0,[one interpreter per child])
    fi
])

# POST_MAX_SIZE (--with-post-max)
# Maximum size of data posted by a form
# Default: 0=unlimited

AC_DEFUN([POST_MAX_SIZE],[
    AC_ARG_WITH(
    post_max,
    [  --with-post-max=BYTES Maximum size of data to be sent with a POST],,
        with_post_max=0
    )
    
    AC_MSG_CHECKING([maximum size of a POST])
    MAX_POST=${with_post_max}
    AC_SUBST(MAX_POST)
    if test $with_post_max = 0; then
        AC_MSG_RESULT([unlimited size])
    else
        AC_MSG_RESULT([$with_post_max])
    fi
])

# UPLOAD_TO_VAR (--enable-upload-var)
# Enables upload of files into Tcl variables
# Default: 1=enabled

AC_DEFUN([UPLOAD_TO_VAR],[
    AC_ARG_ENABLE(
    upload-var,
    [  --disable-upload-var to disable automatic upload to variables ],
    [ upload_var=$enable_upload_var ],
    [ upload_var="yes"]
    )

    AC_MSG_CHECKING([whether files are uploaded to Tcl variables])
    if test "$upload_var" = "yes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(UPLOAD_FILES_TO_VAR,1,[Files are uploaded to Tcl variables])
    else
        AC_MSG_RESULT([no])
        AC_DEFINE(UPLOAD_FILES_TO_VAR,0,[uploads go to files])
    fi

])

#APACHE_VERSION
MOD_RIVET_CORE
GET_RIVET_BASE
APACHE
CHECK_APXS
APACHE_INCLUDES
APR_HANDLING
APU_HANDLING
RIVET_TCL_LIB
DISPLAY_RIVET_VERSION
UPLOAD_DIRECTORY
HONOR_HEAD_REQUESTS
VIRTUAL_INTERPS_SEPARATION
POST_MAX_SIZE
UPLOAD_TO_VAR
IMPORT_RIVET_COMMANDS
APACHE_REQUEST
RIVET_CORE_CMDS
RIVET_CHANNEL
SINGLE_WORKER_TCL_THREAD
HTTP_REQUESTS_MUTEX
BUILD_DEBUG_CODE
AC_HEADER_STDBOOL

# Let's separate the point version from the major and minor version
# to build a Rivet version to be substituted as basic version for 
# package Rivet, central initialization of the Tcl environment

VERSION=${PACKAGE_VERSION}
AX_SPLIT_VERSION
AC_DEFINE_UNQUOTED(INIT_VERSION,"${AX_MAJOR_VERSION}.${AX_MINOR_VERSION}",[Rivet Tcl package version])
AC_MSG_NOTICE([Rivet package version is ${AX_MAJOR_VERSION}.${AX_MINOR_VERSION}])
AC_SUBST(INIT_VERSION,"${AX_MAJOR_VERSION}.${AX_MINOR_VERSION}")
AC_SUBST(RIVETLIB_PACKAGE,"rivetlib",[rivetlib package name])
AC_SUBST(RIVETLIB_PACKAGE_VERSION,"${AX_MAJOR_VERSION}.${AX_MINOR_VERSION}")

if test $import_rivet_commands = "yes"; then
    AC_MSG_NOTICE([forcing Rivet to export commands from ::rivet namespace])
    AC_DEFINE(NAMESPACE_EXPORT,1,[commands will be exported])
else
    RIVET_COMMANDS_EXPORT
fi
AC_C_INLINE

#AC_SUBST(apache_version_dir)
AC_SUBST(MOD_RIVET_INCLUDES)

AC_DEFINE_UNQUOTED(RIVETLIB_DESTDIR,"${RIVET_TCL_TARGET}",[The path to the rivet tcl library])
AC_DEFINE_UNQUOTED(NAMEOFEXECUTABLE,"${TCLSH_PROG}",[The path to a working tclsh executable])
AC_DEFINE_UNQUOTED(UPLOAD_DIR,"${RIVET_UPLOAD_DIR}",[Path to the disk directory where uploads are saved])
AC_DEFINE_UNQUOTED(MAX_POST,[$MAX_POST],[Max size of data in POST operations])
AC_DEFINE_UNQUOTED(SEPARATE_CHANNELS,0,[Separate Channels for virtual hosts])
AC_DEFINE_UNQUOTED(CONFIGURE_CMD,"${CONFIGURE_CMD}",[configure command string])
AC_DEFINE_UNQUOTED(RIVET_CORE,"${rivet_core}",[mod_rivet core])

# We need to use the package path for the installation procedure.  On
#Debian linux TCL_PACKAGE_PATH may have more than one path in the
#TCL_PACKAGE_PATH variable, so we have to handle this
HANDLE_TCL_PACKAGE_PATH
AC_SUBST(TCL_PACKAGE_PATH)


# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting the AC variables in.  Include these here.
#--------------------------------------------------------------------
AC_CONFIG_FILES([Makefile src/Makefile rivet/init.tcl doc/Makefile doc/convert_examples.tcl doc/rivet.xml])

#case $apache_version_dir in
#    apache-1) AC_CONFIG_FILES([Makefile src/Makefile src/apache-1/Makefile doc/Makefile]) ;;
#    apache-2) AC_CONFIG_FILES([Makefile src/Makefile src/apache-2/Makefile doc/Makefile]) ;;
#esac
AC_OUTPUT

if test "$print_fileevent_msg" = "1"; then
    AC_MSG_NOTICE([========================================================================])
    AC_MSG_NOTICE([                             WARNING!])
    AC_MSG_NOTICE([========================================================================])
    AC_MSG_NOTICE([The Tcl notifier (and consequently the event loop) does not work with])
    AC_MSG_NOTICE([Rivet running Tcl $TCL_VERSION$TCL_PATCH_LEVEL and the 'prefork' MPM of Apache.])
    AC_MSG_NOTICE([In order to have asynchronous I/O working the required versions are:])
    AC_MSG_NOTICE([   - threaded builds: Tcl >= 8.5.15 (8.5 version) or Tcl >= 8.6.1])
    AC_MSG_NOTICE([   - any non-threaded build of Tcl >= 8.5.10])
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([Nonetheless Rivet scripts work with Tcl >= 8.5.10 provided])
    AC_MSG_NOTICE([you do not have to rely on the event loop to do I/O (e.g. you])
    AC_MSG_NOTICE([cannot use the 'fileevent' command to set up callbacks)])
    AC_MSG_NOTICE([see https://issues.apache.org/bugzilla/show_bug.cgi?id=55153])
    AC_MSG_NOTICE([========================================================================])
fi 

