##
## cookie [set|get] and supporting functions
##
## $Id$
##


namespace eval ::rivet {

## clock_to_rfc850_gmt seconds -- Convert an integer-seconds-since-1970 
## click value to RFC850 format, with the additional requirement that it 
## be GMT only.
##
    proc clock_to_rfc850_gmt {seconds} {
        return [clock format $seconds -format "%a, %d-%b-%y %T GMT" -gmt 1]
    }

## make_cookie_attributes paramsArray -- Build up cookie parameters.
##
## If an expires element appears in the array, it's appended to the
## result as "; expires=value"
##
## If an element is present named "days", "hours", or "minutes", the
## corresponding value is converted to seconds, added to the current
## time, converted to RFC850 time format, and appended to the result
## as, for example, "; expires=Sun, 25-Sep-05 23:09:52 GMT"
##
## If any other elements named "path", "domain", or "secure" are present,
## they too are appended to the result.
##
## The resut is returned.
##

    proc make_cookie_attributes {paramsArray} {
        upvar 1 $paramsArray params

        set cookieParams ""
        set expiresIn 0

        if { [info exists params(expires)] } {
            append cookieParams "; expires=$params(expires)"
        } else {
            foreach {time num} [list days 86400 hours 3600 minutes 60] {
                if [info exists params($time)] {
                    incr expiresIn [expr $params($time) * $num]
                }
            }
            if {$expiresIn != 0} {
                set secs [expr [clock seconds] + $expiresIn]
                append cookieParams "; expires=[clock_to_rfc850_gmt $secs]"
            }
        }
        if { [info exists params(path)] } {
            append cookieParams "; path=$params(path)"
        }
        if { [info exists params(domain)] } {
            append cookieParams "; domain=$params(domain)"
        }
        if { [info exists params(secure)] && $params(secure) == 1} {
            append cookieParams "; secure"
        }

        return $cookieParams
    }

## cookie [set|get] cookieName ?cookieValue? [-days expireInDays]
##    [-hours expireInHours] [-minutes expireInMinutes]
##    [-expires  Wdy, DD-Mon-YYYY HH:MM:SS GMT]
##    [-path uriPathCookieAppliesTo]
##    [-secure 1|0]
##

    proc cookie {cmd name args} {
        set badchars "\[ \t;\]"

        switch -- $cmd {
        "set" {
            set value [lindex $args 0]
            set args  [lrange $args 1 end]
            import_keyvalue_pairs params $args

            if {[regexp $badchars $name]} {
            return -code error \
                "name may not contain semicolons, spaces, or tabs"
            }
            if {[regexp $badchars $value]} {
            return -code error \
                "value may not contain semicolons, spaces, or tabs"
            }

            set cookieKey "Set-Cookie"
            set cookieValue "$name=$value"

            append cookieValue [make_cookie_attributes params]

            headers add $cookieKey $cookieValue
        }

        "get" {
            ::request::global RivetCookies

            if {![array exists RivetCookies]} { load_cookies RivetCookies }
            if {![info exists RivetCookies($name)]} { return }
            return $RivetCookies($name)
        }

        "delete" {
            ## In order to delete a cookie, we just need to set a cookie
            ## with a time that has already expired.
            cookie set $name "" -minutes -1
        }
        "unset" {
            ::request::global RivetCookies
            if {![array exists RivetCookies]} { load_cookies RivetCookies }
            if {![info exists RivetCookies($name)]} { return }
            unset RivetCookies($name)
        }
        }
    }

}
