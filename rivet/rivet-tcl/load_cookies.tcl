#
# load_cookies - Read the cookies out of the web environment HTTP_COOKIE
#                variable, parse it, and store them as key-value pairs
#                into the named array.
#
# Implementing this command for tclwire's 'rivet' environment brough
# up an inconsistency with the standards: The separator '=' falls in
# the position of the character first occurrence. The character '='
# is otherwise a valid character for a cookie value
#
#

namespace eval ::rivet {

    proc load_cookies {{arrayName cookies}} {
        upvar 1 $arrayName cookies

        set HTTP_COOKIE [env HTTP_COOKIE]

        foreach pair [split $HTTP_COOKIE ";"] {
            set pair [split [string trim $pair] "="]
            if {$pair eq {}} {
                continue
            }

            set separator [string first = $pair]
            if {$separator < 1} {
                continue
            }

            set name [string trim [string range $pair 0 $separator-1]]
            set value [string trim [string range $pair $separator+1 end]]
            set cookies($name) [list $value]
        }
    }

}
