# This file specifies the actual things to test for.

# $Id$

# We need Tcl 8.4 or later.
package require Tcl 8.4

# Add some command-line configuration options specific to Rivet.
configure::AddOption -flag with-apxs -var APXS \
    -desc "Location of the apxs binary" -arg
configure::AddOption -flag with-tclconfig -var TCL_CONFIG \
    -desc "Location of tclConfig.sh" -arg

configure::ProcessOptions

configure::test TCL_CONFIG {
    configure::findtclconfig
}

configure::parsetclconfig $::configs::TCL_CONFIG

configure::test APXS {
    source [file join [file dirname [info script]] buildscripts findapxs.tcl]
    findapxs::FindAPXS
}

configure::test TCL_THREADED {
    set tmp "-DTCL_THREADED=[info exists tcl_platform(threaded)]"
}

configure::test INCLUDEDIR {
    exec $APXS -q INCLUDEDIR
}

configure::test LIBEXECDIR {
    exec $APXS -q LIBEXECDIR
}

configure::test PREFIX {
    set TCL_PACKAGE_PATH
}

configure::test INC {
    set tmp "-I$INCLUDEDIR $TCL_INCLUDE_SPEC"
}

configure::test COMPILE {
    if { $DEBUGSYMBOLS } {
	set tmp "$TCL_CC $TCL_CFLAGS_DEBUG $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING $TCL_SHLIB_CFLAGS $INC $TCL_EXTRA_CFLAGS $TCL_THREADED -c"
    } else {
	set tmp "$TCL_CC $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING $TCL_SHLIB_CFLAGS $INC $TCL_EXTRA_CFLAGS $TCL_THREADED -c"
    }
}

configure::test CRYPT_LIB {
    configure::lib_has_function crypt crypt
}
