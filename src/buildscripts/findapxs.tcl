set apxsDirList {
    /usr/local/apache/bin
    /usr/local/etc/apache/bin
    /usr/bin/
}

proc FindAPXS {{apxs ""}} {
    global apxsDirList

    if {[string length $apxs]} {
	if {[file executable $apxs]} { return $apxs }
    }

    set apxs ""
    foreach dir $apxsDirList {
	if {![file executable [file join $dir apxs]]} { continue }
	set apxs [file join $dir apxs]
	break
    }

    return $apxs
}
