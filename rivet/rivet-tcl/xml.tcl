#
# xml.tcl --
#
# Syntax:
#
#       ::rivet::xml string [list ?tag ?attr val? ?attr val?? ?tag ?attr val? ?attr val??]
#
# or
#
#       ::rivet::xml [list ?tag ?attr val? ?attr val??]
#
# Example 1:
#
# trivial nested markup fragment
#
#  ::rivet::xml Test b i 
# <= <b><i>Test</i></b>
#  
# Example 2:
#
# XHTML Element with attributes
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
# <= <a attr1="val1" attr2="val2"><b attr1="val1" attr2="val2"></b></a>
#
# Example 5
#
# single self closing element
#
# ::rivet::xml [list a attr1 val1 attr2 val2]
# <= <a attr1="val1" attr2="val2" />
#

namespace eval ::rivet {

    proc xml {textstring args} {

        set single_element [::rivet::lempty $args]
        if {$single_element} {

            set tags_list   [list $textstring]

            if {[::rivet::lempty $tags_list]} { return "" }

        } else {

            set tags_list $args

        }

        set tags_stack  {}
        set el          {}
        set xmlout      ""
        foreach el $tags_list {

            set el  [lassign $el tag]
            lappend tags_stack $tag
            append  xmlout "<$tag"

            foreach {attrib attrib_v} $el {
                append xmlout " $attrib=\"$attrib_v\""
            }

            append xmlout ">"
        }


        if {[::rivet::lempty $tags_stack]} {

            return $textstring

        } elseif {$single_element} {

            # variable 'el' keeps the last (innermost) attribute-value list

            if {[::rivet::lempty $el] == 1} {
                set xmlout [string replace $xmlout end end " />"]
            } else {
                set xmlout [string replace $xmlout end end "/>"]
            }

            if {[llength $tags_stack] > 1} {
                set xmlout [append xmlout "</[join [lreverse [lrange $tags_stack 0 end-1]] "></"]>"]
            }
            return $xmlout

        } else {

            return [append xmlout "$textstring</[join [lreverse $tags_stack] "></"]>"]

        }
    }

}
