#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh "$0" "$@"

# Parse a rivet file and execute it.

if {![info exists argv]} { return }

set auto_path "[file dirname [info script]] $auto_path"
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
