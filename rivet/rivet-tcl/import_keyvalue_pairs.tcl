proc import_keyvalue_pairs {arrayName argsList} {
    upvar 1 $arrayName data

    if {[string index $argsList 0] != "-"} {
	set data(args) $argsList
	return
    }

    set endit 0
    set looking 0
    set data(args) ""
    foreach arg $argsList {
	if {$endit} {
	    lappend data(args) $arg
	} elseif {$looking} {
	    set data($varName) $arg
	    set looking 0
	} elseif {[string index $arg 0] == "-"} {
	    if {$arg == "--"} {
		set endit 1
		continue
	    }
	    if {$arg == "-args"} {
		return -code error "-args is a reserved value."
	    }
	    set varName [string range $arg 1 end]
	    set looking 1
	} else {
	    lappend data(args) $arg
	}
    }
}
