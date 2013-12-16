# sql.tcl -- SQL code generator
#
# This class provides a way to abstract to some extent the
# SQL code generation. It's supposed to provide a bridge to
# different implementation in various backends for specific
# functionalities

# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#       http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $Id: $

package require Itcl

namespace eval ::DIO {

    proc generator {backend} {
        
    }

    ::itcl::class Sql {

        constructor { backend } {

        }

        public method build_select_query {table args}
        public method quote {field_value}

        protected method fieldValue {table_name field_name val} {
            return "'[quote $val]'"
        }

        public variable backend
    }
    
    #
    # quote - given a string, return the same string with any single
    #  quote characters preceded by a backslash
    #
    ::itcl::body Sql::quote {field_value} {
        regsub -all {'} $field_value {\'} field_value
        return $field_value
    }

    # build_select_query - build a select query based on given arguments,
    # which can include a table name, a select statement, switches to
    # turn on boolean AND or OR processing, and possibly
    # some key-value pairs that cause the where clause to be
    # generated accordingly

    ::itcl::body Sql::build_select_query {table args} {

        set bool AND
        set first 1
        set req ""
        set myTable $table
        set what "*"

        # for each argument passed us...
        # (we go by integers because we mess with the index based on
        #  what we find)

        for {set i 0} {$i < [llength $args]} {incr i} {

            # fetch the argument we're currently processing
            set elem [lindex $args $i]

            switch -- [::string tolower $elem] {
                "-and" { 
                    # -and -- switch to AND-style processing
                    set bool AND 
                }

                "-or"  { 
                    # -or -- switch to OR-style processing
                    set bool OR 
                }

                "-table" { 
                    # -table -- identify which table the query is about
                    set myTable [lindex $args [incr i]] 
                }

                "-select" {
                    # -select - 
                    set what [lindex $args [incr i]]
                }

                default {
                    # it wasn't -and, -or, -table, or -select...

                    # if the first character of the element is a dash,
                    # it's a field name and a value

                    if {[::string index $elem 0] == "-"} {

                        set field [::string range $elem 1 end]
                        set elem  [lindex $args [incr i]]

                        # if it's the first field being processed, append
                        # WHERE to the SQL request we're generating
                        if {$first} {
                            append req " WHERE"
                            set first 0
                        } else {
                            # it's not the first variable in the comparison
                            # expression, so append the boolean state, either
                            # AND or OR
                            append req " $bool"
                        }

                        # convert any asterisks to percent signs in the
                        # value field
                        regsub -all {\*} $elem {%} elem

                        # if there is a percent sign in the value
                        # field now (having been there originally or
                        # mapped in there a moment ago),  the SQL aspect 
                        # is appended with a "field LIKE value"

                        if {[::string first {%} $elem] != -1} {
                            append req " $field LIKE [fieldValue $myTable $field $elem]"
                        } elseif {[::string equal $elem "-null"]} {
                            append req " $field IS NULL"
                        } elseif {[::string equal $elem "-notnull"]} {
                            append req " $field IS NOT NULL"
                        } elseif {[regexp {^([<>]) *([0-9.]*)$} $elem _ fn val]} {
                            # value starts with <, or >, then space, 
                            # and a something
                            append req " $field$fn$val"
                        } elseif {[regexp {^([<>]=) *([0-9.]*)$} $elem _ fn val]} {
                            # value starts with <= or >=, space, and something.
                            append req " $field$fn$val"
                        } else {
                            # otherwise it's a straight key=value comparison
                            append req " $field=[fieldValue $myTable $field $elem]"
                        }

                        continue
                    }
                    append req " $elem"
                }
            }
        }
        return "select $what from $myTable $req"
    }
}




