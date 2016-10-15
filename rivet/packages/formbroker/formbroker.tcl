# -- formbroker.tcl
# 
# Form validation and sanitation tool. Kindly donated by
# Karl Lehenbauer (Flightaware.com)
#

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

namespace eval FormBroker {
    variable form_definitions   [dict create]
    variable form_list          [dict create]
    variable string_quote       force_quote
    variable form_count         0
    #
    # response_security_error - issue an error with errorCode
    #
    #   set appropriate -- we expect the rivet error handler
    #   to catch this and do the right thing
    #

    proc response_security_error {type message} {

        error $message "" [list RIVET SECURITY $type $message]

    }

    #
    # require_response_vars - error if any of the specified are not in the response
    #

    proc require_response_vars {_response args} {
        upvar $_response response

        foreach var $args {
            if {![info exists response($var)]} {

                response_security_error MISSING_VAR \
                            "var $var not present in $_response"

            }
        }
    }

    #
    # force_response_integers - error if any of named vars in response doesn't exist
    #
    #   or isn't an integer
    #

    proc force_response_integers {_response args} {
        upvar $_response response

        require_response_vars response {*}$args

        foreach var $args {

            if {![regexp {[0-9-]*} response($var)]} {
                response_security_error NOT_INTEGER "illegal content in $var"
            }

            if {![scan $response($var) %d response($var)]} {

                response_security_error NOT_INTEGER "illegal content in $var"

            }
        }

    }


    #
    # force_response_integer_in_range - error if var in response isn't an integer
    # or if it isn't in range
    #

    proc force_response_integer_in_range {_response var lowest highest} {
        upvar $_response response

        force_response_integers response $var

        if {$response($var) < $lowest || $response($var) > $highest} {
            response_security_error "OUT_OF_RANGE" "$var out of range"
        }

    }

    # -- force_quote 
    #

    proc force_quote {str} {
        return "'$str'"
    }


    # -- force_sanitize_response_strings
     
    proc force_sanitize_response_strings {_response args} { }


    #
    # force_quote_response_strings - sanitize and pg_quote all the specified strings in the array
    #

    proc force_quote_response_strings {_response args} {
        upvar $_response response

        force_sanitize_response_strings response {*}$args

        foreach var $args {
            set response($var) [$string_quote $response($var)]
        }

    }

     

    #
    # -- force_quote_response_unfilteredstrings - rewrite named response
    # elements pg_quoted
    #

    proc force_quote_response_unfilteredstrings {_response args} {
        upvar $_response response

        require_response_vars response {*}$args

        foreach var $args {
            set response($var) [$string_quote $response($var)]
        }

    }
    
    # -- base validators
    
    proc validate_string {_var_d} {
        upvar $_var_d var_d

        set valid FB_OK
        dict with var_d {
            if {$bounds > 0} {
                if {$constrain} {
                    set var [string range $var 0 $bounds-1]
                } elseif {[string length $var] > $bounds} {
                    set valid FB_STRING_TOO_LONG
                }
            }
        }
        return $valid
    }

    proc validate_integer {_var_d} {
        upvar $_var_d var_d
        #puts "var_d: $var_d"

        set valid FB_OK
        dict with var_d {
            if {![string is integer $var]} {
                return NOT_INTEGER
            }

            if {[llength $bounds] == 2} {
                ::lassign $bounds min_v max_v

                if {$constrain} {
                    set var [expr min($var,$max_v)]
                    set var [expr max($var,$min_v)]
                    set valid FB_OK
                } elseif {($var > $max_v) || ($var < $min_v)} {
                    set valid FB_OUT_OF_BOUNDS
                } else {
                    set valid FB_OK
                }


            } elseif {([llength $bounds] == 1) && ($bounds > 0)} {

                if {$constrain} {
                    set var [expr min($bounds,$var)]
                    set var [expr max(-$bounds,$var)]
                    set valid FB_OK
                } elseif {(abs($var) > $bounds)} {
                    set valid FB_OUT_OF_BOUNDS
                } else {
                    set valid FB_OK
                }

            }
        }
        return $valid
    }

