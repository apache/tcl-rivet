###
## random [seed | value ]
##
##    Generate a random number using only Tcl code.  This proc tries to
##    emulate what the TclX random function does.  If we don't have TclX
##    though, this is a decent substitute.
###

proc random {args} {
    global _ran

    if {[llength $args] > 1} {
	set _ran [lindex $args 1]
    } else {
	set period 233280
	if {[info exists _ran]} {
	    set _ran [expr { ($_ran*9301 + 49297) % $period }]
	} else {
	    set _ran [expr { [clock seconds] % $period } ]
	}
	return [expr { int($args*($_ran/double($period))) } ]
    }
}
