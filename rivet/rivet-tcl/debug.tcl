###
## debug ?-option value? ?-option value?...
##    A command to make debugging more convenient.  Print strings, arrays
##    and the values of variables as specified by the arguments.
##
##    Also allows the setting of an array called debug which will pick up
##    options for all debug commands.
##
##    We create this command in the ::request namespace because we want the
##    user to be able to use the debug array without actually having to set
##    it at the global level.
##
##    Options:
##	-subst <on|off> - Each word should be considered a variable and subst'd.
##	-separator <string> - A text string that goes between each variable.
##	-ip <ip address> - A list of IP addresses to display to.	
###

proc debug {args} {
    ## If they've turned off debugging, we don't do anything.
    if {[info exists ::RivetUserConf(Debug)] && !$::RivetUserConf(Debug)} {
	return
    }

    ## We want to save the REMOTE_ADDR for any subsequent calls to debug.
    if {![info exists ::RivetUserConf(REMOTE_ADDR)]} {
	load_env
	set ::RivetUserConf(REMOTE_ADDR) $env(REMOTE_ADDR)
    }


    ## Set some defaults for the options.
    set data(subst) 0
    set data(separator) <br>

    ## Check RivetUserConf for globally set options.
    if {[info exists ::RivetUserConf(DebugIp)]} {
	set data(ip) $::RivetUserConf(DebugIp)
    }
    if {[info exists ::RivetUserConf(DebugSubst)]} {
	set data(subst) $::RivetUserConf(DebugSubst)
    }
    if {[info exists ::RivetUserConf(DebugSeparator)]} {
	set data(separator) $::RivetUserConf(DebugSeparator)
    }

    set looking 0
    set endit 0
    foreach arg $args {
	if $endit {
	    lappend list $arg
	    continue
	}
	if $looking {
	    set data($varName) $arg
	    set looking 0
	    continue
	}
	if {[string index $arg 0] == "-"} {
	    if {$arg == "--"} {
		set endit 1
		continue
	    }
	    set varName [string range $arg 1 end]
	    set looking 1
	    continue
	}
	lappend list $arg
    }

    if {[info exists data(ip)]} {
	set can_see 0
	foreach ip $data(ip) {
	    if {[string match $data(ip)* $::RivetUserConf(REMOTE_ADDR)]} {
		set can_see 1
		break
	    }
	}
	if {!$can_see} { return }
    }

    if {[string tolower $data(subst)] != "on"} {
	html [join $list]
	return
    }

    set lastWasArray 0
    foreach varName $list {
	upvar $varName var
	if {[array exists var]} {
	    parray $varName
	    set lastWasArray 1
	} elseif {[info exists var]} {
	    if {!$lastWasArray} {
		html $data(separator)
	    }
	    html $var
	    set lastWasArray 0
	} else {
	    if {!$lastWasArray} {
		html $data(separator)
	    }
	    html $varName
	    set lastWasArray 0
	}
    }
}
