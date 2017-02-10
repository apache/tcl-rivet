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

    # -- validate_integer
    #
    # integer validation checks whether
    #
    # 1- the representation *is* an integer
    # 2- if buonds exist the value must be between [-bound,bound] 
    # 3- if the bounds is a list of 2 elements the value must 
    #    be between them
    #
    # If needed the variable is constrained within the bounds.
    # 

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

    proc validate_boolean {_var_d} {
        upvar $_var_d var_d

        dict with var_d {
            if {[regexp -nocase {Y|N|0|1} $var]} {
                return FB_OK
            } else {
                return FB_INVALID_BOOLEAN
            }
        }
    }


    proc validate_variable_representation {_var_d} {
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


    proc validate_var {form_name var_name var_value {force_quoting "-noforcequote"}} {
        variable form_definitions
        upvar    $var_value value

        set force_quote_var [string match $force_quoting "-forcequote"]

        set variable_d [dict get $form_definitions $form_name $var_name]
        dict set variable_d var $value
        set valid [validate_variable_representation variable_d]

        set value [dict get $variable_d var] 
        if {[dict get $variable_d force_quote] || $force_quote_var} {
            set value  [$string_quote $value]
        }
        return $valid
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

    # -- form_definition
    #
    # currently this call returns the dictionary
    # of form field definitions. It's not meant to be
    # used in regular development. It's supposed to be
    # private to the FormBroker package
    # and it may go away with future developments or
    # change its interface and returned value

    proc form_definition {form_name} {
        variable form_definitions

        return [dict get $form_definitions $form_name]
    }

    # -- validation_error
    #
    # returns the result of the last validation
    # operation called on for this form.
    #


    proc validation_error {form_name} {
        variable form_list

        return [dict get $form_list $form_name form_validation]
    }


    # -- failing
    #
    # returns a list of variable-status pairs for each
    # field in a form that did not validate
    #

    proc failing {form_name} {
        set res {}
        dict for {field field_d} [form_definition $form_name] {
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

    # --require_response_vars 
    # 
    # error if any of the specified are not in the response
    #

    proc require_response_vars {form_name _response} {
        upvar $_response response
        variable form_definitions

        set missing_vars 0
        dict for {var variable_d} [dict get $form_definitions $form_name] {
            if {![info exists response($var)]} { 
                dict with form_definitions $form_name $var {

                    # if the variable was not in the response
                    # but a default was set then we copy this
                    # value in the variable descriptor and
                    # the response array as well

                    if {[info exists default]} {
                        set response($var)  $default
                        set var             $default
                    } else {
                        set field_validation    MISSING_VAR
                        set missing_vars        1
                    }

                }
            }
        }

        if {$missing_vars} {
            response_security_error MISSING_VAR \
                "var $var not present in $_response"
        }


    }

    # -- validate
    #
    # 

    proc validate { form_name args } {
        variable form_definitions
        variable form_list
        variable string_quote

        set force_quote_vars 0
        set arguments        $args
        if {[llength $arguments] == 0} { 
            error "missing required arguments" 
        } elseif {[llength $arguments] > 3} {
            error "error calling validate, usage: validate ?-forcequote? response ?copy_response?"
        }

        while {[llength $arguments]} {
            
            set arguments [::lassign $arguments a]
            if {$a == "-forcequote"} {
                set force_quote_vars 1
            } elseif {![array exists response]} {
                upvar $a response
            } else {
                upvar $a filtered_response
                array set filtered_response {}
            }

        }

        if {![array exists response]} {
            error "error calling validate, usage: validate ?-forcequote? response ?copy_response?"
        }

        # we now go ahead validating the response variables

        set form_valid true

        set vars_to_validate [dict get $form_list $form_name vars]
        if {[catch {
                require_response_vars $form_name response
            } er eopts]} {

            #puts "$er $eopts"
            dict set form_list $form_name form_validation FB_MISSING_VARS
            return false

        }

        # field validation

        dict with form_list $form_name {
            set form_validation     FB_OK
        }

        set form_d [dict get $form_definitions $form_name]
        #puts "form_d: $form_d"

        array unset response_a
        dict for {var variable_d} $form_d {

            dict set variable_d var $response($var)
            if {[validate_variable_representation variable_d] == 0} {

                dict set form_list $form_name form_validation FB_VALIDATION_ERROR
                set form_valid false

            } else {

                # in case it was constrained we write the value back
                # into the response array

                if {[dict get $variable_d constrain]} { 
                    set response_a($var) [dict get $variable_d var] 
                } else {
                    set response_a($var) $response($var)
                }

                if {[dict get $variable_d force_quote] || $force_quote_vars} {

                    set response_a($var)  [$string_quote [dict get $variable_d var]]

                }
            }
            dict set form_definitions $form_name $var $variable_d
            #puts "validated $var -> $variable_d"

        }

        # if 'validate' has been called with a filtered_response array
        # we clean it up and proceed copying the variable values into it

        if {[array exists filtered_response]} {
            array unset filtered_response
            array set filtered_response [array get response_a]
        } else {
            array set response [array get response_a] 
        }
        return $form_valid
    }

    # -- response 
    #
    #

    proc response {form_name {resp_a response}} {
        upvar $resp_a response
        variable form_definitions

        dict for {var_name var_d} [dict get $form_definitions $form_name] {
            catch {unset var}
            catch {unset default}        

            dict with var_d {

                if {[info exists var]} {
                    set response($var_name) $var
                } elseif {[info exists default]} {
                    set response($var_name) $default
                } 

            }

        }
    }

    # -- reset
    #
    #

    proc reset {form_name} {
        variable form_definitions
        variable form_list
        
        dict set form_list $form_name form_validation FB_OK
        dict for {var_name var_d} [dict get $form_definitions $form_name] {
            catch {dict unset var_d $var_name var}
        }
    }

    # -- destroy
    #
    # this method is designed to be called
    # by an 'trace unset' event on the variable
    # keeping the form description object. 
    #

    proc destroy {form_name args} {
        variable form_definitions
        variable form_list

        dict unset form_definitions $form_name
        dict unset form_list        $form_name
        namespace delete            ::FormBroker::${form_name}
        #puts "destroy of $form_name finished"
    }

    # -- create
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

    proc create {args} {
        variable form_definitions
        variable form_list
        variable form_count
        variable string_quote

        set form_name "form${form_count}"
        incr form_count

        catch { namespace delete $form_name }
        namespace eval $form_name {

            foreach cmd { validate failing      \
                          form_definition       \
                          result validate_var   \
                          destroy validation_error \
                          response reset } {
                lappend cmdmap $cmd [list [namespace parent] $cmd [namespace tail [namespace current]]]
            }

            namespace ensemble create -map [dict create {*}$cmdmap]
            unset cmdmap
            unset cmd

        }

        dict set form_definitions $form_name [dict create]
        dict set form_list        $form_name [dict create vars            {}     \
                                                          form_validation FB_OK  \
                                                          failing         {}     \
                                                          default         ""     \
                                                          quoting         $string_quote]

        while {[llength $args]} {

            set args [::lassign $args e]

            if {$e == "-quoting"} {

                dict with form_list $form_name {
                    set args [::lassign $args quoting]

                    if {[uplevel [list info proc $quoting]] == ""} {
                        error [list RIVET INVALID_QUOTING_PROC \
                                          "Non existing quoting proc '$quoting'"]
                    }
                    set string_quote $quoting
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
            # (in general this order would be destroyed by the Tcl's hash
            # tables)

            dict with form_list $form_name {::lappend vars $field_name}

            # this test would handle the case of the most simple possible
            # variable definition (just the variable name)

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
                    boolean {
                        set validator [namespace current]::validate_boolean
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
                        default {
                            set e [::lassign $e default]

                            # we must not assume the variable 'default'
                            # exists in the dictionary because we 
                            # set it only in this code branch

                            dict set form_definitions $form_name $field_name default $default
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

                # let's check for possible inconsitencies between
                # data type and default value. For this purpose
                # we create a copy of the variable dictionary 
                # representation then we call the validator on it

                set variable_d [dict get $form_definitions $form_name $field_name]
                dict set variable_d var $default
                if {[$validator variable_d] != "FB_OK"} {
                    dict unset form_definitions $form_name $field_name default
                }
            }
        }
        return [namespace current]::$form_name 
    }

    proc creategc {varname args} {
        set formv [uplevel [list set $varname [::FormBroker::create {*}$args]]]
        uplevel [list trace add variable $varname unset \
                [list [namespace current]::destroy [namespace tail $formv]]]

        return $formv
    }

    namespace export *
    namespace ensemble create
}

package provide formbroker 1.0
