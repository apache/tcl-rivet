###
## lassign <list> ?varName varName ...?
##
##    Assign a <list> of values to a list of variables.  This command emulates
##    the TclX lassign command.
###

proc lassign {list args} {
    foreach elem $list varName $args {
	upvar 1 $varName var
	set var $elem
    }
}
