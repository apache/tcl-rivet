###
## read_file <file>
##    Read the entire contents of a file and return it as a string.
##
##    file - Name of the file to read.
##
## $Id$
##
###

namespace eval ::rivet {

    proc read_file {file args} {

        set fp [open $file]
        if {[llength $args]} { eval fconfigure $fp $args }
        set x [read $fp]
        close $fp
        return $x

    }

}
