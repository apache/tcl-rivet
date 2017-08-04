#
#  APACHE_FOUND - System has APACHE
#  APACHE_INCLUDE_DIR - The APACHE include directory
#
#  APACHE_LOCATION
#   setting this enables search for apache libraries / headers in this location

#
# Include directories
#
find_path(APACHE_INCLUDE_DIR
          NAMES httpd.h 
          PATH_SUFFIXES httpd apache apache2
          HINTS ${APACHE_INCLUDE_DIR_HINTS} ${APACHE_ROOT}/include)

# Tryto find apxs, in order to get more information...
find_program(APACHE_APXS_BIN NAMES apxs apxs2 apxs.exe apxs2.exe
             PATH_SUFFIXES httpd apache apache2
             HINTS ${APACHE_ROOT}/bin)

if(NOT DEFINED APACHE_MODULE_DIR)
  if(APACHE_APXS_BIN)
    EXEC_PROGRAM(${APACHE_APXS_BIN} ARGS -q LIBEXECDIR
                 OUTPUT_VARIABLE APACHE_MODULE_DIR)
  else(APACHE_APXS_BIN)
    find_path(APACHE_MODULE_DIR
              NAMES mod_alias.so mod_auth_basic.so
              HINTS ${APACHE_MODULE_DIR_HINTS} ${APACHE_ROOT}/modules)
  endif(APACHE_APXS_BIN)
endif(NOT DEFINED APACHE_MODULE_DIR)

if(NOT DEFINED APACHE_LIB_DIR)
    message(STATUS "not found")
  if(APACHE_APXS_BIN)
     EXEC_PROGRAM(${APACHE_APXS_BIN} ARGS -q LIBDIR
                  OUTPUT_VARIABLE APACHE_LIB_DIR )
  else(APACHE_APXS_BIN)
    ## Use the bin dir, inside Apache server...
    find_path(APACHE_LIB_DIR
              NAMES httpd httpd.exe
              HINTS ${APACHE_LIB_DIR_HINTS} ${APACHE_ROOT}/bin)
  endif(APACHE_APXS_BIN)
endif(NOT DEFINED APACHE_LIB_DIR)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set APACHE_FOUND to TRUE if 
# all listed variables are TRUE
find_package_handle_standard_args(APACHE DEFAULT_MSG APACHE_INCLUDE_DIR )
mark_as_advanced(APACHE_INCLUDE_DIR)
mark_as_advanced(APACHE_MODULE_DIR)
mark_as_advanced(APACHE_LIB_DIR)
