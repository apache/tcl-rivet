# Georgios Petasis, petasis@iit.demokritos.gr

# The minimum version of cmake required. This may work also in 2.8,
# but I have not tested. My cmake is version 3.9.0.
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Rivet version:
set(RIVET_MAJOR_VERSION 3)
set(RIVET_MINOR_VERSION 0)
set(RIVET_PATCH_VERSION 0)
set(RIVET_VERSION
  ${RIVET_MAJOR_VERSION}.${RIVET_MINOR_VERSION}.${RIVET_PATCH_VERSION})
set(INIT_VERSION ${RIVET_MAJOR_VERSION}.${RIVET_MINOR_VERSION})

# Rivetlib version:
set(RIVETLIB_PACKAGE "rivetlib")
set(RIVETLIB_PACKAGE_VERSION ${INIT_VERSION})

# The arguments argument are optional, declares version and language
# (in this case C).
project(Rivet VERSION ${RIVET_VERSION} LANGUAGES C)

#  User options...
# ===========================================================================
set(with-tclsh "" CACHE FILEPATH "location of a working tclsh executable")
set(with-tcl   "" CACHE PATH "directory containing tcl configuration (tclConfig.sh)")
set(with-post-max 0 CACHE STRING  "BYTES Maximum size of data to be sent with a POST")
set(with-rivet-core "mod_rivet_ng" CACHE STRING "mod_rivet core directory")
option(version-display            "Display Rivet version in Apache signature" OFF)
option(head-requests              "Returns real headers in response to a HEAD request" OFF)
option(single-thread              "forces the worker brigde to create a single thread (debug)" OFF)
option(rivet-commands-export      "prevent export from ::rivet namespace" ON)
option(import-rivet-commands      "requires explicit namespace import" OFF)
option(virtual-interps-separation "to turn on virtual host separation" OFF)
option(requests-serialization     "Forces HTTP requests serialization among threads(debug)" OFF)
option(upload-var                 "whether files are uploaded to Tcl variables" ON)
option(BUILD_SHARED_LIBS          "build as shared libraries" ON)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/CMake/Rivet)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")

# Use GNU install directories
include(GNUInstallDirs)

#  Rivet source files...
# ===========================================================================
set(RIVET_SRC_DIR "${PROJECT_SOURCE_DIR}/../src")
set(RIVET_LIB_DIR "${PROJECT_SOURCE_DIR}/../rivet")
set(RIVET_CORE ${with-rivet-core})

set(rivetparser_sources
  ${RIVET_SRC_DIR}/parser/rivetParser.c
  ${RIVET_SRC_DIR}/parser/parserPkgInit.c
)

set(rivetlib_sources
  ${RIVET_SRC_DIR}/librivet/rivetList.c
  ${RIVET_SRC_DIR}/librivet/rivetCrypt.c
  ${RIVET_SRC_DIR}/librivet/rivetWWW.c
  ${RIVET_SRC_DIR}/librivet/rivetPkgInit.c
)

set(rivet_worker_mpm_sources
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivet_worker_mpm.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/worker_prefork_common.c
)

set(rivet_prefork_mpm_sources
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivet_prefork_mpm.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/worker_prefork_common.c
)

set(rivet_lazy_mpm_sources
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivet_lazy_mpm.c
)

set(mod_rivet_sources
  ${RIVET_SRC_DIR}/${RIVET_CORE}/mod_rivet.c
  ${RIVET_SRC_DIR}/request/apache_multipart_buffer.c
  ${RIVET_SRC_DIR}/request/apache_request.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivetCore.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivetInspect.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/rivetChannel.c
  ${RIVET_SRC_DIR}/parser/rivetParser.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/TclWebapache.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/apache_config.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/mod_rivet_cache.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/mod_rivet_common.c
  ${RIVET_SRC_DIR}/${RIVET_CORE}/mod_rivet_generator.c
  ${RIVET_SRC_DIR}/parser/rivetParser.c
)

