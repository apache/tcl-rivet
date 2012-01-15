##
## putsnnl ?-sgml? <string> <width>
##
## Shorthand for 'puts -nonewline' with the extra feature
## of being able to print a string padded with spaces.
## The ouput has a fixed width <width> and string in <string>
## is left (width > 0) or right (width < 0) aligned.
##
## When the switch -sgml is passed in as first argument
## the padding is done with &nbsp; SGML entities.
##
## $Id$
##

namespace eval ::rivet {
    proc putsnnl {args } {

        set nargs [llength $args]

        if {$nargs == 1} {
            set width undefined
            set output_string [lindex $args 0]
        } elseif {$nargs == 2} {
            if {[string match "-sgml" [lindex $args 0]]} {
                set output_string [lindex $args 1]
                set padding "&nbsp;"
                set width undefined
            } else {
                set output_string [lindex $args 0]
                set width [lindex $args 1]
                set padding " "
            }
        } elseif {$nargs == 3} {
            if {![string match "-sgml" [lindex $args 0]]} {
                return -code error -error_code wrong_param \
                                   -error_info "Expected -sgml switch" \
                                               "Expected -sgml switch"
            }

            set output_string [lindex $args 1]
            set width [lindex $args 2]
	    set padding "&nbsp;"
        } else {
            return -code error -error_code wrong_num_param \
                               -error_info "Expected at most 3 args, got $nargs ($args)" \
                                           "Expected at most 3 args, got $nargs ($args)"
        }

        if {[string is integer $width]} {
            if {$width == 0} {
                set final_string ""
            } else {

                set string_l [string length $output_string]

                if { $string_l > abs($width)} {
                    set final_string [string range $output_string 0 [expr abs($width)-1]]
                } else {

                    set padding [string repeat $padding [expr abs($width) - $string_l]]
                    if {$width > 0} {
                        set final_string [format "%s%s" $padding $output_string]
                    } else {
                        set final_string [format "%s%s" $output_string $padding]
                    }
                }
            }
        } else {
            set final_string $output_string
        }

        puts -nonewline $final_string
    }
}
##
