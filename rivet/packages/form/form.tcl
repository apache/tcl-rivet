# form.tcl -- generate forms automatically.

# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


package require Itcl

package provide form 1.0

::itcl::class form {

    constructor {args} {
	set arguments(method) post
	set arguments(action) [env DOCUMENT_URI]

	import_data form $this arguments $args

	if {[info exists arguments(defaults)]} {
	    upvar 1 $arguments(defaults) defaults
	    array set DefaultValues [array get defaults]
	    unset arguments(defaults)
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
	    set data(value) $DefaultValues($name)
	}

	if {[expr [llength $list] % 2]} {
	    return -code error "Unmatched key-value pairs"
	}

	set return ""
	foreach {var val} $list {
	    set var [string range [string tolower $var] 1 end]
	    set data($var) $val
	    if {$var == "values"} { continue }
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
	html "<form [argstring arguments]>"
    }

    method end {} {
	html "</form>"
    }

    method field {type name args} {
	import_data $type $name data $args
	set string "<input type=\"$type\" name=\"$name\""
	append string [argstring data]
	switch -- $type {
	    "radio" -
	    "checkbox" {
		if {![info exists data(value)]} { set data(value) "" }
		if {![info exists data(label)]} { set data(label) $data(value) }
		if {[info exists DefaultValues($name)]} {
		    if {$data(value) == $DefaultValues($name)} {
			append string { checked="checked"}
		    }
		}
	    }
	}
	append string " />"
	if {$type == "radio"} {
	    html $string$data(label)
	} else {
	    html $string
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
	set data(values) [list]
	set data(labels) [list]
	set list [import_data radiobuttons $name data $args]
	if {[lempty $data(labels)]} { set data(labels) $data(values) }
	foreach label $data(labels) value $data(values) {
	    eval radio $name $list -label $label -value $value
	}
    }

    method select {name args} {
	set data(values) [list]
	set data(labels) [list]
	import_data select $name data $args
	set values $data(values)
	set labels $data(labels)
	unset data(values) data(labels)
	set default ""
	if {[info exists DefaultValues($name)]} {
	    set default $DefaultValues($name)
	}
	if {[info exists data(value)]} {
	    set default $data(value)
	    unset data(value)
	}

	if {[lempty $labels]} { set labels $values }

	html "<select name=\"$name\" [argstring data]>"
	foreach label $labels value $values {
	    if {$value == $default} {
		set string "<option value=\"$value\" selected=\"selected\">"
	    } else {
		set string "<option value=\"$value\">"
	    }
	    html $string$label
	}
	html "</select>"
    }

    method textarea {name args} {
	import_data textarea $name data $args
	set value ""
	if {[info exists data(value)]} {
	    set value $data(value)
	    unset data(value)
	}
	html "<textarea name=\"$name\" [argstring data]>$value</textarea>"
    }

    public variable defaults "" {
	upvar 1 $defaults array
	array set DefaultValues [array get array]
    }

    private variable DefaultValues
    private variable DefaultArgs

    private variable arguments

} ; ## ::itcl::class form
