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
    variable form_database [dict create]
    variable field_database [dict create]
    variable string_quote   force_quote
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
            set response($var) [string_quote $response($var)]
        }

    }
    
    # -- base validators
    
    proc validate_string {_var_d {costrain 0}} {
        upvar $_var_d var_d

        dict with var_d {
            if {$length > 0} {
                set var [string range $var 0 $length]
            }
        }
    }

    proc validate_integer {_var_d {costrain 0}} {
        upvar $_var_d var_d

        if {![string is integer $var]}} {
            return NOT_INTEGER
        }

        set valid FB_OK
        dict with var_d {
            if {[llength $limit] == 2} {
                lappend $limit min_v max_v

                if {($var > $max_v) || ($var < $min_v)} {

                    set valid FB_OUT_OF_LIMITS

                }

                if {$costrain} {
                    set var [expr min($var,$max_v)]
                    set var [expr max($var,$min_v)]
                    set valid FB_OK
                }

            } elseif {([llength $limit] == 1) && ($limit > 0)} {

                if {(abs($var) > $limit)} {

                    set valid FB_OUT_OF_LIMITS

                }

                if {$costrain} {
                    set var [expr min($limit,$var)]
                    set var [expr -max($limit,$var)]
                    set valid FB_OK
                }
            }
        }
        return $valid
    }

    proc validate_unsigned {_var_d {costrain 0}} {
        upvar $_var_d var_d

        dict with var_d {
            if {[llength $limit] == 2} {
                lappend $limit min_v max_v

                if {($var > $max_v) || ($var < $min_v)} {

                    set valid FB_OUT_OF_LIMITS

                }

                if {$costrain} {

                    set var [expr min($var,$max_v)]
                    set var [expr max($var,$min_v)]
                    set valid FB_OK

                }
            } elseif {([llength $limit] == 1) && \
                      ($limit > 0)} {

                if {($var > $limit) || ($var < 0)} {

                    set valid FB_OUT_OF_LIMITS

                }

                if {$costrain} {
                    set var [expr max(0,$var)]
                    set var [expr min($limit,$var)]
                    set valid FB_OK
                }

            } else {

                if {$costrain} {
                    set var [expr max(0,$var)]
                    set valid FB_OK
                }

            }
        }
        return $valid
    }

    proc validate_email {_var_d {costrain 0}} {
        upvar $_var_d var_d

        dict with var_d {
            if {[regexp -nocase {[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}} $var]} {
                return FB_OK
            } else {
                return FB_INVALID_EMAIL
            }
        }
    }

    proc validate_form_var {_var_d costrain} {
        upvar $_var_d var_d
        variable form_database

        dict with var_d {

            if {[info commands $validator] == ""} {
                set validator ::FormBroker::validate_string
            }

            set field_validation [$validator var_d $costrain]


        }
        return [string match $field_validation FB_OK]
    }


    # -- costrain_limits
    #
    # During the form creation stage this method is called
    # to correct possible inconsistencies with a field limit 
    # definition
    #

    proc constrain_limits {field_type _limit {costrain 0}} {
        upvar $_limit limit

        switch $field_type {
            integer {
                if {[llength $limit] == 1} {

                    set limit [list [expr -abs($limit)] [expr abs($limit)]]

                } elseif {[llength $limit] > 1} {
                    lassign $limit l1 l2

                    set limit [list [expr min($l1,$l2)] [expr max($l1,$l2)]]
                } else {
                    set limit 0
                }
            }
            unsigned {
                if {[llength $limit] == 1} {

                    set limit [list 0 [expr abs($limit)]]

                } elseif {[llength $limit] > 1} {

                    lassign $limit l1 l2
                    if {$l1 < 0} { set l1 0 }
                    if {$l2 < 0} { set l2 0 }

                    set limit [list [expr min($l1,$l2)] [expr max($l1,$l2)]]
                } else {
                    set limit 0
                }
            }
        }
    }

    proc validate { form_name _response} {
        upvar $_response response
        variable form_database
        variable field_database

        set form_valid true
        set fd [dict get $form_database $form_name]
        dict set field_database $form_name failing {}
        dict with fd {

            set vars_to_validate [dict get $field_database $form_name vars]
            if {[catch {
                    require_response_vars response {*}$vars_to_validate
                } er eopts]} {

                #puts "$er $eopts"
                dict set field_database $form_name form_validation $er
                return false

            }

            # field validation

            set costrain [dict get $field_database costrain]
            foreach var $vars_to_validate {
                set variable_d [dict get $form_database $form_name $var]
                dict set variable_d var $response($var)
                if {[validate_form_var variable_d $costrain] == 0} {
                    dict set field_database $form_name form_validation FB_VALIDATION_ERROR
                    dict lappend field_database $form_name failing $var
                    set form_valid false
                }
            }

        }
        dict set form_database $form_name $fd
        return $form_valid
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
    #  - limit <value>
    #  - limit [low high]
    #  - check_routine [validation routine]
    #  - length [max length]
    #

    proc initialize {form_name args} {
        variable form_database
        variable field_database

        catch { namespace delete $form_name }
        namespace eval $form_name { 
            namespace ensemble create -map [dict create     \
                                                validate [list [namespace parent] validate \
                                                          [namespace tail [namespace current]]]]
        }

        dict set form_database $form_name [dict create]

        # arguments processing. The command accept a flag -constrain|-noconstrain
        # as first argument in variable length argument list
        
        set first [lindex $args 0]
        if {$first == "-constrain"} {
            dict set field_database $form_name constrain 1

            set args [lrange $args 1 end]
        } elseif {$first == "-nocostrain"} {

            dict set field_database $form_name constrain 0
            set args [lrange $args 1 end]

        } else {

            dict set field_database $form_name constrain 0

        }


        foreach e $args {
            set e [::lassign $e field_name field_type]

            # the 'order' dictionary fields stores the
            # order of form fields in which they are processed
            # (in general it's destroyed by the internal hash
            # tables algoritm).

            dict lappend field_database $form_name vars $field_name

            if {$field_type == ""} {
                set field_type string
            }

            dict set form_database $form_name $field_name \
                [list   type            $field_type \
                        limit           0           \
                        validator       [namespace current]::validate_string \
                        field_validation FB_OK       \
                        length          0]

            dict with form_database $form_name $field_name {

                switch $field_type {
                    integer -
                    unsigned {
                        set validator [namespace current]::validate_integer
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
                        limit {
                            set e [::lassign $e limit]
                            constrain_limits $field_type limit
                        }
                        length {
                            set e [::lassign $e length]
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

package provide formbroker
