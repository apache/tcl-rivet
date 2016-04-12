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
#
# $Id$
#

package require Itcl

package provide form 1.0

#
# Rivet form class
#
::itcl::class form {

    constructor {args} {

	# first of all we make sure DefaultValues is an array

	array set DefaultValues {}

        # set the form method to be a post and the action to be
        # a refetching of the current page
        set arguments(method) post
        set arguments(action) [::rivet::env DOCUMENT_URI]

        # use $this for the type for form-global stuff like form arguments
        import_data form $this arguments $args

        if {[info exists arguments(defaults)]} {
            # make the public variable contain the name of the array
            # we are sucking default values out of
            set defaults $arguments(defaults)

            upvar 1 $arguments(defaults) callerDefaults

            array set DefaultValues [array get callerDefaults]
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
        if {[llength $list] % 2} {
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
            if {($var == "values") || ($var == "labels")} { continue }
            lappend return -$var $val
        }
        return $return
    }

    #
    # argstring - given an array name, construct a string of the
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
        if {[llength $args] % 2} {
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

        switch -- $type {
            "radio" -
            "checkbox" {

                # if there's a label then prepare to output it.
                if {[info exists data(label)]} {

                    # if there's no id defined, generate something 
                    # unique so we can reference it.

                    if {![info exists data(id)]} {
                        set data(id) "autogen[incr field_cnt]"
                    }
                    set label "<label for=\"$data(id)\">$data(label)</label>"
                }

                # ...and if the is a default value for this field
                # and it matches the value we have for it, make
                # the field show up as selected (checked)

                if {[info exists DefaultValues($name)] && [info exists data(value)]} {

		    # if there is no __$var variable in the defaults we are
		    # dealing with a single value so we don't look it up 
		    # with lsearch, but we compare the 2 variable as strings, 
		    # so that spaces in the value won't be confused as a list of
		    # multiple elements....

		    if {![info exists DefaultValues(__${name})]} {
			if {[string match $DefaultValues($name) $data(value)]} {
			    set data(checked) "checked"
			}			
		    } else {
			if {[lsearch $DefaultValues($name) $data(value)] >= 0} {
			    set data(checked) "checked"
			}
		    }
                }
            }
        }
        # generate the field definition
        set string "<input type=\"$type\" name=\"$name\" [argstring data] />"
        if {[info exists label]} {
            append string $label
        }

        # ...and emit it
        html $string

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
    # button -- emit an HTML "button" field
    #
    method button {name args} {
        eval field button $name $args
    }

    #
    # reset -- emit an HTML "reset" button
    #
    method reset {name args} {
        eval field reset $name $args
    }

    #
    #  image -- emit an HTML image field
    #
    method image {name args} {
        eval field image $name $args
    }

    #
    # checkbox -- emit an HTML "checkbox" form field
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
    # color -- emit an HTML 5 "color" form field
    #
    method color {name args} {
        eval field color $name $args
    }

    #
    # date -- emit an HTML 5 "date" form field
    #
    method date {name args} {
        eval field date $name $args
    }

    #
    # datetime -- emit an HTML 5 "datetime" form field
    #
    method datetime {name args} {
        eval field datetime $name $args
    }

    #
    # datetime_local -- emit an HTML 5 "datetime-local" form field
    #
    method datetime_local {name args} {
        eval field datetime-local $name $args
    }

    #
    # email -- emit an HTML 5 "email" form field
    #
    method email {name args} {
        eval field email $name $args
    }

    #
    # file -- emit an HTML 5 "file" form field
    #
    method file {name args} {
        eval field file $name $args
    }

    #
    # month -- emit an HTML 5 "month" form field
    #
    method month {name args} {
        eval field month $name $args
    }

    #
    # number -- emit an HTML 5 "number" form field
    #
    method number {name args} {
        eval field number $name $args
    }

    #
    # range -- emit an HTML 5 "range" form field
    #
    method range {name args} {
        eval field range $name $args
    }

    #
    # search -- emit an HTML 5 "search" form field
    #
    method search {name args} {
        eval field search $name $args
    }

    #
    # tel -- emit an HTML 5 "tel" form field
    #
    method tel {name args} {
        eval field tel $name $args
    }

    #
    # time -- emit an HTML 5 "time" form field
    #
    method time {name args} {
        eval field time $name $args
    }

    #
    # url -- emit an HTML 5 "url" form field
    #
    method url {name args} {
        eval field url $name $args
    }

    #
    # week -- emit an HTML 5 "week" form field
    #
    method week {name args} {
        eval field week $name $args
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
            html "$string$label</option>"
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

    private variable field_cnt       0
    private variable DefaultValues
    private variable DefaultArgs

    private variable arguments

} ; ## ::itcl::class form
