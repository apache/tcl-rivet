##
## Detect switches from a string (e.g. "-all -regexp -- foo bar args")
## and extract them into an array.
##
proc import_switch_args {arrayName argsList {switchList ""}} {
    upvar 1 $arrayName array
    set index 0
    set array(args) ""
    set array(switches) ""
    if {[llength $switchList] > 0} {
	set proofSwitches 1
    } else {
	set proofSwitches 0
    }
    foreach arg $argsList {
	if {[string index $args 0] != "-"} {
            set array(args) [lrange $argsList $index end]
            break
        } elseif {$arg == "--"} {
	    set array(args) [lrange $argsList [expr $index + 1] end]
	    break
	}
        set switch [string range $arg 1 end]
	if {!$proofSwitches || [lsearch -exact $switchList $switch] >= 0} {
            set array($switch) $index
	    lappend array(switches) $switch
	}
        incr index
    }
}
