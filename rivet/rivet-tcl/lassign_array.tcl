#
# -- lassign_array 
# 
# given a list, an array name, and a variable number
# of arguments consisting of variable names, assign each element in
# the list, in turn, to elements corresponding to the variable
# arguments, into the named array.  From DIO (originally from TclX).
#
# $Id$
#

namespace eval ::rivet {

    proc lassign_array {list arrayName args} {
        upvar 1 $arrayName array
        foreach elem $list field $args {
            set array($field) $elem
        }
    }

}
