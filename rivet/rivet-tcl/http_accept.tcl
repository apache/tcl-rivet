# -- http_accept
#
# function for parsing Accept-* HTTP headers lines.
#
# http_accept parses an HTTP header line (as in the case
# for language or media type negoziation) and returns a dictionary
# where fields are associated to their precedence
#
#   Output can be controlled with the following switches
#
#      -zeroweight: appends also fiels with 0 precedence
#      -default: set default weight value to unset fields
#      -list: returns a list of the fields in the header line
#             in descending order of precedence
#
# This function was contributed by Harald Oehlmann
# 
# $Id$
#

namespace eval ::rivet {

    proc ::rivet::http_accept {args} {
        set lqValues {}
        set lItems {}

        # parameter
        while { [llength $args] > 1 } {
            set args [lassign $args argCur]
            switch -exact -- $argCur {
                -zeroweight { set fZeroWeight 1 }
                -list {set oList 1}
                -default { set fDefault 1 }
                -- {}
                default { return -code error "Unknown argument '$argCur'" }
            }
        }
        # loop over comma-separated items
        foreach itemCur [split [lindex $args 0] ,] {
            # Find q value as last element separated by ;
            set qCur 1
            if {[regexp {^(.*); *q=([^;]*)$} $itemCur match itemCur qString]} {
                if { 1 == [scan $qString %f qVal] && $qVal >= 0 && $qVal <= 1 } {
                    set qCur $qVal
                }
            }
            set itemCur [string trim $itemCur]
            if { $itemCur in {"*" "*/*" "*-*"} } {
                unset -nocomplain fDefault
            }
            if { [info exists fZeroWeight] || $qCur > 0 } {
                lappend lqValues $qCur
                lappend lItems $itemCur
            }
        }
        # build output dict in decreasing q order
        set dOut {}

        # we are going to keep a list of keys in order of decresing precedence,
        # in case the list has to be returned.

        # we return a list if oList was set otherwise a dictionary is build
        # and returned

        if {[info exists oList]} {

            set sorted_keys {}
            foreach indexCur [lsort -real -decreasing -indices $lqValues] {
                lappend sorted_keys [lindex $lItems $indexCur]
            }
            return $sorted_keys

        } else {
            foreach indexCur [lsort -real -decreasing -indices $lqValues] {
                set qCur [lindex $lqValues $indexCur]
                if {$qCur == 0 && [info exists fDefault]} {
                    dict set dOut * 0.01
                    unset fDefault
                }
                set item_key [lindex $lItems $indexCur]

                dict set dOut $item_key $qCur
            }

            if { [info exists fDefault] } {
                dict set dOut * 0.01
            }

            return $dOut
        }
    }

}
