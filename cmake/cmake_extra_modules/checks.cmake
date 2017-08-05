# Check headers
include(CheckIncludeFile)
include(CheckIncludeFiles)
check_include_files(dlfcn.h        HAVE_DLFCN_H)
check_include_files(inttypes.h     HAVE_INTTYPES_H)
check_include_files(limits.h       HAVE_LIMITS_H)
check_include_files(memory.h       HAVE_MEMORY_H)
check_include_files(net/errno.h    HAVE_NET_ERRNO_H)
check_include_files(stdint.h       HAVE_STDINT_H)
check_include_files(stdlib.h       HAVE_STDLIB_H)
check_include_files(strings.h      HAVE_STRINGS_H)
check_include_files(string.h       HAVE_STRING_H)
check_include_files(sys/param.h    HAVE_SYS_PARAM_H)
check_include_files(sys/stat.h     HAVE_SYS_STAT_H )
check_include_files(sys/types.h    HAVE_SYS_TYPES_H )
check_include_files(unistd.h       HAVE_UNISTD_H )
check_include_files(dirent.h       HAVE_DIRENT_H)
check_include_files(limits.h       HAVE_LIMITS_H)
check_include_files(alloca.h       HAVE_ALLOCA_H )
check_include_files(stdint.h       HAVE_STDINT_H )
check_include_files(sys/mman.h     HAVE_SYS_MMAN_H)
check_include_files(errno.h        HAVE_ERRNO_H)
check_include_files(float.h        HAVE_FLOAT_H)
check_include_files(sys/wait.h     HAVE_SYS_WAIT_H)
check_include_files(values.h       HAVE_VALUES_H)
check_include_files(stdarg.h       HAVE_STDARG_H)

# Check functions
include(CheckFunctionExists)
check_function_exists(fseek64      HAVE_FSEEK64)
check_function_exists(open64       HAVE_OPEN64)
check_function_exists(memcpy       HAVE_MEMCPY)
check_function_exists(mmap         HAVE_MMAP)
check_function_exists(lseek64      HAVE_LSEEK64)
set(CMAKE_EXTRA_INCLUDE_FILES math.h)
check_function_exists(round        HAVE_ROUND)
set(CMAKE_EXTRA_INCLUDE_FILES)

# Check types
include ( CheckTypeSize )
check_type_size ( "long double" HAVE_LONG_DOUBLE )
check_type_size ( "double"      SIZEOF_DOUBLE )
check_type_size ( "long double" SIZEOF_LONG_DOUBLE )
check_type_size ( "void*"       SIZEOF_VOID_P )
# set(CMAKE_EXTRA_INCLUDE_FILES sys/stat.h)
# check_type_size("struct stat64" _LARGEFILE64_SOURCE)
# set(CMAKE_EXTRA_INCLUDE_FILES)

# Check symbols
include(CheckSymbolExists) 

