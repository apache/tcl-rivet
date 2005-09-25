###
## import_keyvalue_pairs -- Import an argument list into the named array.
##
## key-value pairs, like "-foo bar" are stored in the array.  In that
## case, the value "bar" would be stored in the element "foo"
##
## If "--" appears or a key doesn't begin with "-", the rest of the arg 
## list is stored in the special args element of the array.
##
## $Id$
##
###
proc import_keyvalue_pairs {arrayName argsList} {
    upvar 1 $arrayName data

    # if the first character of the arg list isn't a dash, put the whole
    # body in the args element of the array, and we're done

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
		# "--" appears as an argument, store the reset of the arg list
		# in the args element of the array
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
