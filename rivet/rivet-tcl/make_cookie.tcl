##
## Convert an integer-seconds-since-1970 click value to RFC850 format,
## with the additional requirement that it be GMT only.
##
proc clock_to_rfc850_gmt {seconds} {
    return [clock format $seconds -format "%a, %d-%b-%y %T GMT" -gmt 1]
}

proc make_cookie_attributes {paramsArray} {
    upvar $paramsArray params

    set cookieParams ""
    set expiresIn 0

    foreach {time num} [list days 86400 hours 3600 minutes 60] {
        if [info exists params($time)] {
	    incr expiresIn [expr $params($time) * $num]
	}
    }
    if {$expiresIn != 0} {
	set secs [expr [clock seconds] + $expiresIn]
	append cookieParams "; expires=[clock_to_rfc850_gmt $secs]"
    }
    if [info exists params(path)] {
        append cookieParams "; path=$params(path)"
    }
    if [info exists params(domain)] {
        append cookieParams "; domain=$params(domain)"
    }
    if {[info exists params(secure)] && $params(secure) == 1} {
        append cookieParams "; secure"
    }

    return $cookieParams
}

# make_cookie cookieName cookieValue [-days expireInDays]
#    [-hours expireInHours] [-minutes expireInMinutes]
#    [-path uriPathCookieAppliesTo]
#    [-secure 1|0]
#
proc make_cookie {name value args} {
    import_keyvalue_pairs params $args

    set badchars "\[ \t;\]"
    if {[regexp $badchars $name]} {
        return -code error "name may not contain semicolons, spaces, or tabs"
    }
    if {[regexp $badchars $value]} {
        return -code error "value may not contain semicolons, spaces, or tabs"
    }

    set cookieKey "Set-cookie"
    set cookieValue "$name=$value"

    append cookieValue [make_cookie_attributes params]

    set_header $cookieKey $cookieValue
}
