###
##
## wrap - Split a string on newlines.  For each line, wrap the line at a space
## character to be equal to or shorter than the maximum length value passed.
##
## if a third argument called "-html" is present, the string is put together
## with html <br> line breaks, otherwise it's broken with newlines.
##
## $Id$
##
###

namespace eval ::rivet {

    proc wrap {string maxlen {html ""}} {
        set splitstring {}
        foreach line [split $string "\n"] {
            lappend splitstring [wrapline $line $maxlen $html]
        }
        if {$html == "-html"} {
            return [join $splitstring "<br>"]
        } else {
            return [join $splitstring "\n"]
        }
    }

##
## wrapline -- Given a line and a maximum length and option "-html"
## argument, split the line into multiple lines by splitting on space
## characters and making sure each line is less than maximum length.
##
## If the third argument, "-html", is present, return the result with
## the lines separated by html <br> line breaks, otherwise the lines
## are returned separated by newline characters.
##
    proc wrapline {line maxlen {html ""}} {
        set string [split $line " "]
        set newline [list [lindex $string 0]]
        foreach word [lrange $string 1 end] {
            if {[string length $newline]+[string length $word] > $maxlen} {
                lappend lines [join $newline " "]
                set newline {}
            }
            lappend newline $word
        }
        lappend lines [join $newline " "]
        if {$html == "-html"} {
            return [join $lines <br>]
        } else {
            return [join $lines "\n"]
        }
    }

}
