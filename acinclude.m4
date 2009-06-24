#
# Rivet build stuff
#
# $Id$
#


#
# Include the TEA standard macro set
#
#builtin([include],[tclconfig/tcl.m4])
m4_include([tclconfig/tcl.m4])

#
# Include the libtool macro set
#
m4_include([tclconfig/libtool.m4])

#
# Add here whatever m4 macros you want to define for your package
#
m4_include([m4/ax_prefix_config_h.m4])

# 
# check for apache base directory
#
AC_DEFUN([APACHE_DIR],[

  AC_ARG_WITH(
    apache,
    [  --with-apache[=DIR]     Apache server directory],
    ,
    [with_apache="no"]
  )

  AC_MSG_CHECKING(for Apache directory)

  if test "$with_apache" = "no"; then
    AC_MSG_ERROR( Specify the apache using --with-apache)
  else
    # make sure that a well known include file exists
    if test -e $with_apache/include/httpd.h; then
      apache_dir=$with_apache
      AC_MSG_RESULT(APACHE found!)
    else
      AC_MSG_ERROR( $with_apache not found. )
    fi
  fi

])

