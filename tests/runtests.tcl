#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

proc getbinname { } {
    global argv
    set binname [lindex $argv 0]
    if { $binname == "" || ! [file exists $binname] } {
	puts stderr "Please supply the full name and path of the Apache executable on the command line!"
	exit 1
    }
    return $binname
}

set binname [ getbinname ]

source makeconf.tcl
makeconf $binname server.conf

# we do this to keep tcltest happy - it reads argv...
set commandline [lindex $argv 1]
set argv {}

switch -exact $commandline {
    startserver {
	if { [catch {
	    exec $binname -X -f "[file join [pwd] server.conf]"
	} err] } {
	    puts "$errorInfo"
	}
    }
    default {
	source [file join . rivet.test]
    }
}
