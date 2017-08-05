#ifndef _RIVET_CONFIG_H
#define _RIVET_CONFIG_H 1
 
/*
 * rivet_config.h. CMake generator template for rivet_config.h
 * Each field bracketed by @'s is corresponds to a variable
 * that is either set/not set by the CMakeLists.txt config
 * script.
 */

/* Define if building universal (internal helper macro) */
#ifndef RIVET_AC_APPLE_UNIVERSAL_BUILD
#cmakedefine RIVET_AC_APPLE_UNIVERSAL_BUILD @AC_APPLE_UNIVERSAL_BUILD@
#endif

/* configure command string */
#ifndef RIVET_CONFIGURE_CMD
#cmakedefine RIVET_CONFIGURE_CMD "@CONFIGURE_CMD@"
#endif

/* Display Rivet version in Apache signature */
#ifndef RIVET_DISPLAY_VERSION
#cmakedefine01 RIVET_DISPLAY_VERSION
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef RIVET_HAVE_DLFCN_H
#cmakedefine RIVET_HAVE_DLFCN_H @HAVE_DLFCN_H@
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef RIVET_HAVE_INTTYPES_H
#cmakedefine RIVET_HAVE_INTTYPES_H @HAVE_INTTYPES_H@
#endif

/* Define to 1 if you have the <limits.h> header file. */
#ifndef RIVET_HAVE_LIMITS_H
#cmakedefine RIVET_HAVE_LIMITS_H @HAVE_LIMITS_H@
#endif

/* Define to 1 if you have the `lseek64' function. */
#ifndef RIVET_HAVE_LSEEK64
#cmakedefine RIVET_HAVE_LSEEK64 @HAVE_LSEEK64@
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef RIVET_HAVE_MEMORY_H
#cmakedefine RIVET_HAVE_MEMORY_H @HAVE_MEMORY_H@
#endif

/* Define to 1 if you have the <net/errno.h> header file. */
#ifndef RIVET_HAVE_NET_ERRNO_H
#cmakedefine RIVET_HAVE_NET_ERRNO_H @HAVE_NET_ERRNO_H@
#endif

/* Define to 1 if you have the `open64' function. */
#ifndef RIVET_HAVE_OPEN64
#cmakedefine RIVET_HAVE_OPEN64 @HAVE_OPEN64@
#endif

/* Define to 1 if you have the `round' function. */
#ifndef RIVET_HAVE_ROUND
#cmakedefine RIVET_HAVE_ROUND @HAVE_ROUND@
#endif
#ifndef RIVET_NO_HAVE_ROUND
#cmakedefine RIVET_NO_HAVE_ROUND @NO_HAVE_ROUND@
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef RIVET_HAVE_STDINT_H
#cmakedefine RIVET_HAVE_STDINT_H @HAVE_STDINT_H@
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef RIVET_HAVE_STDLIB_H
#cmakedefine RIVET_HAVE_STDLIB_H @HAVE_STDLIB_H@
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef RIVET_HAVE_STRINGS_H
#cmakedefine RIVET_HAVE_STRINGS_H @HAVE_STRINGS_H@
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef RIVET_HAVE_STRING_H
#cmakedefine RIVET_HAVE_STRING_H @HAVE_STRING_H@
#endif

/* Is 'struct dirent64' in <sys/types.h>? */
#ifndef RIVET_HAVE_STRUCT_DIRENT64
#cmakedefine RIVET_HAVE_STRUCT_DIRENT64 @HAVE_STRUCT_DIRENT64@
#endif

/* Is 'struct stat64' in <sys/stat.h>? */
#ifndef RIVET_HAVE_STRUCT_STAT64
#cmakedefine RIVET_HAVE_STRUCT_STAT64 @HAVE_STRUCT_STAT64@
#endif

/* Define to 1 if you have the <sys/param.h> header file. */
#ifndef RIVET_HAVE_SYS_PARAM_H
#cmakedefine RIVET_HAVE_SYS_PARAM_H @HAVE_SYS_PARAM_H@
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef RIVET_HAVE_SYS_STAT_H
#cmakedefine RIVET_HAVE_SYS_STAT_H @HAVE_SYS_STAT_H@
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef RIVET_HAVE_SYS_TYPES_H
#cmakedefine RIVET_HAVE_SYS_TYPES_H @HAVE_SYS_TYPES_H@
#endif

/* Is off64_t in <sys/types.h>? */
#ifndef RIVET_HAVE_TYPE_OFF64_T
#cmakedefine RIVET_HAVE_TYPE_OFF64_T @HAVE_TYPE_OFF64_T@
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef RIVET_HAVE_UNISTD_H
#cmakedefine RIVET_HAVE_UNISTD_H @HAVE_UNISTD_H@
#endif

/* Honor HEAD requests */
#ifndef RIVET_HEAD_REQUESTS
#cmakedefine01 RIVET_HEAD_REQUESTS
#endif

