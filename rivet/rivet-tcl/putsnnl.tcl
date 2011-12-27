##
## putsnnl <string> <nspaces>
##
## Shorthand for 'puts -nonewline' with the extra feature
## of being able to output a string prepended with <nspaces> 
##
## $Id: $
##

namespace eval ::rivet {
    proc putsnnl {s {spaces 0}} {

        puts -nonewline [string repeat " " $spaces]
        puts -nonewline $s

    }
}
##
