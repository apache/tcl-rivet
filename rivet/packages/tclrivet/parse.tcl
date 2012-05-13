#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh "$0" "$@"

# Parse a rivet file and execute it.

if {![info exists argv]} { return }

# more consistent manipulation of auto_path as suggested by
# Harald Oehlmann. Fixes bug #52898

set auto_path [linsert $auto_path 0 [file dirname [info script]]]

package require tclrivet

proc main {} {
    global argv
    for {set i 0} {$i < [llength $argv] - 1} {incr i} {
	source [lindex $argv $i]
    }
    set script [rivet::parserivet [lindex $argv end]]
    if { [catch {eval $script } err] } {
	puts "Error: $err"
    }
}

main
