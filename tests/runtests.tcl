#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

source makeconf.tcl

set binname [ getbinname ]

makeconf $binname server.conf

set apachepid [ exec $binname -X -f "[pwd]/server.conf" & ]
set oput [ exec ./rivet.test ]
puts $oput
exec kill $apachepid
