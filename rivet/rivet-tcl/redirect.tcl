#
# -- ::rivet::redirect
#
#  Redirecting to a new URL by issuing a 301 or 302 (permanent)
# diversion to a new resource. 
#
# Arguments:
#
#   - url               - URL to which we are redirecting the client
#   - permanent:[0 | 1] - whether redirection will be permanent (default)
#
#

namespace eval ::rivet {

    proc ::rivet::redirect {url {permanent 0}} {

        if {[::rivet::headers sent]} {

            return  -code error \
                    -errorcode headers_already_sent \
                    -errorinfo "Impossible to redirect: headers already sent"
        }

        ::rivet::no_body ; ## don’t output anything on a redirect
        ::rivet::headers set Location $url
        ::rivet::headers numeric [expr {$permanent ? "301" : "302"}]
        ::rivet::abort_page [dict create error_code redirect location $url] ; ## stop any further processing

        return -error ok 
    }

}
