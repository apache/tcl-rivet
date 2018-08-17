#
# xml.tcl string ?tag ?attr val? ?attr val?? ?tag ?attr val? ?attr val??
#
# Example 1:
#
#  ::rivet::xml Test b i 
# <= <b><i>Test</i></b>
#  
# Example 2:
#
# ::rivet::xml Test [list div class box id testbox] b i
# <= <div class="box" id="testbox"><b><i>Test</i></b></div>
#
# Example 3
#
# ::rivet::xml "anything ..." div [list a href "http://..../" title "info message"] 
# <= <div><a href="http://..../" title="info message">anything ...</a></div>
#
# Example 4
#
# ::rivet::xml "" [list a attr1 val1 attr2 val2] [list b attr1 val1 attr2 val2]
# <= <a attr1="val1" attr2="val2"><b attr1="val1" attr2="val2"/></a>
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
            set last_elem_attr $el
        }


        if {[::rivet::lempty $tags_stack]} {

            return $textstring

        } else {

            if {$textstring == ""} {

                if {[llength $last_elem_attr]} { 
                    set closing_seq "/>"
                } else {
                    set closing_seq " />"
                }
                set xmlout [string replace $xmlout end end $closing_seq]
                if {[llength $tags_stack] > 1} {
                    set xmlout [append xmlout "</[join [lreverse [lrange $tags_stack 0 end-1]] "></"]>"]
                }
                return $xmlout

            }
            return [append xmlout "$textstring</[join [lreverse $tags_stack] "></"]>"]

        }
    }
}
