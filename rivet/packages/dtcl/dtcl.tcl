###
## This package is meant as a compatibility layer between Rivet and mod_dtcl.
##
## All of the mod_dtcl commands call their Rivet equivalents and return the
## proper responses.
###

package provide Dtcl 1.0

proc hgetvars {} {
    uplevel {
	load_env ENVS
	load_cookies COOKIES
    }
}

proc hputs {args} {
    set nargs [llength $args]
    if {$nargs < 1 || $nargs > 2} {
	return -code error {wrong # args: should be "hputs ?-error? text"}
    }

    if {$nargs == 2} {
	set string [lindex $args 1]
    } else {
	set string [lindex $args 0]
    }

    puts $string
}

proc hflush {} {
    flush stdout
}

proc dtcl_info {} { }
