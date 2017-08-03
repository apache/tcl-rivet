#ifndef _RIVET_CONFIG_H
#define _RIVET_CONFIG_H 1
 
/*
 * rivet_config.h. CMake generator template for rivet_config.h
 * Each field bracketed by @'s is corresponds to a variable
 * that is either set/not set by the CMakeLists.txt config
 * script.
 */

/* Define if building universal (internal helper macro) */
#cmakedefine RIVET_AC_APPLE_UNIVERSAL_BUILD @AC_APPLE_UNIVERSAL_BUILD@

/* configure command string */
#cmakedefine RIVET_CONFIGURE_CMD "@CONFIGURE_CMD@"

/* Display Rivet version in Apache signature */
#cmakedefine RIVET_DISPLAY_VERSION @DISPLAY_VERSION@

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine RIVET_HAVE_DLFCN_H @HAVE_DLFCN_H@

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine RIVET_HAVE_INTTYPES_H @HAVE_INTTYPES_H@

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine RIVET_HAVE_LIMITS_H @HAVE_LIMITS_H@

/* Define to 1 if you have the `lseek64' function. */
#cmakedefine RIVET_HAVE_LSEEK64 @HAVE_LSEEK64@

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine RIVET_HAVE_MEMORY_H @HAVE_MEMORY_H@

/* Define to 1 if you have the <net/errno.h> header file. */
#cmakedefine RIVET_HAVE_NET_ERRNO_H @HAVE_NET_ERRNO_H@

/* Define to 1 if you have the `open64' function. */
#cmakedefine RIVET_HAVE_OPEN64 @HAVE_OPEN64@

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine RIVET_HAVE_STDINT_H @HAVE_STDINT_H@

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine RIVET_HAVE_STDLIB_H @HAVE_STDLIB_H@

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine RIVET_HAVE_STRINGS_H @HAVE_STRINGS_H@

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine RIVET_HAVE_STRING_H @HAVE_STRING_H@

/* Is 'struct dirent64' in <sys/types.h>? */
/* #undef HAVE_STRUCT_DIRENT64 */

/* Is 'struct stat64' in <sys/stat.h>? */
/* #undef HAVE_STRUCT_STAT64 */

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine RIVET_HAVE_SYS_PARAM_H @HAVE_SYS_PARAM_H@

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine RIVET_HAVE_SYS_STAT_H @HAVE_SYS_STAT_H@

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine RIVET_HAVE_SYS_TYPES_H @HAVE_SYS_TYPES_H@

/* Is off64_t in <sys/types.h>? */
/* #undef HAVE_TYPE_OFF64_T */

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine RIVET_HAVE_UNISTD_H @HAVE_UNISTD_H@

/* Honor HEAD requests */
#cmakedefine01 RIVET_HEAD_REQUESTS

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
#cmakedefine RIVET_MPM_SINGLE_TCL_THREAD @MPM_SINGLE_TCL_THREAD@

/* The path to a working tclsh executable */
#ifndef RIVET_NAMEOFEXECUTABLE
#define RIVET_NAMEOFEXECUTABLE "@NAMEOFEXECUTABLE@"
#endif

/* commands will not be exported */
#cmakedefine01 RIVET_NAMESPACE_EXPORT

/* good, no automatic import will be done */
#cmakedefine01 RIVET_NAMESPACE_IMPORT

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.  */
#cmakedefine RIVET_HAVE_DIRENT_H @HAVE_DIRENT_H@
#cmakedefine RIVET_NO_DIRENT_H @NO_DIRENT_H@

/* Do we have <dlfcn.h>? */
#cmakedefine RIVET_NO_DLFCN_H @NO_DLFCN_H@

/* Do we have <errno.h>? */
#cmakedefine RIVET_NO_ERRNO_H @NO_ERRNO_H@

/* Do we have <float.h>? */
#cmakedefine RIVET_NO_FLOAT_H @NO_FLOAT_H@

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine RIVET_HAVE_LIMITS_H @HAVE_LIMITS_H@
#cmakedefine RIVET_NO_LIMITS_H @NO_LIMITS_H@

/* Do we have <stdlib.h>? */
#cmakedefine RIVET_NO_STDLIB_H @NO_STDLIB_H@

/* Do we have <string.h>? */
#cmakedefine RIVET_NO_STRING_H @NO_STRING_H@

/* Do we have <sys/wait.h>? */
#cmakedefine RIVET_NO_SYS_WAIT_H @NO_SYS_WAIT_H@

/* Do we have <values.h>? */
#cmakedefine RIVET_NO_VALUES_H @NO_VALUES_H@

/* No description provided for NO_VIZ... */
#cmakedefine RIVET_NO_VIZ

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
#cmakedefine RIVET_SERIALIZE_HTTP_REQUESTS @SERIALIZE_HTTP_REQUESTS@

/* Is this a static build? */
#cmakedefine RIVET_STATIC_BUILD @STATIC_BUILD@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine RIVET_STDC_HEADERS @STDC_HEADERS@

/* Is memory debugging enabled? */
#cmakedefine RIVET_TCL_MEM_DEBUG @TCL_MEM_DEBUG@

/* Are we building with threads enabled? */
#cmakedefine RIVET_TCL_THREADS @TCL_THREADS@

/* Are wide integers to be implemented with C 'long's? */
#cmakedefine RIVET_TCL_WIDE_INT_IS_LONG @TCL_WIDE_INT_IS_LONG@

/* What type should be used to define wide integers? */
#cmakedefine RIVET_TCL_WIDE_INT_TYPE @TCL_WIDE_INT_TYPE@

/* UNDER_CE version */
#cmakedefine RIVET_UNDER_CE @UNDER_CE@

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
#cmakedefine RIVET__ISOC99_SOURCE @_ISOC99_SOURCE@

/* Add the _LARGEFILE64_SOURCE flag when building */
#ifndef RIVET__LARGEFILE64_SOURCE
#cmakedefine RIVET__LARGEFILE64_SOURCE @_LARGEFILE64_SOURCE@
#endif

/* Add the _LARGEFILE_SOURCE64 flag when building */
#cmakedefine RIVET__LARGEFILE_SOURCE64 @_LARGEFILE_SOURCE64@

/* # needed in sys/socket.h Should OS/390 do the right thing with sockets? */
#cmakedefine RIVET__OE_SOCKETS @_OE_SOCKETS@

/* Do we really want to follow the standard? Yes we do! */
#cmakedefine RIVET__POSIX_PTHREAD_SEMANTICS @_POSIX_PTHREAD_SEMANTICS@

/* Do we want the reentrant OS API? */
#cmakedefine RIVET__REENTRANT @_REENTRANT@

/* Do we want the thread-safe OS API? */
#cmakedefine RIVET__THREAD_SAFE @_THREAD_SAFE@

/* _WIN32_WCE version */
#cmakedefine RIVET__WIN32_WCE @_WIN32_WCE@

/* Do we want to use the XOPEN network library? */
#cmakedefine RIVET__XOPEN_SOURCE_EXTENDED @_XOPEN_SOURCE_EXTENDED@

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#ifndef _rivet_inline
#define _rivet_inline 
#endif
#endif
 
/* once: _RIVET_CONFIG_H */
#endif