    proc validate_unsigned {_var_d} {
        upvar $_var_d var_d

        dict with var_d {
            if {![string is integer $var]} {
                return NOT_INTEGER
            }
            if {[llength $bounds] == 2} {
                ::lassign $bounds min_v max_v
                if {$constrain} {
                    set var [expr min($var,$max_v)]
                    set var [expr max($var,$min_v)]
                    set valid FB_OK
                } elseif {($var > $max_v) || ($var < $min_v)} {
                    set valid FB_OUT_OF_BOUNDS
                } else {
                    set valid FB_OK
                }

            } elseif {([llength $bounds] == 1) && \
                      ($bounds > 0)} {
                
                if {$constrain} {
                    set var [expr max(0,$var)]
                    set var [expr min($bounds,$var)]
                    set valid FB_OK
                } elseif {($var > $bounds) || ($var < 0)} {
                    set valid FB_OUT_OF_BOUNDS
                } else {
                    set valid FB_OK
                }

            } else {

                if {$constrain} {
                    set var [expr max(0,$var)]
                    set valid FB_OK
                } elseif {$var < 0} {
                    set valid FB_OUT_OF_BOUNDS
                } else {
                    set valid FB_OK
                }
            }
        }
        return $valid
    }

    proc validate_email {_var_d} {
        upvar $_var_d var_d

        dict with var_d {
            if {[regexp -nocase {[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}} $var]} {
                return FB_OK
            } else {
                return FB_INVALID_EMAIL
            }
        }
    }

    proc validate_form_var {_var_d} {
        upvar $_var_d var_d
        variable form_definitions

        set validator [dict get $var_d validator]
        if {[info commands $validator] == ""} {
            set validator ::FormBroker::validate_string
        }
        set validation [$validator var_d]

        dict set var_d field_validation $validation

        return [string match $validation FB_OK]
    }


    # -- constrain_bounds
    #
    # During the form creation stage this method is called
    # to correct possible inconsistencies with a field bounds 
    # definition
    #

    proc constrain_bounds {field_type _bounds} {
        upvar $_bounds bounds

        switch $field_type {
            integer {
                if {[llength $bounds] == 1} {

                    set bounds [list [expr -abs($bounds)] [expr abs($bounds)]]

                } elseif {[llength $bounds] > 1} {
                    lassign $bounds l1 l2

                    set bounds [list [expr min($l1,$l2)] [expr max($l1,$l2)]]
                } else {
                    set bounds 0
                }
            }
            unsigned {
                if {[llength $bounds] == 1} {

                    set bounds [list 0 [expr abs($bounds)]]

                } elseif {[llength $bounds] > 1} {

                    lassign $bounds l1 l2
                    if {$l1 < 0} { set l1 0 }
                    if {$l2 < 0} { set l2 0 }

                    set bounds [list [expr min($l1,$l2)] [expr max($l1,$l2)]]
                } else {
                    set bounds 0
                }
            }
        }
    }

    # - fields
    #
    # currently this call returns the dictionary
    # of form field definitions. It's not meant to be
    # used in regular development. It's supposed to be
    # private to the FormBroker package
    # and it may go away with future developments or
    # change its interface and returned value

    proc fields {form_name} {
        variable form_definitions

        return [dict get $form_definitions $form_name]
    }

    # -- failing
    #
    # returns a list of variable-status pairs for each
    # field in a form that did not validate
    #

    proc failing {form_name} {
        set res {}
        dict for {field field_d} [fields $form_name] {
            dict with field_d {
                if {$field_validation != "FB_OK"} {
                    lappend res $field $field_validation
                }
            }
        }
        return $res
    }

    # -- result
    #
    # accessor to the form field definitions. This procedure
    # too is not (at least temporarily) to be called from
    # outside the package
    #

    proc result {form_name form_field} {
        variable form_definitions

        return [dict get $form_definitions $form_name $form_field]
    }

    # -- validate
    #
    # 

