###
## load_response ?arrayName?
##    Load any form variables passed to this page into an array.
##
##    arrayName - Name of the array to set.  Default is 'response'.
###

proc load_response {{arrayName response}} {
    upvar 1 $arrayName response

    foreach {var elem} [var all] {
	set response($var) $elem
    }
}
