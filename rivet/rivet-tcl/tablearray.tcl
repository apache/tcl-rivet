##
## tablearray <arrayName> ?pattern? ?html-attibutes?
##
##	tablearray prints an array data in HTML table
##	This is good when a table is enough to print consistently
##	related data. 
##
##	arrayName - Name of the array to display
##	pattern -   Wildcard pattern of variables. An empty string 
##		    is tantamout a "*" and prints the whole array
##	html-attributes - list of attribute,value pairs to be put
##                  in the <table> tag
##
## $Id$
##
##

namespace eval ::rivet {

    proc tablearray {arrayName {pattern "*"} {htmlAttributes ""}} {
        upvar 1 $arrayName array
        if {![array exists array]} {
            return -code error "\"$arrayName\" isn't an array"
        }
        puts -nonewline stdout "<table"
	foreach {attr attrval} $htmlAttributes {
	    puts -nonewline " $attr=\"$attrval\""
	}

	puts "><thead><tr><th colspan=\"2\">$arrayName</th></tr></thead>"
        puts stdout "<tbody>"
        foreach name [lsort [array names array $pattern]] {
            puts stdout [format "<tr><td>%s</td><td>%s</td></tr>" $name $array($name)]
        }
        puts stdout "</tbody></table>"
    }
}
