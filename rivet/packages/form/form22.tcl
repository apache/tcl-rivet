# form.tcl -- generate forms automatically.

# Copyright 2002-2021 The Apache Software Foundation

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
package provide form 2.2

# Rivet form class
#
#

::itcl::class form {

    constructor {args} {
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
        } else {
            array set DefaultValues {}
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
    # then store the resulting key-value pairs in the named array
    #
    protected method import_data {type name arrayName list} {
        upvar 1 $arrayName data

        # we now guarantee an array, though empty, will exist

        array set data {}

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
	
            if {$var == "prefix"} { 
                set prefix $val 
                continue
            }

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
    # default_value ?-list? ?--? name ?value?
    #
    # If value is not given, returns a default value
    # for that name if one exists, else an empty list.
    #
    # if a name and a value are given, the default value  is set to that
    # name (and the new default value is returned).
    #
    # The default value is a list if "-list" is given.

    method default_value {args} {
        # Command line
        if {[lindex $args 0] eq "-list"} {
            set isList 1
            set args [lrange $args 1 end]
        }
        if {[lindex $args 0] eq "--"} {
            set args [lrange $args 1 end]
        }
        switch -exact -- [llength $args] {
            1 { # Return default value
                lassign $args name
                if {default_exists $name]} {
                    if {[info exists isList]} {
                        return [default_list_get $name]
                    } else {
                        return [default_value_get $name]
                    }
                } else {
                    return
                }
            }
            2 { # Set default value
                lassign $args name value
                set DefaultValues($name) $value
                if {[info exists isList]} {
                    set DefaultValues(__$name) 1
                } else {
                    unset -nocomplain DefaultValues(__$name)
                }
            }
            default { error "wrong argument count" }
        }
    }

    #
    # default_exists - return true, if a default value exists
    protected method default_exists {name} {
        return [info exists DefaultValues($name)]
    }

    #
    # default_list_get - get the default value as a list
    # return with error if there is no default value
    protected method default_list_get {name} {
        if {[info exists DefaultValues(__$name)]} {
            return $DefaultValues($name)
        } else {
            return [list $DefaultValues($name)]
        }
    }
    #
    # default_value_get - get the default value as a value
    # return with error if there is no default value
    protected method default_value_get {name} {
        if {[info exists DefaultValues(__$name)]} {
            return [lindex $DefaultValues($name) 0]
        } else {
            return $DefaultValues($name)
        }
    }
    #
    # default_value_exists - return true, if the given value exists in the
    # default list
    protected method default_value_exists {name value} {
        if { ! [info exists DefaultValues($name)] } {
            return 0
        }
        if {[info exists DefaultValues(__$name)]} {
            return [expr {$value in $DefaultValues($name)}]
        }
        return [expr {$value eq $DefaultValues($name)}]
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
        if {[::rivet::lempty $args]} {
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
        if {![::rivet::lempty $args]} {
            # replicated in constructor
            import_data form $this arguments $args
        }
        $this emit_html "<form [argstring arguments]>"
    }

    #
    # end - generate the </form>
    #
    method end {} {
        $this emit_html "</form>"
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
                    set label "<label"
                    # if there's no id defined, generate something unique so we can reference it.
                    if { ![info exists data(id)] } {
                        set data(id) "${prefix}_[incr auto_cnt]"
                        append label { for="} $data(id) {"}
                    } else {
                        append label { for="} $data(id) {"}
                    }
                    append label ">" $data(label) "</label>"
                }

                # if there is a default value for this field
                # and it matches the value we have for it, make
                # the field show up as selected (checked)
                # Alternatively, select a checkbox, if it has no value but a
                # default value with arbitrary value.
                if {    [info exists data(value)]
                            && [default_value_exists $name $data(value)]
                        || ![info exists data(value)]
                            && $type eq "checkbox"
                            && [info exists DefaultValues($name)]
                } {
                    set data(checked) "checked"
                }
            }
        }
        # For non multi-choice widgets: set default value if there is no value
        # given
        if {    ! [info exists data(value)]
                && [default_exists $name]
                && $type ni {"select" "radio" "checkbox"}
        } {
            set data(value) [default_value_get $name]
        }
        
        # generate the field definition
        set string "<input type=\"$type\" name=\"$name\" [argstring data] />"
        if {[info exists label]} {
            append string $label
        }

        # ...and emit it
        $this emit_html $string

    }

    #
    # text -- emit an HTML "text" field
    #
    method text {name args} {
        field text $name {*}$args
    }

    #
    # password -- emit an HTML "password" field
    #
    method password {name args} {
        field password $name {*}$args
    }

    #
    # hidden -- emit an HTML "hidden" field
    #
    method hidden {name args} {
        field hidden $name {*}$args
    }

    #
    # submit -- emit an HTML "submit" field
    #
    method submit {name args} {
        field submit $name {*}$args
    }

    #
    # button -- emit an HTML "button" field
    #
    method button {name args} {
        field button $name {*}$args
    }

    #
    # reset -- emit an HTML "reset" button
    #
    method reset {name args} {
        field reset $name {*}$args
    }

    #
    #  image -- emit an HTML image field
    #
    method image {name args} {
        field image $name {*}$args
    }

    #
    # checkbox -- emit an HTML "checkbox" form field
    #
    method checkbox {name args} {
        field checkbox $name {*}$args
    }

    #
    # radio -- emit an HTML "radiobutton" form field
    #
    method radio {name args} {
        field radio $name {*}$args
    }

    #
    # color -- emit an HTML 5 "color" form field
    #
    method color {name args} {
        field color $name {*}$args
    }

    #
    # date -- emit an HTML 5 "date" form field
    #
    method date {name args} {
        field date $name {*}$args
    }

    #
    # datetime -- emit an HTML 5 "datetime" form field
    #
    method datetime {name args} {
        field datetime $name {*}$args
    }

    #
    # datetime_local -- emit an HTML 5 "datetime-local" form field
    #
    method datetime_local {name args} {
        field datetime-local $name {*}$args
    }

    #
    # email -- emit an HTML 5 "email" form field
    #
    method email {name args} {
        field email $name {*}$args
    }

    #
    # file -- emit an HTML 5 "file" form field
    #
    method file {name args} {
        field file $name {*}$args
    }

    #
    # month -- emit an HTML 5 "month" form field
    #
    method month {name args} {
        field month $name {*}$args
    }

    #
    # number -- emit an HTML 5 "number" form field
    #
    method number {name args} {
        field number $name {*}$args
    }

    #
    # range -- emit an HTML 5 "range" form field
    #
    method range {name args} {
        field range $name {*}$args
    }

    #
    # search -- emit an HTML 5 "search" form field
    #
    method search {name args} {
        field search $name {*}$args
    }

    #
    # tel -- emit an HTML 5 "tel" form field
    #
    method tel {name args} {
        field tel $name {*}$args
    }

    #
    # time -- emit an HTML 5 "time" form field
    #
    method time {name args} {
        field time $name {*}$args
    }

    #
    # url -- emit an HTML 5 "url" form field
    #
    method url {name args} {
        field url $name {*}$args
    }

    #
    # week -- emit an HTML 5 "week" form field
    #
    method week {name args} {
        field week $name {*}$args
    }

    #
    # radiobuttons -- 
    #
    method radiobuttons {name args} {
        set data(values) [list]
        set data(labels) [list]

        set list [import_data radiobuttons $name data $args]

        if {[::rivet::lempty $data(labels)]} { 
            set data(labels) $data(values) 
        }

        foreach label $data(labels) value $data(values) {
            radio $name {*}$list -label $label -value $value
        }
    }

    #
    # checkboxes -- 
    #
    method checkboxes {name args} {
        set data(values) [list]
        set data(labels) [list]

        set list [import_data checkboxes $name data $args]

        if {[::rivet::lempty $data(labels)]} { 
            set data(labels) $data(values) 
        }

        foreach label $data(labels) value $data(values) {
            checkbox $name {*}$list -label $label -value $value
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

        # get the list of default values

        if {[default_exists $name]} {
            set default_list [default_list_get $name]
        }

        # if there is a value set in the value field of the data array,
        # use that instead (that way if we're putting up a form with
        # data already, the data'll show up)
        # This data is a list for multiple forms
        if {[info exists data(value)]} {
            if {[info exists data(multiple)]} {
                set default_list $data(value)
            } else {
                set default_list [list $data(value)]
            }
            unset data(value)
        }

        #
        # if there are no separate labels defined, use the list of
        # values for the labels
        #
        if {[::rivet::lempty $labels]} { 
            set labels $values 
        }

        # emit the selector with each label-value pair
        # we adopt the style imposed by the ::rivet::xml command generating
        # the innermost elements and then wrapping them up with the 'select' tag
        set options_list {}
        foreach label $labels value $values {
            if {[info exists default_list] && $value in $default_list } {
                lappend options_list [::rivet::xml $label [list option value $value selected selected]]
            } else {
                lappend options_list [::rivet::xml $label [list option value $value]]
            }
        }
        puts [::rivet::xml [join $options_list "\n"] [list select name $name {*}[array get data]]]
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
        } elseif {[default_exists $name]} {
			set value [default_value_get $name]
		}
        $this emit_html "<textarea name=\"$name\" [argstring data]>$value</textarea>"
    }

    private method emit_html {html_fragment} {

        if {$emit} {
            puts $html_fragment
        } else {
            return $html_fragment
        }

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
    private variable auto_cnt 0
    public  variable prefix   autogen
    public  variable emit     true  { set noemit [expr !$emit] }
    public  variable noemit   false { set emit [expr !$noemit] }

} ; ## ::itcl::class form
