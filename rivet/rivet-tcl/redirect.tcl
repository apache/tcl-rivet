#
# -- ::rivet::redirect
#
#  Redirecting to a new URL by issuing a 301 or 302 (permanent)
# diversion to a new resource. 
#
# Arguments:
#
#   - url               - URL to which we are redirecting the client
#   - permanent:[0 | 1] - whether redirection will be permanent (default: 0)
#     or
#   - permanent: code   - returns any HTTP integer code. In this context
#                         only the 3xx status codes are meaningful
#
#   $ Id: $
#

namespace eval ::rivet {

    proc redirect {url {permanent 0}} {

        if {[::rivet::headers sent]} {

            return  -code error \
                    -errorcode headers_already_sent \
                    -errorinfo "Impossible to redirect: headers already sent"

        }

        # In order to preserve compatibility 
        # with the past we chec whether we are
        # dealing with a boolean argument and handle
        # it accordingly 

        if {[string is boolean $permanent] } {

            if {[string is true $permanent]} {
                set http_code 301
            } else {
                set http_code 302
            } 

        } elseif {[string is integer $permanent] && ($permanent > 0)} {

            set http_code $permanent

        } else {

            return  -code error \
                    -errorcode invalid_http_code \
                    -errorinfo "Invalid HTTP status code: $permanent"

        }

        ::rivet::no_body ; ## don’t output anything on a redirect
        ::rivet::headers set Location $url
        ::rivet::headers numeric $http_code
        ::rivet::abort_page [dict create error_code redirect location $url] ; ## stop any further processing

        return -error ok 
    }

}
