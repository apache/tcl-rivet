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

#
# Rivet form class
#
::itcl::class form {

    constructor {args} {
	# set the form method to be a post and the action to be
	# a refetching of the current page
	set arguments(method) post
	set arguments(action) [env DOCUMENT_URI]

	# use $this for the type for form-global stuff like form arguments
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

    #
    # import_data -- given a field type, field name, name of an array, and a 
    # list of key-value pairs, prepend any default key-value pairs,
    # set a default value if one exists, then store the resulting
    # key-value pairs in the named array
    #
    protected method import_data {type name arrayName list} {
	upvar 1 $arrayName data

	#
	# If there are elements in the defaultArgs array for the
	# specified type, combine them with the list of key-value
	# pairs, putting the DefaultArgs values first so the
	# key-value pairs from list can possibly override them.
	#
	if {[info exists DefaultArgs($type)]} {
	    set list [concat $DefaultArgs($type) $list]
	}

	#
	# if there is a default value for the name stored in the
	# DefaultValues array in this class, set the value
	# element of the array the caller named to contain that
	# value
	#
	if {[info exists DefaultValues($name)]} {
	    set data(value) $DefaultValues($name)
	}

	#
	# if we don't have an even number of key-value pairs,
	# that just ain't right
	#
	if {[expr [llength $list] % 2]} {
	    return -code error "Unmatched key-value pairs"
	}

	#
	# for each key-value pair in the list, strip the first
	# dash character from the key part and map it to lower
	# case, then use that as the key for the passed-in
	# array and store the corresonding value in there
	#
	# we also prep and return the list of key-value pairs, normalized
	# with the lowercase thing
	#
	set return ""
	foreach {var val} $list {
	    set var [string range [string tolower $var] 1 end]
	    set data($var) $val
	    if {$var == "values"} { continue }
	    lappend return -$var $val
	}
	return $return
    }

    #
    # argstring - given an array name, construct areturn string of the
    # style key1="data1" key2="data2" etc for each key value pair in the
    # array
    #
    protected method argstring {arrayName} {
	upvar 1 $arrayName data
	set string ""
	foreach arg [lsort [array names data]] {
	    append string " $arg=\"$data($arg)\""
	}
	return $string
    }

    #
    # default_value -- if only a name is given, returns a default value
    # for that name if one exists else an empty list.
    #
    # if a name and a value are given, the default value  is set to that
    # name (and the new default value is returned).
    #
    method default_value {name {newValue ""}} {
	if {[lempty $newValue]} {
	    if {![info exists DefaultValues($name)]} { return }
	    return $DefaultValues($name)
	}
	return [set DefaultValues($name) $newValue]
    }

    #
    # default_args - given a type and a variable number of arguments,
    #  if there are no arguments other than the type, return the
    #  element of that name from the DefaultArgs array, if that element
    #  exists, else return an empty list.
    #
    # if a name and a value are given, sets the DefaultArgs to the variable
    # list of arguments.
    #
    method default_args {type args} {

	# if only one argument was specified
	if {[lempty $args]} {
	    if {![info exists DefaultArgs($type)]} { return }
	    return $DefaultArgs($type)
	}

	# make sure we have an even number of key-value pairs
	if {[expr [llength $args] % 2]} {
	    return -code error "Unmatched key-value pairs"
	}

	# set the DefaultArgs for the specified type
	return [set DefaultArgs($type) $args]
    }

    #
    # start - generate the <form> with all of its arguments
    #
    method start {{args ""}} {
        if {![lempty $args]} {
	    # replicated in constructor
	    import_data form $this arguments $args
	}
	html "<form [argstring arguments]>"
    }

    #
    # end - generate the </form>
    #
    method end {} {
	html "</form>"
    }

    #
    # field - emit a field of the given field type and name, including
    # any default key-value pairs defined for this field type and
    # optional key-value pairs included with the statement
    #
    method field {type name args} {

	# import any default key-value pairs, then any specified in this
	# field declaration
	import_data $type $name data $args

	# generate the field definition
	set string "<input type=\"$type\" name=\"$name\""
	append string [argstring data]

	switch -- $type {
	    "radio" -
	    "checkbox" {
		# if there's no value defined, create an empty value
		if {![info exists data(value)]} { 
		    set data(value) "" 
		}

		# if there's no label defined, make the label be the
		# same as the value
		if {![info exists data(label)]} { 
		    set data(label) $data(value) 
		}

		# ...and if the is a default value for this field
		# and it matches the value we have for it, make
		# the field show up as selected (checked)
		if {[info exists DefaultValues($name)]} {
		    if {$data(value) == $DefaultValues($name)} {
			append string { checked="checked"}
		    }
		}
	    }
	}
	append string " />"

	# ...and emit it
	if {$type == "radio"} {
	    html $string$data(label)
	} else {
	    html $string
	}
    }

    #
    # text -- emit an HTML "text" field
    #
    method text {name args} {
	eval field text $name $args
    }

    #
    # password -- emit an HTML "password" field
    #
    method password {name args} {
	eval field password $name $args
    }

    #
    # hidden -- emit an HTML "hidden" field
    #
    method hidden {name args} {
	eval field hidden $name $args
    }

    #
    # submit -- emit an HTML "submit" field
    #
    method submit {name args} {
	eval field submit $name $args
    }

    #
    # reset -- emit an HTML "reset" button
    #
    method reset {name args} {
	eval field reset $name $args
    }

    #
    # reset -- emit an HTML image field
    #
    method image {name args} {
	eval field image $name $args
    }

    #
    # reset -- emit an HTML "checkbox" form field
    #
    method checkbox {name args} {
	eval field checkbox $name $args
    }

    #
    # radio -- emit an HTML "radiobutton" form field
    #
    method radio {name args} {
	eval field radio $name $args
    }

    #
    # radiobuttons -- 
    #
    method radiobuttons {name args} {
	set data(values) [list]
	set data(labels) [list]

	set list [import_data radiobuttons $name data $args]

	if {[lempty $data(labels)]} { 
	    set data(labels) $data(values) 
	}

	foreach label $data(labels) value $data(values) {
	    eval radio $name $list -label $label -value $value
	}
    }

    #
    # select -- generate a selector
    #
    # part of the key value pairs can include -values with a list,
    # and -labels with a list and it'll populate the <option>
    # elements with them.  if one matches the default value,
    # it'll select it too.
    #
    method select {name args} {
	# start with empty values and labels so they'll exist even if not set
	set data(values) [list]
	set data(labels) [list]

	# import any default data and key-value pairs from the method args
	import_data select $name data $args

	# pull the values and labels into scalar variables and remove them
	# from the data array
	set values $data(values)
	set labels $data(labels)
	unset data(values) data(labels)

	# get the default value, use an empty string if there isn't one
	set default ""
	if {[info exists DefaultValues($name)]} {
	    set default $DefaultValues($name)
	}

	# if there is a value set in the value field of the data array,
	# use that instead (that way if we're putting up a form with
	# data already, the data'll show up)
	if {[info exists data(value)]} {
	    set default $data(value)
	    unset data(value)
	}

	#
	# if there are no separate labels defined, use the list of
	# values for the labels
	#
	if {[lempty $labels]} { 
	    set labels $values 
	}

	# emit the selector
	html "<select name=\"$name\" [argstring data]>"

	# emit each label-value pair
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

    #
    # textarea -- emit an HTML "textarea" form field
    #
    method textarea {name args} {
	import_data textarea $name data $args
	set value ""
	if {[info exists data(value)]} {
	    set value $data(value)
	    unset data(value)
	}
	html "<textarea name=\"$name\" [argstring data]>$value</textarea>"
    }

    #
    # defaults -- when set, the value is the name of an array to suck
    # the key-value pairs out of and copy them into DefaultValues
    #
    public variable defaults "" {
	upvar 1 $defaults array
	array set DefaultValues [array get array]
    }

    private variable DefaultValues
    private variable DefaultArgs

    private variable arguments

} ; ## ::itcl::class form
