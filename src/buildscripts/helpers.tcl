proc lremove {list string} {
    set s [lsearch $list $string]
    if {$s >= 0} {
	set list [lreplace $list $s $s]
    }
    return $list
}