# Custom checks...
include (CheckCSourceCompiles)
check_c_source_compiles("#include <sys/types.h>
int main () { off64_t offset; return 0;}" HAVE_TYPE_OFF64_T)

check_c_source_compiles("#include <sys/stat.h>
int main () { struct stat64 p; return 0;}" HAVE_STRUCT_STAT64)
check_c_source_compiles("#include <sys/types.h>
#include <sys/dirent.h>
int main () { struct dirent64 p; return 0;}" HAVE_STRUCT_DIRENT64)

check_c_source_compiles("#include <sys/stat.h>
int main () {struct stat64 buf; int i = stat64(\"/\", &buf); return 0;}"
NO_LARGEFILE64_SOURCE)
if(NOT NO_LARGEFILE64_SOURCE)
check_c_source_compiles("#define _LARGEFILE64_SOURCE 1
#include <sys/stat.h>
int main () {struct stat64 buf; int i = stat64(\"/\", &buf); return 0;}"
                        _LARGEFILE64_SOURCE)
endif(NOT NO_LARGEFILE64_SOURCE)

check_c_source_compiles("#include <stdlib.h>
int main () {char *p = (char *)strtoll; char *q = (char *)strtoull; return 0;}"
NO_ISOC99_SOURCE)
if(NOT NO_ISOC99_SOURCE)
check_c_source_compiles("#define _ISOC99_SOURCE 1
#include <stdlib.h>
int main () {char *p = (char *)strtoll; char *q = (char *)strtoull; return 0;}"
                        _ISOC99_SOURCE)
endif(NOT NO_ISOC99_SOURCE)

check_c_source_compiles("#include <sys/stat.h>
int main () {char *p = (char *)open64; return 0;}"
NO_LARGEFILE_SOURCE64)
if(NOT NO_LARGEFILE_SOURCE64)
check_c_source_compiles("#define _LARGEFILE_SOURCE64 1
#include <sys/stat.h>
int main () {char *p = (char *)open64; return 0;}"
                        _LARGEFILE_SOURCE64)
endif(NOT NO_LARGEFILE_SOURCE64)

# See if the compiler knows natively about __int64
set(tcl_cv_type_64bit "none")
check_c_source_compiles("int main () {__int64 value = (__int64) 0; return 0;}"
HAS___int64)
if(HAS___int64)
  set(tcl_type_64bit "__int64")
else(HAS___int64)
  set(tcl_type_64bit "long long")
endif(HAS___int64)
check_c_source_compiles("int main () {switch (0) {case 1:
case (sizeof(${tcl_type_64bit})==sizeof(long)): ; }; return 0;}"
HAS_WIDE_INT_NE_LONG)
if(HAS_WIDE_INT_NE_LONG)
  set(tcl_cv_type_64bit ${tcl_type_64bit})
endif(HAS_WIDE_INT_NE_LONG)

if(${tcl_cv_type_64bit} STREQUAL "none")
  set(TCL_WIDE_INT_IS_LONG 1)
elseif((${tcl_cv_type_64bit} STREQUAL "__int64") AND WIN32)
else(${tcl_cv_type_64bit} STREQUAL "none")
  set(TCL_WIDE_INT_TYPE ${tcl_cv_type_64bit})
endif(${tcl_cv_type_64bit} STREQUAL "none")

#  Check for ANSI C header files...
# ==========================================================================
message(STATUS "Cheking for ANSI C header files...")
if(HAVE_STDLIB_H AND HAVE_STDARG_H AND HAVE_STRING_H AND HAVE_FLOAT_H)
  set(ac_cv_header_stdc ON)
endif(HAVE_STDLIB_H AND HAVE_STDARG_H AND HAVE_STRING_H AND HAVE_FLOAT_H)

if(ac_cv_header_stdc)
  # SunOS 4.x string.h does not declare mem*, contrary to ANSI.
  SET(CMAKE_EXTRA_INCLUDE_FILES string.h)
  check_function_exists(memchr ac_cv_header_stdc)
  SET(CMAKE_EXTRA_INCLUDE_FILES)
endif(ac_cv_header_stdc)

if(ac_cv_header_stdc)
  # ISC 2.0.2 stdlib.h does not declare free, contrary to ANSI.
  SET(CMAKE_EXTRA_INCLUDE_FILES stdlib.h)
  check_function_exists(free ac_cv_header_stdc)
  SET(CMAKE_EXTRA_INCLUDE_FILES)
endif(ac_cv_header_stdc)

if(ac_cv_header_stdc)
  set(STDC_HEADERS 1)
endif(ac_cv_header_stdc)

if(NOT HAVE_DIRENT_H)
  set(NO_DIRENT_H 1)
endif()

if(NOT HAVE_DLFCN_H)
  set(NO_DLFCN_H 1)
endif()

if(NOT HAVE_ERRNO_H)
  set(NO_ERRNO_H 1)
endif()

if(NOT HAVE_FLOAT_H)
  set(NO_FLOAT_H 1)
endif()

if(NOT HAVE_LIMITS_H)
  set(NO_LIMITS_H 1)
endif()

if(NOT HAVE_SYS_WAIT_H)
  set(NO_SYS_WAIT_H 1)
endif()

if(NOT HAVE_VALUES_H)
  set(NO_VALUES_H 1)
endif()

if(NOT HAVE_STDLIB_H)
  set(NO_STDLIB_H 1)
endif()

if(NOT HAVE_STRING_H)
  set(NO_STRING_H 1)
endif()

if(NOT HAVE_ROUND)
  set(NO_HAVE_ROUND 1)
endif()

if(BUILD_STATIC_LIBS)
  set(STATIC_BUILD 1)
endif(BUILD_STATIC_LIBS)
