proc load_cookies {{arrayName cookies}} {
    upvar 1 $arrayName cookies

    load_env
    if {![info exists env(HTTP_COOKIE)]} { return }

    foreach pair [split $env(HTTP_COOKIE) ";"] {
	set pair [split [string trim $pair] "="]
	set key [lindex $pair 0]
	set value [lindex $pair 1]
	set cookies($key) [list $value]
    }
}
