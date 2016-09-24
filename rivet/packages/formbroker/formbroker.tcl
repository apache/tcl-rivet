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

     

    #
    # force_quote_response_strings - sanitize and pg_quote all the specified strings in the array
    #

    proc force_quote_response_strings {_response args} {
        upvar $_response response

        force_sanitize_response_strings response {*}$args

        foreach var $args {
            set response($var) [pg_quote $response($var)]
        }

    }

     

    #
    # force_quote_response_unfilteredstrings - rewrite named response
    # elements pg_quoted
    #

    proc force_quote_response_unfilteredstrings {_response args} {
        upvar $_response response

        require_response_vars response {*}$args

        foreach var $args {
            set response($var) [pg_quote $response($var)]
        }

    }

    namespace export *
    namespace ensemble create
}
