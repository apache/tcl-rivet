/*
 * config.h.cmake - CMake generator template for config.h 
 * Each field bracketed by @'s is corresponds to a variable
 * that is either set/not set by the CMakeLists.txt config
 * script.
 */ 
 
/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD @AC_APPLE_UNIVERSAL_BUILD@

/* configure command string */
#cmakedefine CONFIGURE_CMD "@CONFIGURE_CMD@"

/* Display Rivet version in Apache signature */
#cmakedefine DISPLAY_VERSION @DISPLAY_VERSION@

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H @HAVE_DLFCN_H@

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H @HAVE_INTTYPES_H@

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H @HAVE_LIMITS_H@

/* Define to 1 if you have the `lseek64' function. */
#cmakedefine HAVE_LSEEK64 @HAVE_LSEEK64@

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H @HAVE_MEMORY_H@

/* Define to 1 if you have the <net/errno.h> header file. */
#cmakedefine HAVE_NET_ERRNO_H @HAVE_NET_ERRNO_H@

/* Define to 1 if you have the `open64' function. */
#cmakedefine HAVE_OPEN64 @HAVE_OPEN64@

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H @HAVE_STDINT_H@

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H @HAVE_STDLIB_H@

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H @HAVE_STRINGS_H@

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H @HAVE_STRING_H@

/* Is 'struct dirent64' in <sys/types.h>? */
/* #undef HAVE_STRUCT_DIRENT64 */

/* Is 'struct stat64' in <sys/stat.h>? */
/* #undef HAVE_STRUCT_STAT64 */

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HAVE_SYS_PARAM_H @HAVE_SYS_PARAM_H@

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H @HAVE_SYS_STAT_H@

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H @HAVE_SYS_TYPES_H@

/* Is off64_t in <sys/types.h>? */
/* #undef HAVE_TYPE_OFF64_T */

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H @HAVE_UNISTD_H@

/* Honor HEAD requests */
#cmakedefine01 HEAD_REQUESTS

/* Rivet Tcl package version */
#cmakedefine INIT_VERSION "@INIT_VERSION@"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#cmakedefine LT_OBJDIR "@LT_OBJDIR@"
#ifndef RIVET_LT_OBJDIR
#define RIVET_LT_OBJDIR ".libs/"
#endif

/* Max size of data in POST operations */
#define MAX_POST @MAX_POST@

/* No Compiler support for module scope symbols */
#ifndef RIVET_MODULE_SCOPE
#define RIVET_MODULE_SCOPE extern __attribute__((__visibility__("hidden")))
#endif

/* yes, MPM worker single thread */
#cmakedefine MPM_SINGLE_TCL_THREAD @MPM_SINGLE_TCL_THREAD@

/* The path to a working tclsh executable */
#ifndef RIVET_NAMEOFEXECUTABLE
#define NAMEOFEXECUTABLE "@NAMEOFEXECUTABLE@"
#endif

/* commands will not be exported */
#cmakedefine01 NAMESPACE_EXPORT

/* good, no automatic import will be done */
#cmakedefine01 NAMESPACE_IMPORT

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.  */
#cmakedefine HAVE_DIRENT_H @HAVE_DIRENT_H@
#cmakedefine NO_DIRENT_H @NO_DIRENT_H@

/* Do we have <dlfcn.h>? */
#cmakedefine NO_DLFCN_H @NO_DLFCN_H@

/* Do we have <errno.h>? */
#cmakedefine NO_ERRNO_H @NO_ERRNO_H@

/* Do we have <float.h>? */
#cmakedefine NO_FLOAT_H @NO_FLOAT_H@

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H @HAVE_LIMITS_H@
#cmakedefine NO_LIMITS_H @NO_LIMITS_H@

/* Do we have <stdlib.h>? */
#cmakedefine NO_STDLIB_H @NO_STDLIB_H@

/* Do we have <string.h>? */
#cmakedefine NO_STRING_H @NO_STRING_H@

/* Do we have <sys/wait.h>? */
#cmakedefine NO_SYS_WAIT_H @NO_SYS_WAIT_H@

/* Do we have <values.h>? */
#cmakedefine NO_VALUES_H @NO_VALUES_H@

/* No description provided for NO_VIZ... */
#cmakedefine NO_VIZ

/* Name of package */
#define PACKAGE "rivet"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "@PROJECT_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@PROJECT_NAME@ @RIVET_VERSION@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "rivet"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "@RIVET_VERSION@"

/* The path to the rivet tcl library */
#ifndef RIVETLIB_DESTDIR
#define RIVETLIB_DESTDIR "@RIVETLIB_DESTDIR@"
#endif

/* mod_rivet core */
#ifndef RIVET_CORE
#define RIVET_CORE "@RIVET_CORE@"
#endif

/* Separate Channels for virtual hosts */
#cmakedefine01 SEPARATE_CHANNELS

/* one interpreter per child */
#cmakedefine01 SEPARATE_VIRTUAL_INTERPS

/* requests will be serialized */
#cmakedefine SERIALIZE_HTTP_REQUESTS @SERIALIZE_HTTP_REQUESTS@

/* Is this a static build? */
#cmakedefine STATIC_BUILD @STATIC_BUILD@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS @STDC_HEADERS@

/* Is memory debugging enabled? */
#cmakedefine TCL_MEM_DEBUG @TCL_MEM_DEBUG@

/* Are we building with threads enabled? */
#cmakedefine TCL_THREADS @TCL_THREADS@

/* Are wide integers to be implemented with C 'long's? */
#cmakedefine TCL_WIDE_INT_IS_LONG @TCL_WIDE_INT_IS_LONG@

/* What type should be used to define wide integers? */
#cmakedefine TCL_WIDE_INT_TYPE @TCL_WIDE_INT_TYPE@

/* UNDER_CE version */
#cmakedefine UNDER_CE @UNDER_CE@

/* Path to the disk directory where uploads are saved */
#define UPLOAD_DIR "@UPLOAD_DIR@"

/* uploads go to files */
#cmakedefine01 UPLOAD_FILES_TO_VAR

/* Do we want to use the threaded memory allocator? */
#cmakedefine USE_THREAD_ALLOC @USE_THREAD_ALLOC@

/* Version number of package */
#define VERSION "@RIVET_VERSION@"

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
#cmakedefine _ISOC99_SOURCE @_ISOC99_SOURCE@

/* Add the _LARGEFILE64_SOURCE flag when building */
#cmakedefine _LARGEFILE64_SOURCE @_LARGEFILE64_SOURCE@

/* Add the _LARGEFILE_SOURCE64 flag when building */
#cmakedefine _LARGEFILE_SOURCE64 @_LARGEFILE_SOURCE64@

/* # needed in sys/socket.h Should OS/390 do the right thing with sockets? */
#cmakedefine _OE_SOCKETS @_OE_SOCKETS@

/* Do we really want to follow the standard? Yes we do! */
#cmakedefine _POSIX_PTHREAD_SEMANTICS @_POSIX_PTHREAD_SEMANTICS@

/* Do we want the reentrant OS API? */
#cmakedefine _REENTRANT @_REENTRANT@

/* Do we want the thread-safe OS API? */
#cmakedefine _THREAD_SAFE @_THREAD_SAFE@

/* _WIN32_WCE version */
#cmakedefine _WIN32_WCE @_WIN32_WCE@

/* Do we want to use the XOPEN network library? */
#cmakedefine _XOPEN_SOURCE_EXTENDED @_XOPEN_SOURCE_EXTENDED@

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline 
#endif
/*
 * vim: syntax=c
 */ 