    proc validate { form_name _response} {
        upvar $_response response
        variable form_definitions
        variable form_list
        variable string_quote

        set form_valid true
        #set fd [dict get $form_definitions $form_name]
        set form_validation FB_VALIDATION_ERROR

        set vars_to_validate [dict get $form_list $form_name vars]
        if {[catch {
                require_response_vars response {*}$vars_to_validate
            } er eopts]} {

            #puts "$er $eopts"
            dict set form_list $form_name form_validation $er
            return false

        }

        # field validation

        dict with form_list $form_name {
            #set failing            {}
            set form_validation     FB_OK
        }

        set form_d [dict get $form_definitions $form_name]
        #puts "form_d: $form_d"

        dict for {var variable_d} $form_d {

            dict set variable_d var $response($var)
            if {[validate_form_var variable_d] == 0} {
                dict set form_list $form_name form_validation FB_VALIDATION_ERROR
                #dict with form_list $form_name {
                #    ::lappend failing $var
                #}
                set form_valid false
            }

            # in case it was constrained we write the value back
            # into the response array

            if {[dict get $variable_d constrain]} { 
                set response($var) [dict get $variable_d var] 
            }

            if {[dict get $variable_d force_quote]} {
                set response($var)  [$string_quote [dict get $variable_d var]]
            }


            dict set form_definitions $form_name $var $variable_d
            #puts "validate $var -> $variable_d"

        }

        #dict set form_definitions $form_name $fd
        return $form_valid
    }

    # -- destroy
    #
    #

    proc destroy {form_name} {
        variable form_definitions
        variable form_list

        dict unset form_definition $form_name
        dict unset form_list       $form_name
        namespace delete $form_name
    }

    # -- initialize
    #
    # creates a form object starting from a list of element descriptors
    #
    # the procedure accept a list of single descriptors, being each 
    # descriptor a sub-list itself
    #
    #  - field_name
    #  - type (string, integer, unsigned, email, base64)
    #  - a list of the following keywords and related values
    #
    #  - bounds <value>
    #  - bounds [low high]
    #  - check_routine [validation routine]
    #  - length [max length]
    #

    proc initialize {args} {
        variable form_definitions
        variable form_list
        variable form_count
        variable string_quote

        set form_name "form${form_count}"
        incr form_count

        catch { namespace delete $form_name }
        namespace eval $form_name {

            foreach cmd {validate failing fields result destroy} {
                lappend cmdmap $cmd [list [namespace parent] $cmd [namespace tail [namespace current]]]
            }

            namespace ensemble create -map [dict create {*}$cmdmap]
            unset cmdmap
            unset cmd

        }

        dict set form_definitions $form_name [dict create]
        dict set form_list $form_name   [dict create    vars            {}      \
                                                        form_validation FB_OK   \
                                                        failing         {}      \
                                                        quoting         $string_quote]

        while {[llength $args]} {

            set args [::lassign $args e]

            if {$e == "-quoting"} {

                dict with form_list $form_name {
                    set args [::lassign $args quoting]

                    if {[info proc $quoting] == ""} {
                        error [list RIVET INVALID_QUOTING_PROC \
                                          "Non existing quoting proc '$quoting'"]
                    }

                }
                continue

            }

            # each variable (field) definition must start with the
            # variable name and variable type. Every other variable
            # specification argument can be listed in arbitrary order
            # with the only constraint that argument values must follow
            # an argument name. If an argument is specified multiple times
            # the last definition overrides the former ones

            set e [::lassign $e field_name field_type]

            # the 'vars' dictionary field stores the
            # order of form fields in which they are processed
            # (in general this order is destroyed by the Tcl's hash
            # tables algorithm)

            dict with form_list $form_name {::lappend vars $field_name}

            if {$field_type == ""} {
                set field_type string
            }

            dict set form_definitions $form_name    $field_name \
                        [list   type                $field_type \
                                bounds              0           \
                                constrain           0           \
                                validator           [namespace current]::validate_string \
                                force_quote         0           \
                                field_validation    FB_OK]

            dict with form_definitions $form_name $field_name {

                switch $field_type {
                    integer {
                        set validator [namespace current]::validate_integer
                    }
                    unsigned {
                        set validator [namespace current]::validate_unsigned
                    }
                    email {
                        set validator [namespace current]::validate_email
                    }
                    string -
                    default {
                        set validator [namespace current]::validate_string
                    }
                }

                # 

                while {[llength $e] > 0} {
                    set e [::lassign $e field_spec]
                    
                    switch $field_spec {
                        check_routine -
                        validator {
                            set e [::lassign $e validator]
                        }
                        length -
                        bounds {
                            set e [::lassign $e bounds]
                            constrain_bounds $field_type bounds
                        }
                        constrain {
                            set constrain 1
                        }
                        noconstrain {
                            set constrain 0
                        }
                        quote {
                            set force_quote 1
                        }
                    }

                }

            }
        }
        return [namespace current]::$form_name 
    }

    namespace export *
    namespace ensemble create
}

package provide formbroker 0.1
