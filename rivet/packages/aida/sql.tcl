# sql.tcl -- SQL code generator

# Copyright 2024 The Apache Tcl Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This class provides a way to abstract to some extent the
# SQL code generation. It's supposed to provide a bridge to
# different implementation in various backends for specific
# functionalities
#

package require Itcl

### 
catch { ::itcl::delete class ::DIO::Sql }
###
namespace eval ::Aida {

    proc generator {backend} {
        
    }

    ::itcl::class Sql {

        public variable backend
        public variable what
        public variable table

        constructor {backend} {

        }

        private method where_clause {where_arguments}

        public method build_select_query {table row_d}
        public method escape_quotes {field_value}

        # DEPRECATED. Kept for compatibility, but it might go away
        public method quote {field_value} { return [$this escape_quotes $field_value] }

        protected method field_value {table_name field_name val} {
            return "'[quote $val]'"
        }

        public method build_insert_query {table row_d}
        public method build_update_query {table row_d}

    }
    
    # -- build_insert_query
    #
    #

    ::itcl::body Sql::build_insert_query {table row_d} {

        set vars [dict keys $row_d]
        foreach field $vars {
        
            lappend vals [$this field_value $table $field [dict get $row_d $field]]

        }

        return "INSERT INTO $table ([join $vars {,}]) VALUES ([join $vals {,}])"
    }

    # -- build_update_query
    #
    #

    ::itcl::body Sql::build_update_query {table row_d} {
      
        foreach field [dict keys $row_d] {
            lappend rowfields "$field=[field_value $table $field [dict get $row_d $field]]"
        }

        return "UPDATE $table SET [join $rowfields {,}]"
    }


    # build_where_clause 
    #
    #
    ::itcl::body Sql::where_clause {where_expr} {

        set sql ""
        for {set i 0} {$i < [llength [dict keys $where_expr]]} {incr i} {

            set d [dict get $where_expr $i]

            set col [dict get $d column]
            set op  [dict get $d operator]
            if {$i > 0} {

                append sql " [dict get $d logical]"

            }
            switch $op {

                "eq" {
                    set sqlop "="
                }
                "ne" {
                    set sqlop "!="
                }
                "lt" {
                    set sqlop "<"
                }
                "gt" {
                    set sqlop ">"
                }
                "le" {
                    set sqlop "<="
                }
                "ge" {
                    set sqlop ">="
                }
                "notnull" {

                    append sql " $col IS NOT NULL"
                    continue

                }
                "null" {
                    append sql " $col IS NULL"
                    continue

                }

            }

            set predicate [dict get $d predicate]
            if {[::string first {%} $predicate] != -1} {
                append sql " $col LIKE [$this field_value $table $col [[string range $predicate 1 end]]"
            } else {
                append sql " $col$sqlop[$this field_value $table $col $predicate]"
            }
        }

        return $sql
    }    


    #
    # escape_quotes - given a string, return the same string with any single
    #                 quote characters preceded by a backslash
    #
    ::itcl::body Sql::escape_quotes {field_value} {
        regsub -all {'} $field_value {\'} field_value
        return $field_value
    }

    # build_select_query - build a select query based on given arguments,
    # which can include a table name, a select statement, switches to
    # turn on boolean AND or OR processing, and possibly
    # some key-value pairs that cause the where clause to be
    # generated accordingly

    ::itcl::body Sql::build_select_query {args} {

        set bool    AND
        set first   1
        set req     ""
        set table   $from_table
        set what    "*"

        set parser_st   state0
        set condition_count 0
        set where_expr [dict create]

        # for each argument passed us...
        # (we go by integers because we mess with the index depending on
        # what we find)
        #puts "args: $args"
        for {set i 0} {$i < [llength $args]} {incr i} {

            # fetch the argument we're currently processing
            set elem [lindex $args $i]
            # puts "cycle: $i (elem: $elem, status: $parser_st, first: $first)"
            
            switch $parser_st {
                state0 {

                    switch -- [::string tolower $elem] {

                        # -table and -select don't drive the parser state machine
                        # and whatever they have as arguments on the command
                        # line they're set 

                        "-table" { 
                            # -table -- identify which table the query is about
                            set table [lindex $args [incr i]]
                        }
                        "-select" {
                            # -select - 
                            set what [lindex $args [incr i]]
                        }
                        "-or" - 
                        "-and" {
                            if {$first} {
                                return -code error "$elem can not be the first element of a where clause"
                            } else {
                                incr condition_count
                                dict set where_expr $condition_count logical [string range $elem 1 end] 
                                set parser_st where_op
                            }
                        }    
                        default {
                            
                            if {[::string index $elem 0] == "-"} {
                                if {!$first} { 
                                    incr condition_count 
                                }
                                dict set where_expr $condition_count column [string range $elem 1 end]
                                set first 0
                                set parser_st where_op
                            } else {

                                return -code error "Error: expected -<column_name>"
                            }

                        }

                    }

                }

                where_op {

                    switch -- [string tolower $elem] {

                        "-lt" -
                        "-gt" -
                        "-ne" -
                        "-eq" {

                            dict set where_expr $condition_count operator [string range $elem 1 end]
                            set parser_st cond_predicate

                        }
                         
                        "-null" -
                        "-notnull" {

                            dict set where_expr $condition_count operator [string range $elem 1 end]
                            set parser_st state0

                        }

                        default {
                            if {[::string index $elem 0] == "-"} {
                                dict set where_expr $condition_count column [string range $elem 1 end]
                            } else {
                                dict set where_expr $condition_count operator "eq"
                                dict set where_expr $condition_count predicate $elem 
                                set parser_st state0
                            }
                        }

                    }
                }

                cond_predicate {
                    
                    switch -- [string tolower $elem] {

                        "-expr" {
                            dict set where_expr $condition_count predicate [lindex $args [incr i]] 
                        }
                        default  {

                            # convert any asterisks to percent signs in the
                            # value field
                            regsub -all {\*} $elem {%} elem

                            dict set where_expr $condition_count predicate $elem

                        }

                    }
                    set parser_st state0
                }
                default {
                    return -code error "invalid parser status"
                }
            }
        }

        set sql "SELECT $what from $table WHERE[$this where_clause $where_expr]"

        return $sql
    }




}




