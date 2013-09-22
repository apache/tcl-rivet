##
## -- parray_table <arrayName> ?pattern? ?html-attibutes?
##
##	tablearray prints an array data in HTML table
##	This is good when a table is enough to print consistently
##	related data. 
##
##	arrayName - Name of the array to display
##	pattern   - Wildcard pattern of variables. An empty string 
##		        is tantamout a "*" and prints the whole array
##	html-attributes - 
##              list attribute-value pairs to be given
##              to the <table> element tag
##
## $Id$
##
##

namespace eval ::rivet {

    proc parray_table {arrayName {pattern "*"} {htmlAttributes ""}} {
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
            puts stdout [format "<tr><td>%s</td><td>%s</td></tr>" [::rivet::escape_sgml_chars $name]    \
                                                                  [::rivet::escape_sgml_chars $array($name)]]
        }
        puts stdout "</tbody></table>"
    }

}
