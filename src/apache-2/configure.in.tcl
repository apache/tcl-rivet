# This file specifies the actual things to test for.

# $Id: configure.in.tcl,v 1.7 2003/12/13 14:58:50 davidw Exp $

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

# Merge apxs and Tcl CFLAGS.
configure::test CFLAGS {
    set apachecflags [exec $APXS -q CFLAGS]
    if { $DEBUGSYMBOLS } {
	set tclcflags [concat $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING \
			   $TCL_EXTRA_CFLAGS]
    } else {
	set tclcflags [concat $TCL_CFLAGS_DEBUG $TCL_CFLAGS_OPTIMIZE \
			   $TCL_CFLAGS_WARNING $TCL_EXTRA_CFLAGS]
    }
    set res $apachecflags
    foreach f $tclcflags {
	if { [lsearch $apachecflags $f] == -1 } {
	    lappend res $f
	}
    }
    set res
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
    set tmp "$TCL_CC $CFLAGS $TCL_SHLIB_CFLAGS $INC $TCL_EXTRA_CFLAGS $TCL_THREADED -c"
}

configure::test CRYPT_LIB {
    configure::lib_has_function crypt crypt
}
