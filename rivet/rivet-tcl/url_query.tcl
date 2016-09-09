# -- url_query
#
# Build a URL query starting from a list of parameter-value
# pairs. If the list size is odd the last element in the list
# is discarded. Values in the list are escaped using 
# ::rivet::escape_string


namespace eval ::rivet {

    proc url_query {args} {

        set args_list $args

        set urlarg ""
        while {[llength $args_list] > 1} {
            set args_list [lassign $args_list par val]
            lappend urlarg "$par=[::rivet::escape_string $val]"
        }
        return [join $urlarg "&"]

    }

}
