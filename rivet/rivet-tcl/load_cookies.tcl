proc load_cookies {{arrayName cookies}} {
    upvar 1 $arrayName cookies

    set HTTP_COOKIE [env HTTP_COOKIE]

    foreach pair [split $HTTP_COOKIE ";"] {
	set pair [split [string trim $pair] "="]
	set key [lindex $pair 0]
	set value [lindex $pair 1]
	set cookies($key) [list $value]
    }
}
