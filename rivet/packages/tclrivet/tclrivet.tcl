# Tcl versions of Rivet commands.

# $Id$

package provide tclrivet 0.1

load [file join [file dirname [info script]] .. .. .. src \
	  librivetparser[info sharedlibextension]]

proc include { filename } {
    set fl [ open $filename ]
    fconfigure $fl -translation binary
    puts -nonewline [ read $fl ]
    close $fl
}

# We need to fill these in, of course.

proc makeurl {} {}
proc headers {} {}
proc load_env {} {}
proc load_headers {} {}
proc var {} {}
proc var_qs {} {}
proc var_post {} {}
proc upload {} {}
proc include {} {}
proc parse {} {}
proc no_body {} {}
proc env {} {}
proc abort_page {} {}
proc virtual_filename {} {}