#  Add build targets...
# ===========================================================================
add_library(mod_rivet            ${mod_rivet_sources})
add_library(rivetparser          ${rivetparser_sources})
add_library(rivetlib             ${rivetlib_sources})
add_library(rivet_worker_mpm     ${rivet_worker_mpm_sources})
add_library(rivet_prefork_mpm    ${rivet_prefork_mpm_sources})
add_library(rivet_lazy_mpm       ${rivet_lazy_mpm_sources})
add_library(rivet::parser        ALIAS rivetparser)
add_library(rivet::lib           ALIAS rivetparser)
add_library(rivet::rivet         ALIAS mod_rivet)
add_library(rivet::worker_mpm    ALIAS rivet_worker_mpm)
add_library(rivet::prefork_mpm   ALIAS rivet_prefork_mpm)
add_library(rivet::lazy_mpm      ALIAS rivet_lazy_mpm)

#  Definitions...
# ===========================================================================
target_compile_definitions (rivetparser       PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 USE_TCL_STUBS=1 )
target_compile_definitions (rivetlib          PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 USE_TCL_STUBS=1 )
target_compile_definitions (rivet_worker_mpm  PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 )
target_compile_definitions (rivet_prefork_mpm PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 )
target_compile_definitions (rivet_lazy_mpm    PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 )
target_compile_definitions (mod_rivet         PRIVATE
   TCL_THREADS=1 HAVE_CONFIG_H=1 )

# ===========================================================================
#  Locate needed packages...
# ===========================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_extra_modules")
include(checks)

#  Locate Tcl...
# ===========================================================================
MESSAGE ( STATUS "Searching for Tcl..." )
if (NOT "${with-tclsh}" STREQUAL "")
  MESSAGE ( STATUS "  manually set by -Dwith-tclsh=" ${with-tclsh} )
  find_program(TCL_TCLSH ${with-tclsh})
endif ()
if (NOT "${with-tcl}" STREQUAL "")
  MESSAGE ( STATUS "  manually set by -Dwith-tcl=" ${with-tcl} )
  find_library(TCL_LIBRARY NAMES tcl
    tcl${TCL_LIBRARY_VERSION} tcl${TCL_TCLSH_VERSION} tcl${TK_WISH_VERSION}
    tcl89 tcl8.9 tcl88 tcl8.8 tcl87 tcl8.7
    tcl86 tcl8.6 tcl85 tcl8.5 tcl84 tcl8.4
    tcl83 tcl8.3 tcl82 tcl8.2 tcl80 tcl8.0
    PATHS ${with-tcl} )
endif ()
FIND_PACKAGE ( TCL 8.5.10 REQUIRED )
FIND_PACKAGE ( TclStub 8.5.10 REQUIRED )
MESSAGE ( STATUS "  TCL_TCLSH:               " ${TCL_TCLSH} )
MESSAGE ( STATUS "  TCL_INCLUDE_PATH:        " ${TCL_INCLUDE_PATH} )
MESSAGE ( STATUS "  TCL_LIBRARY:             " ${TCL_LIBRARY} )
MESSAGE ( STATUS "  TCL_STUB_LIBRARY:        " ${TCL_STUB_LIBRARY} )

#  Locate Apache...
# ===========================================================================
MESSAGE ( STATUS "Searching for Apache..." )
find_package(APACHE REQUIRED)
MESSAGE ( STATUS "  APACHE_INCLUDE_DIR:      " ${APACHE_INCLUDE_DIR} )
MESSAGE ( STATUS "  APACHE_MODULE_DIR:       " ${APACHE_MODULE_DIR} )
MESSAGE ( STATUS "  APACHE_LIB_DIR:          " ${APACHE_LIB_DIR} )

#  Locate Apr...
# ===========================================================================
MESSAGE ( STATUS "Searching for Apr..." )
find_package(APR REQUIRED)
MESSAGE ( STATUS "  APR_INCLUDE_DIR:         " ${APR_INCLUDE_DIR} )
MESSAGE ( STATUS "  APR_LIBRARY:             " ${APR_LIBRARY} )
MESSAGE ( STATUS "  APRUTIL_INCLUDE_DIR:     " ${APRUTIL_INCLUDE_DIR} )
MESSAGE ( STATUS "  APRUTIL_LIBRARY:         " ${APRUTIL_LIBRARY} )

