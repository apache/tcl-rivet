proc lassign {list args} {
    foreach elem $list varName $args {
	upvar 1 $varName var
	set var $elem
    }
}
