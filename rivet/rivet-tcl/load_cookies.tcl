proc load_cookies {{arrayName cookies}} {
    upvar 1 $arrayName cookies

    set HTTP_COOKIES [env HTTP_COOKIES]

    foreach pair [split $HTTP_COOKIE ";"] {
	set pair [split [string trim $pair] "="]
	set key [lindex $pair 0]
	set value [lindex $pair 1]
	set cookies($key) [list $value]
    }
}
