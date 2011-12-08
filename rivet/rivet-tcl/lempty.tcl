###
## lempty <list>
##
##    Returns 1 if <list> is empty or 0 if it has any elements.  This command
##    emulates the TclX lempty command.
##
## $Id$
##
###

namespace eval ::rivet {

    proc lempty {list} {
        if {[catch {llength $list} len]} { return 0 }
        return [expr $len == 0]
    }

}
