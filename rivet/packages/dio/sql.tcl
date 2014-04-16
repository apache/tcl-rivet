# sql.tcl -- SQL code generator

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

# This class provides a way to abstract to some extent the
# SQL code generation. It's supposed to provide a bridge to
# different implementation in various backends for specific
# functionalities
#
# $Id$

package require Itcl

### 
catch { ::itcl::delete class ::DIO::Sql }
###
namespace eval ::DIO {

    proc generator {backend} {
        
    }

    ::itcl::class Sql {

        public variable backend
        public variable what
        public variable table

        constructor { backend } {

        }

        private method where_clause {where_arguments}

        public method build_select_query {table row_d}
        public method quote {field_value}

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

    ::itcl::class Result {

        public variable resultid    ""
        public variable fields      ""
        public variable rowid       0
        public variable numrows     0
        public variable error       0
        public variable errorcode   0
        public variable errorinfo   ""
        public variable autocache   1

        protected variable cached           0
        protected variable cacheSize        0
        protected variable cacheArray

        constructor {args} {
            eval configure $args
        }

        destructor { }

        method destroy {} {
            ::itcl::delete object $this
        }
        #
        # seek - set the current row ID (our internal row cursor, if you will)
        # to the specified row ID
        #
        method seek {newrowid} { set rowid $newrowid }
        protected method configure_variable {varName string} 
        protected method lassign_array {list arrayName args} 
        public    method cache {{size "all"}}
        public    method forall {type varName body} 
        public    method next {type {varName ""}} 
        public    method resultid {{string ""}} { return [configure_variable resultid $string] }
        public    method fields {{string ""}} { return [configure_variable fields $string] }
        public    method rowid {{string ""}} { return [configure_variable rowid $string] }
        public    method numrows {{string ""}} { return [configure_variable numrows $string] }
        public    method error {{string ""}} { return [configure_variable error $string] }
        public    method errorcode {{string ""}} { return [configure_variable errorcode $string] }
        public    method errorinfo {{string ""}} { return [configure_variable errorinfo $string] }
        public    method autocache {{string ""}} { return [configure_variable autocache $string] }
    }


    #
    # configure_variable - given a variable name and a string, if the
    # string is empty return the variable name, otherwise set the
    # variable to the strings
    #
    ::itcl::body Result::configure_variable {varName string} {
        if {[lempty $string]} { return [cget -$varName] }
        $this configure -$varName $string
    }
    #
    # lassign_array - given a list, an array name, and a variable number
    # of arguments consisting of variable names, assign each element in
    # the list, in turn, to elements corresponding to the variable
    # arguments, into the named array.  From TclX.
    #
    ::itcl::body Result::lassign_array {list arrayName args} {
        upvar 1 $arrayName array
        foreach elem $list field $args {
            set array($field) $elem
        }
    }

    ::itcl::body Result::cache {{size "all"}} {

        set cacheSize $size
        if {$size == "all"} { set cacheSize $numrows }

        ## Delete the previous cache array.
        catch {unset cacheArray}

        set autostatus $autocache
        set currrow    $rowid
        set autocache 1
        seek 0
        set i 0
        while {[$this next -list list]} {
            if {[incr i] >= $cacheSize} { break }
        }
        set autocache $autostatus
        seek $currrow
        set cached 1

    }


    #
    # forall -- walk the result object, executing the code body over it
    #
    ::itcl::body Result::forall {type varName body} {
        upvar 1 $varName $varName
        set currrow $rowid
        seek 0
        while {[next $type $varName]} {
            uplevel 1 $body
        }
        set rowid $currrow
        return
    }

    ::itcl::body Result::next {type {varName ""}} {
        set return 1
        if {![lempty $varName]} {
            upvar 1 $varName var
            set return 0
        }

        catch {unset var}

        set list ""
        ## If we have a cached result for this row, use it.
        if {[info exists cacheArray($rowid)]} {
            set list $cacheArray($rowid)
        } else {
            set list [$this nextrow]
            if {[lempty $list]} {
                if {$return} { return }
                set var ""
                return 0
            }
            if {$autocache} { set cacheArray($rowid) $list }
        }

        incr rowid

        switch -- $type {
            "-list" {
                if {$return} {
                    return $list
                } else {
                    set var $list
                }
            }
            "-array" {
                if {$return} {
                    foreach field $fields elem $list {
                        lappend var $field $elem
                    }
                    return $var
                } else {
                    eval lassign_array [list $list] var $fields
                }
            }
            "-keyvalue" {
                foreach field $fields elem $list {
                    lappend var -$field $elem
                }
                if {$return} { return $var }
            }

            default {
                incr rowid -1
                return -code error \
                    "In-valid type: must be -list, -array or -keyvalue"
            }
        }
        return [expr [lempty $list] == 0]
    }



}




