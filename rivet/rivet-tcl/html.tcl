###
## html <string> ?arg.. arg.. arg..?
##    Print text with the added ability to pass HTML tags following the string.
##    Example:
##	html "Test" b i
##
##    Will produce:
##	<b><i>Test</i></b>
##
##    string - A text string to be displayed.
##    args   - A list of HTML tags (without <>) to surround <string> in.
###

proc html {string args} {
    foreach arg $args { append output <$arg> }
    append output $string
    for {set i [expr [llength $args] - 1]} {$i >= 0} {incr i -1} {
	append output </[lindex [lindex $args $i] 0]>
    }
    puts $output
}
