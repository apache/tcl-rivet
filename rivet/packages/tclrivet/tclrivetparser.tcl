# tclrivetparser.tcl -- parse Rivet files in pure Tcl.

# $Id$

package provide tclrivetparser 0.1

namespace eval tclrivetparser {
    set starttag <?
    set endtag   ?>
    namespace export parserivetdata
}

# tclrivetparser::parse --
#
#	Parse a buffer, transforming <? and ?> into the appropriate
#	Tcl strings.  Note that initial 'puts "' is not performed
#	here.
#
# Arguments:
#	data - data to scan.
#	outbufvar - name of the output buffer.
#
# Side Effects:
#	None.
#
# Results:
#	Returns the $inside variable - 1 if we are inside a <? ?>
#	section, 0 if we outside.

proc tclrivetparser::parse { data outbufvar } {
    variable starttag
    variable endtag
    set inside 0

    upvar $outbufvar outbuf

    set i 0
    set p 0
    set len [expr {[string length $data] + 1}]
    set next [string index $data 0]
    while {$i < $len} {
	incr i
	set cur $next
	set next [string index $data $i]
	if { $inside == 0 } {
	    # Outside the delimiting tags.
	    if { $cur == [string index $starttag $p] } {
		incr p
		if { $p == [string length $starttag] } {
		    append outbuf "\"\n"
		    set inside 1
		    set p 0
		    continue
		}
	    } else {
		if { $p > 0 } {
		    append outbuf [string range $starttag 0 [expr {$p - 1}]]
		    set p 0
		}
		switch -exact -- $cur {
		    "\{" {
			append outbuf "\\{"
		    }
		    "\}" {
			append outbuf "\\}"
		    }
		    "\$" {
			append outbuf "\\$"
		    }
		    "\[" {
			append outbuf "\\["
		    }
		    "\]" {
			append outbuf "\\]"
		    }
		    "\"" {
			append outbuf "\\\""
		    }
		    "\\" {
			append outbuf "\\\\"
		    }
		    default {
			append outbuf $cur
		    }
		}
		continue
	    }
	} else {
	    # Inside the delimiting tags.
	    if { $cur == [string index $endtag $p] } {
		incr p
		if { $p == [string length $endtag] } {
		    append outbuf "\nputs -nonewline \""
		    set inside 0
		    set p 0
		}
	    } else {
		if { $p > 0 } {
		    append outbuf [string range $endtag 0 $p]
		    set p 0
		}
		append outbuf $cur
	    }
	}
    }
    return $inside
}


# tclrivetparser::parserivetdata --
#
#	Parse a rivet script, and add the relavant opening and closing
#	bits.
#
# Arguments:
#	data - data to parse.
#
# Side Effects:
#	None.
#
# Results:
#	Returns the parsed script.

proc tclrivetparser::parserivetdata { data } {
    set outbuf {}
    append outbuf "puts -nonewline \""
    if { [parse $data outbuf] == 0 } {
	append outbuf "\"\n"
    }
    return $outbuf
}