/* Rivet Tcl package version */
#ifndef RIVET_INIT_VERSION
#define RIVET_INIT_VERSION "@INIT_VERSION@"
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef RIVET_LT_OBJDIR
#define RIVET_LT_OBJDIR ".libs/"
#endif

/* Max size of data in POST operations */
#ifndef RIVET_MAX_POST
#define RIVET_MAX_POST @MAX_POST@
#endif

/* No Compiler support for module scope symbols */
#ifndef RIVET_MODULE_SCOPE
#define RIVET_MODULE_SCOPE extern __attribute__((__visibility__("hidden")))
#endif

/* yes, MPM worker single thread */
#ifndef RIVET_MPM_SINGLE_TCL_THREAD
#cmakedefine RIVET_MPM_SINGLE_TCL_THREAD @MPM_SINGLE_TCL_THREAD@
#endif

/* The path to a working tclsh executable */
#ifndef RIVET_NAMEOFEXECUTABLE
#define RIVET_NAMEOFEXECUTABLE "@NAMEOFEXECUTABLE@"
#endif

/* commands will not be exported */
#ifndef RIVET_NAMESPACE_EXPORT
#cmakedefine01 RIVET_NAMESPACE_EXPORT
#endif

/* good, no automatic import will be done */
#ifndef RIVET_NAMESPACE_IMPORT
#cmakedefine01 RIVET_NAMESPACE_IMPORT
#endif

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.  */
#ifndef RIVET_HAVE_DIRENT_H
#cmakedefine RIVET_HAVE_DIRENT_H @HAVE_DIRENT_H@
#endif
#ifndef RIVET_NO_DIRENT_H
#cmakedefine RIVET_NO_DIRENT_H @NO_DIRENT_H@
#endif

/* Do we have <dlfcn.h>? */
#ifndef RIVET_NO_DLFCN_H
#cmakedefine RIVET_NO_DLFCN_H @NO_DLFCN_H@
#endif

/* Do we have <errno.h>? */
#ifndef RIVET_NO_ERRNO_H
#cmakedefine RIVET_NO_ERRNO_H @NO_ERRNO_H@
#endif

/* Do we have <float.h>? */
#ifndef RIVET_NO_FLOAT_H
#cmakedefine RIVET_NO_FLOAT_H @NO_FLOAT_H@
#endif

/* Define to 1 if you have the <limits.h> header file. */
#ifndef RIVET_HAVE_LIMITS_H
#cmakedefine RIVET_HAVE_LIMITS_H @HAVE_LIMITS_H@
#endif
#ifndef RIVET_NO_LIMITS_H
#cmakedefine RIVET_NO_LIMITS_H @NO_LIMITS_H@
#endif

/* Do we have <stdlib.h>? */
#ifndef RIVET_NO_STDLIB_H
#cmakedefine RIVET_NO_STDLIB_H @NO_STDLIB_H@
#endif

/* Do we have <string.h>? */
#ifndef RIVET_NO_STRING_H
#cmakedefine RIVET_NO_STRING_H @NO_STRING_H@
#endif

/* Do we have <sys/wait.h>? */
#ifndef RIVET_NO_SYS_WAIT_H
#cmakedefine RIVET_NO_SYS_WAIT_H @NO_SYS_WAIT_H@
#endif

/* Do we have <values.h>? */
#ifndef RIVET_NO_VALUES_H
#cmakedefine RIVET_NO_VALUES_H @NO_VALUES_H@
#endif

/* No description provided for NO_VIZ... */
#ifndef RIVET_NO_VIZ
#cmakedefine RIVET_NO_VIZ
#endif

/* Name of package */
#ifndef RIVET_PACKAGE
#define RIVET_PACKAGE "rivet"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef RIVET_PACKAGE_BUGREPORT
#define RIVET_PACKAGE_BUGREPORT ""
#endif

/* Define to the full name of this package. */
#ifndef RIVET_PACKAGE_NAME
#define RIVET_PACKAGE_NAME "@PROJECT_NAME@"
#endif

/* Define to the full name and version of this package. */
#ifndef RIVET_PACKAGE_STRING
#define RIVET_PACKAGE_STRING "@PROJECT_NAME@ @RIVET_VERSION@"
#endif

/* Define to the one symbol short name of this package. */
#ifndef RIVET_PACKAGE_TARNAME
#define RIVET_PACKAGE_TARNAME "rivet"
#endif

/* Define to the home page for this package. */
#ifndef RIVET_PACKAGE_URL
#define RIVET_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef RIVET_PACKAGE_VERSION
#define RIVET_PACKAGE_VERSION "@RIVET_VERSION@"
#endif

/* The path to the rivet tcl library */
#ifndef RIVET_RIVETLIB_DESTDIR
#define RIVET_RIVETLIB_DESTDIR "@RIVETLIB_DESTDIR@"
#endif

