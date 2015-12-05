# -- try.tcl
#
# Wrapper of the core [try] command 
#
# $Id: $
#

namespace eval ::rivet {

    proc try {script args} {

        uplevel [list ::try $script trap {RIVET ABORTPAGE} {} {
                return -errorcode ABORTPAGE -code error
            } trap {RIVET THREAD_EXIT} {} {
                return -errorcode EXITPAGE -code error
            } {*}$args]

    }
}

