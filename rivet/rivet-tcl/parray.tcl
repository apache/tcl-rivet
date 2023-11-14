###
## parray <arrayName> ?pattern?
##    An html version of the standard Tcl 'parray' command.
##    Displays the entire contents of an array in a sorted, nicely-formatted
##    way.  Mostly used for debugging purposes.
##
##    arrayName - Name of the array to display.
##    pattern   - A wildcard pattern of variables within the array to display.
##
###

namespace eval ::rivet {

    proc parray {arrayName {pattern *} {outputcmd "puts stdout"}} {
        upvar 1 $arrayName array
        if {![array exists array]} {
            return -code error "\"$arrayName\" isn't an array"
        }
        set maxl 0
        foreach name [lsort [array names array $pattern]] {
            if {[string length $name] > $maxl} {
                set maxl [string length $name]
            }
        }
        set html_text [list "<b>$arrayName</b>"]
        set maxl [expr {$maxl + [string length $arrayName] + 2}]
        foreach name [lsort [array names array $pattern]] {
            set nameString [format "%s(%s)" $arrayName [::rivet::escape_sgml_chars $name]]
            lappend html_text [format "%-*s = %s" $maxl $nameString [::rivet::escape_sgml_chars $array($name)]]
        }
        eval [list {*}$outputcmd [join [list <pre> [join $html_text "\n"] </pre>] "\n"]]

    }

}
