# -- catch.tcl
#
# Wrapper of the core [catch] command that checks whether 
# an error condition is actually raised by [::rivet::abort_page]
# or [::rivet::exit]. In case the error is thrown again to allow
# the interpreter to interrupt and pass execution to AbortScript
#
# $Id$
#

namespace eval ::rivet {

    proc catch {script args} {

        set catch_ret [uplevel [list ::catch $script {*}$args]]

        if {$catch_ret && [::rivet::abort_page -aborting]} {

            return -code error -errorcode {RIVET ABORTPAGE} "Page abort"

        } elseif {$catch_ret && [::rivet::abort_page -exiting]} {

            return -code error -errorcode {RIVET THREAD_EXIT} "Thread exit"

        } else {

            return $catch_ret

        }

    }
}
