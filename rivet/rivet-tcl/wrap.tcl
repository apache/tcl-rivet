proc wrap {string maxlen {html ""}} {
    set splitstring {}
    foreach line [split $string "\n"] {
	lappend splitstring [wrapline $line $maxlen $html]
    }
    if {$html == "-html"} {
	return [join $splitstring "<br>"]
    } else {
	return [join $splitstring "\n"]
    }
}

proc wrapline {line maxlen {html ""}} {
    set string [split $line " "]
    set newline [list [lindex $string 0]]
    foreach word [lrange $string 1 end] {
	if {[string length $newline]+[string length $word] > $maxlen} {
	    lappend lines [join $newline " "]
	    set newline {}
	}
	lappend newline $word
    }
    lappend lines [join $newline " "]
    if {$html == "-html"} {
	return [join $lines <br>]
    } else {
	return [join $lines "\n"]
    }
}
