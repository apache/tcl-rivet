#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh "$0" "$@"

source [file join [file dirname [info script]] findconfig.tcl]

set config [FindTclConfig]

proc parseconfig { config } {
    set fl [open $config r]
    while { ! [eof $fl] } {
	gets $fl line
	if { [string index $line 0] != "#" } {
	    set line [ split $line = ]
	    if { [llength $line] == 2 } {
		set val ""
		set var [lindex $line 0]
		global $var
		catch {
		    set val [subst [string trim [lindex $line 1] ']]
		}
		set $var $val
	    }
	}
    }
}

parseconfig $config