###
## incr0 ?varName? ?num?
##    Increment a variable by <num>.  If the variable doesn't exist, create
##    it instead of returning an error.
##
##    varName - Name of the variable to increment.
##    num     - Number to increment by.
###

proc incr0 {varName {num 1}} {
    upvar 1 $varName var
    if {![info exists var]} { set var 0 }
    return [incr var $num]
}