find_package(Threads)

#  Set up include directories...
# ===========================================================================
set(RIVET_INCLUDE_DIRS_PUBLIC
  "${TCL_INCLUDE_PATH}"
  "${APACHE_INCLUDE_DIR}"
  "${APR_INCLUDE_DIR}"
  "${APRUTIL_INCLUDE_DIR}"
)
set(RIVET_INCLUDE_DIRS_PRIVATE
  "${RIVET_SRC_DIR}"
  "${RIVET_SRC_DIR}/${RIVET_CORE}"
  "${RIVET_SRC_DIR}/parser"
  "${RIVET_SRC_DIR}/request"
  "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}"
  "${PROJECT_BINARY_DIR}" 
)

target_include_directories(rivetparser
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})
target_include_directories(rivetlib
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})
target_include_directories(rivet_worker_mpm
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})
target_include_directories(rivet_prefork_mpm
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})
target_include_directories(rivet_lazy_mpm
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})
target_include_directories(mod_rivet
  PUBLIC ${RIVET_INCLUDE_DIRS_PUBLIC} PRIVATE ${RIVET_INCLUDE_DIRS_PRIVATE})

#  Set up libraries...
# ===========================================================================
target_link_libraries(rivetparser        ${TCL_STUB_LIBRARY})
target_link_libraries(rivetlib           ${TCL_STUB_LIBRARY})
target_link_libraries(mod_rivet          ${TCL_LIBRARY})

#  Handle user options...
# ===========================================================================
MESSAGE( STATUS "Rivet version ${RIVET_VERSION}:")
MESSAGE( STATUS "  Core in use: " ${RIVET_CORE})
set(CONFIGURE_CMD "cmake -${CMAKE_ARGC} ${CMAKE_ARGV0}")
if(version-display)
  set(DISPLAY_VERSION 1)
else(version-display)
  set(DISPLAY_VERSION 0)
endif()
if(head-requests)
  set(HEAD_REQUESTS 1)
endif()
set(MAX_POST ${with-post-max})
if(single-thread)
  set(MPM_SINGLE_TCL_THREAD 1)
endif()
set(NAMEOFEXECUTABLE ${TCL_TCLSH})
if(import-rivet-commands)
  message(STATUS "  forcing Rivet to import commands from ::rivet namespace")
  set(NAMESPACE_IMPORT 1)
endif()
if(rivet-commands-export)
  message(STATUS "  forcing Rivet to export commands from ::rivet namespace")
  set(NAMESPACE_EXPORT 1)
endif()
set(SEPARATE_CHANNELS 0)
if(virtual-interps-separation)
  set(SEPARATE_VIRTUAL_INTERPS 1)
endif()
if(requests-serialization)
  set(SERIALIZE_HTTP_REQUESTS 1)
endif()
if(upload-var)
  set(UPLOAD_FILES_TO_VAR 1)
endif()

#  Location of the Rivet library...
# ===========================================================================
if(NOT DEFINED RIVETLIB_DESTDIR)
  set(RIVETLIB_DESTDIR "${APACHE_LIB_DIR}rivet${RIVET_VERSION}")
endif(NOT DEFINED RIVETLIB_DESTDIR)

