# $Id$
# This program ought to return the location of tclConfig.sh
# Code borrowed from Don Porter's usenet posting.

proc RelToExec {exe cf} {
    file join [file dirname [file dirname $exe]] lib $cf
}

proc RelToExec2 {cf} {
    file join [ info library ] $cf
}

proc FindTclConfig {} {
    set exec [file tail [info nameofexecutable]]
    # If we're running tclsh...
    if {[string match -nocase "*tclsh*" $exec]} {
	set cf [RelToExec [info nameofexecutable] tclConfig.sh]
	if {[file readable $cf]} {
	    return $cf
	} else {
	    set cf [RelToExec2 tclConfig.sh]
	    if {[file readable $cf]} {
		return $cf
	    }
	}
    }
    # If tcl_pkgPath is available, look there...
    global tcl_pkgPath
    if {[info exists tcl_pkgPath]} {
	foreach libdir $tcl_pkgPath {
	    if {[file readable [file join $libdir tclConfig.sh]]} {
		return [file join $libdir tclConfig.sh]
	    }
	}
    } 
    # Not in usual places, go searching for tclsh...
    set candshells [list]
    if {[regsub -nocase wish $exec tclsh shell]} {
	lappend candshells $shell
    }
    lappend candshells tclsh[package provide Tcl]
    lappend candshells tclsh[join [split [package provide Tcl] .] ""] foreach shell $candshells {
	set shell [auto_execok $shell]
	if {[string length $shell]} {
	    set cf [RelToExec $shell tclConfig.sh]
	    if {[file readable $cf]} {
		return $cf
	    }
	}
    }
    return -code error "tclConfig.sh not found"
}
