package require Itcl

package provide form 1.0

::itcl::class form {

    constructor {args} {
	set arguments(METHOD) POST
	set arguments(ACTION) [env DOCUMENT_URI]

	import_data form $this arguments $args

	if {[info exists arguments(DEFAULTS)]} {
	    upvar 1 $arguments(DEFAULTS) defaults
	    array set DefaultValues [array get defaults]
	    unset arguments(DEFAULTS)
	}
    }

    destructor {

    }

    method destroy {} {
	::itcl::delete object $this
    }

    protected method import_data {type name arrayName list} {
	upvar 1 $arrayName data

	if {[info exists DefaultArgs($type)]} {
	    set list [concat $DefaultArgs($type) $list]
	}

	if {[info exists DefaultValues($name)]} {
	    set data(VALUE) $DefaultValues($name)
	}

	if {[expr [llength $list] % 2]} {
	    return -code error "Unmatched key-value pairs"
	}

	set return ""
	foreach {var val} $list {
	    set var [string range [string toupper $var] 1 end]
	    set data($var) $val
	    if {$var == "VALUES"} { continue }
	    lappend return -$var $val
	}
	return $return
    }

    protected method argstring {arrayName} {
	upvar 1 $arrayName data
	set string ""
	foreach arg [lsort [array names data]] {
	    append string " $arg=\"$data($arg)\""
	}
	return $string
    }

    method default_value {name {newValue ""}} {
	if {[lempty $newValue]} {
	    if {![info exists DefaultValues($name)]} { return }
	    return $DefaultValues($name)
	}
	return [set DefaultValues($name) $newValue]
    }

    method default_args {type args} {
	if {[lempty $args]} {
	    if {![info exists DefaultArgs($type)]} { return }
	    return $DefaultArgs($type)
	}
	if {[expr [llength $args] % 2]} {
	    return -code error "Unmatched key-value pairs"
	}
	return [set DefaultArgs($type) $args]
    }

    method start {} {
	puts "<FORM [argstring arguments]>"
    }

    method end {} {
	puts "</FORM>"
	destroy
    }

    method field {type name args} {
	import_data $type $name data $args
	set string "<INPUT TYPE=\"$type\" NAME=\"$name\""
	append string [argstring data]
	switch -- $type {
	    "radio" -
	    "checkbox" {
		if {![info exists data(VALUE)]} { set data(VALUE) "" }
		if {[info exists DefaultValues($name)]} {
		    if {$data(VALUE) == $DefaultValues($name)} {
			append string " CHECKED"
		    }
		}
	    }
	}
	append string ">"
	if {$type == "radio"} {
	    puts $string$data(VALUE)
	} else {
	    puts $string
	}
    }

    method text {name args} {
	eval field text $name $args
    }

    method password {name args} {
	eval field password $name $args
    }

    method hidden {name args} {
	eval field hidden $name $args
    }

    method submit {name args} {
	eval field submit $name $args
    }

    method reset {name args} {
	eval field reset $name $args
    }

    method image {name args} {
	eval field image $name $args
    }

    method checkbox {name args} {
	eval field checkbox $name $args
    }

    method radio {name args} {
	eval field radio $name $args
    }

    method radiobuttons {name args} {
	set list [import_data radiobuttons $name data $args]
	if {![info exists data(VALUES)]} {
	    return -code error "No -values specified for radiobuttons"
	}
	foreach value $data(VALUES) {
	    eval radio $name $list -value $value
	}
    }

    method select {name args} {
	import_data select $name data $args
	set values $data(VALUES)
	unset data(VALUES)
	set default ""
	if {[info exists DefaultValues($name)]} {
	    set default $DefaultValues($name)
	}
	puts "<SELECT NAME=\"$name\" [argstring data]>"
	foreach value $values {
	    if {$value == $default} {
		set string "<OPTION SELECTED>"
	    } else {
		set string "<OPTION>"
	    }
	    puts $string$value
	}
	puts "</SELECT>"
    }

    method textarea {name args} {
	import_data textarea $name data $args
	set value ""
	if {[info exists data(VALUE)]} {
	    set value $data(VALUE)
	    unset data(VALUE)
	}
	puts "<TEXTAREA NAME=\"$name\" [argstring data]>$value</TEXTAREA>"
    }

    public variable defaults "" {
	upvar 1 $defaults array
	unset array
	array set DefaultValues [array get array]
    }

    private variable DefaultValues
    private variable DefaultArgs

    private variable arguments

} ; ## ::itcl::class form