#  Set variables for generatting config files...
# ===========================================================================
set(RIVET_AC_APPLE_UNIVERSAL_BUILD ${AC_APPLE_UNIVERSAL_BUILD})
set(RIVET_CONFIGURE_CMD            ${CONFIGURE_CMD})
set(RIVET_DISPLAY_VERSION          ${DISPLAY_VERSION})
set(RIVET_HAVE_DLFCN_H             ${HAVE_DLFCN_H})
set(RIVET_HAVE_INTTYPES_H          ${HAVE_INTTYPES_H})
set(RIVET_HAVE_LIMITS_H            ${HAVE_LIMITS_H})
set(RIVET_HAVE_LSEEK64             ${HAVE_LSEEK64})
set(RIVET_HAVE_MEMORY_H            ${HAVE_MEMORY_H})
set(RIVET_HAVE_NET_ERRNO_H         ${HAVE_NET_ERRNO_H})
set(RIVET_HAVE_OPEN64              ${HAVE_OPEN64})
set(RIVET_HAVE_STDINT_H            ${HAVE_STDINT_H})
set(RIVET_HAVE_STDLIB_H            ${HAVE_STDLIB_H})
set(RIVET_HAVE_STRINGS_H           ${HAVE_STRINGS_H})
set(RIVET_HAVE_STRING_H            ${HAVE_STRING_H})
set(RIVET_HAVE_SYS_PARAM_H         ${HAVE_SYS_PARAM_H})
set(RIVET_HAVE_SYS_STAT_H          ${HAVE_SYS_STAT_H})
set(RIVET_HAVE_SYS_TYPES_H         ${HAVE_SYS_TYPES_H})
set(RIVET_HAVE_UNISTD_H            ${HAVE_UNISTD_H})
set(RIVET_HEAD_REQUESTS            ${HEAD_REQUESTS})
set(RIVET_MPM_SINGLE_TCL_THREAD    ${MPM_SINGLE_TCL_THREAD})
set(RIVET_NAMESPACE_EXPORT         ${NAMESPACE_EXPORT})
set(RIVET_NAMESPACE_IMPORT         ${NAMESPACE_IMPORT})
set(RIVET_HAVE_DIRENT_H            ${HAVE_DIRENT_H})
set(RIVET_NO_DIRENT_H              ${NO_DIRENT_H})
set(RIVET_NO_DLFCN_H               ${NO_DLFCN_H})
set(RIVET_NO_ERRNO_H               ${NO_ERRNO_H})
set(RIVET_NO_FLOAT_H               ${NO_FLOAT_H})
set(RIVET_HAVE_LIMITS_H            ${HAVE_LIMITS_H})
set(RIVET_NO_LIMITS_H              ${NO_LIMITS_H})
set(RIVET_NO_STDLIB_H              ${NO_STDLIB_H})
set(RIVET_NO_STRING_H              ${NO_STRING_H})
set(RIVET_NO_SYS_WAIT_H            ${NO_SYS_WAIT_H})
set(RIVET_NO_VALUES_H              ${NO_VALUES_H})
set(RIVET_NO_VIZ                   ${NO_VIZ})
set(RIVET_SEPARATE_CHANNELS        ${SEPARATE_CHANNELS})
set(RIVET_SEPARATE_VIRTUAL_INTERPS ${SEPARATE_VIRTUAL_INTERPS})
set(RIVET_SERIALIZE_HTTP_REQUESTS  ${SERIALIZE_HTTP_REQUESTS})
set(RIVET_STATIC_BUILD             ${STATIC_BUILD})
set(RIVET_STDC_HEADERS             ${STDC_HEADERS})
set(RIVET_TCL_MEM_DEBUG            ${TCL_MEM_DEBUG})
set(RIVET_TCL_THREADS              ${TCL_THREADS})
set(RIVET_TCL_WIDE_INT_IS_LONG     ${TCL_WIDE_INT_IS_LONG})
set(RIVET_TCL_WIDE_INT_TYPE        ${TCL_WIDE_INT_TYPE})
set(RIVET_UNDER_CE                 ${UNDER_CE})
set(RIVET_UPLOAD_FILES_TO_VAR      ${UPLOAD_FILES_TO_VAR})
set(RIVET_USE_THREAD_ALLOC         ${USE_THREAD_ALLOC})
set(RIVET__ISOC99_SOURCE           ${_ISOC99_SOURCE})
set(RIVET__LARGEFILE64_SOURCE      ${_LARGEFILE64_SOURCE})
set(RIVET__LARGEFILE_SOURCE64      ${_LARGEFILE_SOURCE64})
set(RIVET__OE_SOCKETS              ${_OE_SOCKETS})
set(RIVET__POSIX_PTHREAD_SEMANTICS ${_POSIX_PTHREAD_SEMANTICS})
set(RIVET__REENTRANT               ${_REENTRANT})
set(RIVET__THREAD_SAFE             ${_THREAD_SAFE})
set(RIVET__WIN32_WCE               ${_WIN32_WCE})
set(RIVET__XOPEN_SOURCE_EXTENDED   ${_XOPEN_SOURCE_EXTENDED})

