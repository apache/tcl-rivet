###
##
## load_response ?arrayName?
##    Load any form variables passed to this page into an array.
##
##    arrayName - Name of the array to set.  Default is 'response'.
##
## $Id$
##
###

namespace eval ::rivet {

    proc load_response {{arrayName response}} {
        upvar 1 $arrayName response

        foreach {var elem} [var all] {
            if {[info exists response(__$var)]} {
                # we have seen var multiple times already, add to the list
                lappend response($var) $elem
            } elseif {[info exists response($var)]} {
                # second occurence of var,  convert response(var) list:
                set response($var) [list $response($var) $elem]
                set response(__$var) ""
            } else {
                # first time seeing this var
                set response($var) $elem
            }
        }
    }

}
