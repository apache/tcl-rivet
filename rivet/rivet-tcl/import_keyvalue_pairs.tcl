proc import_keyvalue_pairs {arrayName argsList} {
    upvar 1 $arrayName data

    if {[string index $argsList 0] != "-"} {
	set data(args) $argsList
	return
    }

    set index 0
    set looking 0
    set data(args) ""
    foreach arg $argsList {
	if {$looking} {
	    set data($varName) $arg
	    set looking 0
	} elseif {[string index $arg 0] == "-"} {
	    if {$arg == "--"} {
		set data(args) [lrange $argsList [expr $index + 1] end]
		break
	    }
	    if {$arg == "-args"} {
		return -code error "-args is a reserved value."
	    }
	    set varName [string range $arg 1 end]
	    set looking 1
	} else {
	    set data(args) [lrange $argsList $index end]
	    break
	}
	incr index
    }
}