#  Generate headers rivet_config.h, config.h
# ===========================================================================
configure_file("${PROJECT_SOURCE_DIR}/cmake_extra_modules/rivet_config.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/rivet_config.h" )
configure_file("${PROJECT_SOURCE_DIR}/cmake_extra_modules/config.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/config.h" )

#  Generate init.tcl
# ===========================================================================
configure_file("${RIVET_LIB_DIR}/init.tcl.in"
               "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/init.tcl" @ONLY)

# ===========================================================================
#  Installation section...
# ===========================================================================

MESSAGE ( STATUS
"==========================================================================" )
MESSAGE ( STATUS "Rivet ${RIVET_VERSION} will be installed in the following "
                 "directories:" )
MESSAGE ( STATUS " + ${CMAKE_SHARED_LIBRARY_PREFIX}mod_rivet${CMAKE_SHARED_LIBRARY_SUFFIX}: " ${APACHE_MODULE_DIR} )
MESSAGE ( STATUS "     (to override this location, use -DAPACHE_MODULE_DIR=...)")
MESSAGE ( STATUS " + Rivet library: " ${RIVETLIB_DESTDIR} )
MESSAGE ( STATUS "     (to override this location, use -DAPACHE_LIB_DIR=... or")
MESSAGE ( STATUS "                                     -DRIVETLIB_DESTDIR=...)")
MESSAGE ( STATUS
"==========================================================================" )

#  Library mod_rivet (mod_rivet.so) must be installed in the directory Apache2
#  searches for modules...
# ===========================================================================
install(TARGETS mod_rivet EXPORT Rivet_mod_rivet
  ARCHIVE  DESTINATION ${APACHE_MODULE_DIR}
  LIBRARY  DESTINATION ${APACHE_MODULE_DIR}
  RUNTIME  DESTINATION ${APACHE_MODULE_DIR})

#  Install mpms...
# ===========================================================================
install(TARGETS rivet_worker_mpm rivet_prefork_mpm rivet_lazy_mpm
  EXPORT Rivet_MPMs
  ARCHIVE  DESTINATION ${RIVETLIB_DESTDIR}/mpm
  LIBRARY  DESTINATION ${RIVETLIB_DESTDIR}/mpm
  RUNTIME  DESTINATION ${RIVETLIB_DESTDIR}/mpm)

#  Install libraries...
# ===========================================================================
install(TARGETS rivetparser rivetlib
	EXPORT Rivet_Libraries
  ARCHIVE  DESTINATION ${RIVETLIB_DESTDIR}
  LIBRARY  DESTINATION ${RIVETLIB_DESTDIR}
  RUNTIME  DESTINATION ${RIVETLIB_DESTDIR})

#  Install library files...
# ===========================================================================
install(FILES
       	"${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/init.tcl"
        ${RIVET_LIB_DIR}/default_request_handler.tcl
        ${RIVET_LIB_DIR}/pkgIndex.tcl
        DESTINATION ${RIVETLIB_DESTDIR})

#  Install library directories...
# ===========================================================================
install(DIRECTORY   ${RIVET_LIB_DIR}/rivet-tcl
        DESTINATION ${RIVETLIB_DESTDIR})
install(DIRECTORY
     	${RIVET_LIB_DIR}/packages/asciiglyphs
     	${RIVET_LIB_DIR}/packages/calendar
     	${RIVET_LIB_DIR}/packages/dio
     	${RIVET_LIB_DIR}/packages/entities
     	${RIVET_LIB_DIR}/packages/form
     	${RIVET_LIB_DIR}/packages/formbroker
     	${RIVET_LIB_DIR}/packages/session
     	${RIVET_LIB_DIR}/packages/tclrivet
        DESTINATION ${RIVETLIB_DESTDIR}/packages)
