#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

source makeconf.tcl

set binname [ getbinname ]

makeconf $binname server.conf

# We could include some sort of loop here, running the server + tests

set apachepid [ exec $binname -X -f "[file join [pwd] server.conf]" & ]
set oput [ exec [file join . rivet.test] ]
puts $oput
exec kill $apachepid
