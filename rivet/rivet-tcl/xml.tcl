#
# xml.tcl string ?tag ?attr val? ?attr val?? ?tag ?attr val? ?attr val??
#
# Example 1:
#
#  ::rivet::xml Test b i 
# <== <b><i>Test</i></b>
#  
# Example 2:
#
# ::rivet::xml Test [list div class box id testbox] b i
# <== <div class="box" id="testbox"><b><i>Test</i></b></div>
#
# Example 3
#
# ::rivet::xml "anything ..." div [list a href "http://..../" title "info message"] 
# <== <div><a href="http://..../" title="info message">anything ...</a></div>
#
# $Id$
#

namespace eval ::rivet {

    proc xml {textstring args} {

        set xmlout      ""
        set tags_stack  {}

        foreach el $args {

            set el  [lassign $el tag]
            lappend tags_stack $tag
            append xmlout "<$tag"

            foreach {attrib attrib_v} $el {
                append xmlout " $attrib=\"$attrib_v\""
            }

            append xmlout ">"
        }

        if {[::rivet::lempty $tags_stack]} {
            return $textstring
        } else {
            return [append xmlout "$textstring</[join [lreverse $tags_stack] "></"]>"]
        }
    }

}
