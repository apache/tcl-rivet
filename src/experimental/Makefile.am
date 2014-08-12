#
# mod_rivet
#

# The weird and wild magic of auto* associates these two.

apxs_libexecdir = @APXS_LIBEXECDIR@
apxs_libexec_LTLIBRARIES = mod_rivet.la

mod_rivet_la_SOURCES =  mod_rivet.c                                         \
                        ../@apache_request@/apache_multipart_buffer.c 	    \
                        ../@apache_request@/apache_request.c                \
                        ../@rivet_commands@/rivetCore.c                    \
                        ../@rivet_commands@/rivetInspect.c                 \
                        ../@rivet_channel@/rivetChannel.c                   \
                        ../common/TclWebapache.c                            \
                        ../common/apache_config.c                           \
                        ../common/mod_rivet_common.c                        \
                        ../parser/rivetParser.c

mod_rivet_la_LDFLAGS = @TCL_LIB_SPEC@ @APXS_LDFLAGS@ @APR_LDFLAGS@ -module -avoid-version
mod_rivet_la_LIBADD = @TCL_LIBS@ @APXS_LIBS@ 
mod_rivet_la_CPPFLAGS = -I../@rivet_core@ -I../@rivet_channel@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@/parser -I../@apache_request@ @apache_include@ -I@RIVET_BASE_INCLUDE@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -DSTART_TAG='"<?"' -DEND_TAG='"?>"'

lib_libexecdir = @RIVET_TCL_TARGET@/mpm
lib_libexec_LTLIBRARIES = rivet_worker_mpm.la rivet_prefork_mpm.la

rivet_worker_mpm_la_SOURCES = rivet_worker_mpm.c
rivet_worker_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_worker_mpm_la_LIBADD  = @APXS_LIBS@
rivet_worker_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I../@apache_request@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@

rivet_prefork_mpm_la_SOURCES = rivet_prefork_mpm.c
rivet_prefork_mpm_la_LDFLAGS = @TCL_STUB_LIB_SPEC@ @APXS_LDFLAGS@ -module -avoid-version
rivet_prefork_mpm_la_LIBADD  = @APXS_LIBS@
rivet_prefork_mpm_la_CPPFLAGS = @apache_include@ -I@rivet_core@ @TCL_INCLUDES@ @APXS_CPPFLAGS@ @APXS_INCLUDES@ @APR_CPPFLAGS@ @APR_INCLUDES@ -I../@apache_request@ -I@RIVET_BASE_INCLUDE@/common -I@RIVET_BASE_INCLUDE@


