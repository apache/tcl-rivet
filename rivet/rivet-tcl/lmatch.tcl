###
## lmatch ?-exact|-glob|-regexp? <list> <pattern>
##
##    Look for elements in <list> that match <pattern>.  This command emulates
##    the TclX lmatch command, but if TclX isn't available, it's a decent
##    substitute.
###

proc lmatch {args} {
    set modes(-exact)  0
    set modes(-glob)   1
    set modes(-regexp) 2

    if {[llength $args] == 3} {
	lassign $args mode list pattern
    } elseif {[llength $args] == 2} {
	set mode -glob
	lassign $args list pattern
    } else {
        return -code error \
	    {wrong # args: should be "lmatch ?mode? list pattern"}
    }

    if {![info exists modes($mode)]} {
	return -code error \
	    "bad search mode \"$mode\": must be -exact, -glob, or -regexp"
    }
    set mode $modes($mode)

    set return {}
    foreach elem $list {
	if {$mode == 0} {
	    if {[string compare $elem $pattern] == 0} { lappend return $elem }
	}
	if {$mode == 1} {
	    if {[string match $pattern $elem]} { lappend return $elem }
	}
	if {$mode == 2} {
	    if {[regexp $pattern $elem]} { lappend return $elem }
	}
    }
    return $return
}