/* mod_rivet core */
#ifndef RIVET_RIVET_CORE
#define RIVET_RIVET_CORE "@RIVET_CORE@"
#endif

/* Separate Channels for virtual hosts */
#ifndef RIVET_SEPARATE_CHANNELS
#cmakedefine01 RIVET_SEPARATE_CHANNELS
#endif

/* one interpreter per child */
#ifndef RIVET_SEPARATE_VIRTUAL_INTERPS
#cmakedefine01 RIVET_SEPARATE_VIRTUAL_INTERPS
#endif

/* requests will be serialized */
#ifndef RIVET_SERIALIZE_HTTP_REQUESTS
#cmakedefine RIVET_SERIALIZE_HTTP_REQUESTS @SERIALIZE_HTTP_REQUESTS@
#endif

/* Is this a static build? */
#ifndef RIVET_STATIC_BUILD
#cmakedefine RIVET_STATIC_BUILD @STATIC_BUILD@
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef RIVET_STDC_HEADERS
#cmakedefine RIVET_STDC_HEADERS @STDC_HEADERS@
#endif

/* Is memory debugging enabled? */
#ifndef RIVET_TCL_MEM_DEBUG
#cmakedefine RIVET_TCL_MEM_DEBUG @TCL_MEM_DEBUG@
#endif

/* Are we building with threads enabled? */
#ifndef RIVET_TCL_THREADS
#cmakedefine RIVET_TCL_THREADS 1
#endif

/* Are wide integers to be implemented with C 'long's? */
#ifndef RIVET_TCL_WIDE_INT_IS_LONG
#cmakedefine RIVET_TCL_WIDE_INT_IS_LONG @TCL_WIDE_INT_IS_LONG@
#endif

/* What type should be used to define wide integers? */
#ifndef RIVET_TCL_WIDE_INT_TYPE
#cmakedefine RIVET_TCL_WIDE_INT_TYPE @TCL_WIDE_INT_TYPE@
#endif

/* UNDER_CE version */
#ifndef RIVET_UNDER_CE
#cmakedefine RIVET_UNDER_CE @UNDER_CE@
#endif

/* Path to the disk directory where uploads are saved */
#ifndef RIVET_UPLOAD_DIR
#define RIVET_UPLOAD_DIR "@UPLOAD_DIR@"
#endif

/* uploads go to files */
#ifndef RIVET_UPLOAD_FILES_TO_VAR
#cmakedefine01 RIVET_UPLOAD_FILES_TO_VAR
#endif

/* Do we want to use the threaded memory allocator? */
#ifndef RIVET_USE_THREAD_ALLOC
#cmakedefine RIVET_USE_THREAD_ALLOC @USE_THREAD_ALLOC@
#endif

/* Version number of package */
#ifndef RIVET_VERSION
#define RIVET_VERSION "@RIVET_VERSION@"
#endif

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Add the _ISOC99_SOURCE flag when building */
#ifndef RIVET__ISOC99_SOURCE
#cmakedefine RIVET__ISOC99_SOURCE @_ISOC99_SOURCE@
#endif

/* Add the _LARGEFILE64_SOURCE flag when building */
#ifndef RIVET__LARGEFILE64_SOURCE
#cmakedefine RIVET__LARGEFILE64_SOURCE @_LARGEFILE64_SOURCE@
#endif

/* Add the _LARGEFILE_SOURCE64 flag when building */
#ifndef RIVET__LARGEFILE_SOURCE64
#cmakedefine RIVET__LARGEFILE_SOURCE64 @_LARGEFILE_SOURCE64@
#endif

/* # needed in sys/socket.h Should OS/390 do the right thing with sockets? */
#ifndef RIVET__OE_SOCKETS
#cmakedefine RIVET__OE_SOCKETS @_OE_SOCKETS@
#endif

/* Do we really want to follow the standard? Yes we do! */
#ifndef RIVET__POSIX_PTHREAD_SEMANTICS
#cmakedefine RIVET__POSIX_PTHREAD_SEMANTICS @_POSIX_PTHREAD_SEMANTICS@
#endif

/* Do we want the reentrant OS API? */
#ifndef RIVET__REENTRANT
#cmakedefine RIVET__REENTRANT @_REENTRANT@
#endif

/* Do we want the thread-safe OS API? */
#ifndef RIVET__THREAD_SAFE
#cmakedefine RIVET__THREAD_SAFE @_THREAD_SAFE@
#endif

/* _WIN32_WCE version */
#ifndef RIVET__WIN32_WCE
#cmakedefine RIVET__WIN32_WCE @_WIN32_WCE@
#endif

/* Do we want to use the XOPEN network library? */
#ifndef RIVET__XOPEN_SOURCE_EXTENDED
#cmakedefine RIVET__XOPEN_SOURCE_EXTENDED @_XOPEN_SOURCE_EXTENDED@
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#ifndef _rivet_inline
#define _rivet_inline 
#endif
#endif
 
/* once: _RIVET_CONFIG_H */
#endif

