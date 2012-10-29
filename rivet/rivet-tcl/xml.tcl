#
# xml.tcl string ?tag ?attr val? ?attr val?? ?tag ?attr val? ?attr val??
#
# Example 1:
#
#  ::rivet::xml Test b i -> <b><i>Test</i></b>
#  
# Example 2:
#
# ::rivet::xml Test [list div class box id testbox] b i
#
#    -> <div class="box" id="testbox"><b><i>Test</i></b></div>
#
# Example 3
#
# set d [list div [list a 1 b 2] b [list c 3 d 4] i [list e 5 f 6]]
# ::rivet::xml Test {*}$d
#
#   -> <div a="1" b="2"><b c="3" d="4"><b c="3" d="4"><i e="5" f="6">Test</i></b></div>
#
# $Id: $
#

namespace eval ::rivet {

    proc xml {textstring args} {

        set xmlout ""
        set tags_stack   {}

        foreach el $args {

            set tag  [lindex $el 0]
            set tags_stack [linsert $tags_stack 0 $tag]
            append xmlout "<$tag"

            if {[llength $el] > 1} {

                foreach {attrib attrib_v} [lrange $el 1 end] {
                    append xmlout " $attrib=\"$attrib_v\""
                }

            } 

            append xmlout ">"
        }

        append xmlout "$textstring</[join $tags_stack ></]>"

    }
}
