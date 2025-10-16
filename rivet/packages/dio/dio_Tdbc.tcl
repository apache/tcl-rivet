# tdbc.tcl -- connector for tdbc, the Tcl database abstraction layer
#
# Copyright 2024 The Apache Software Foundation
#
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations
#    under the License.

package require tdbc
package require DIO      1.2.3
package provide dio_Tdbc 1.2.4

namespace eval DIO {
    ::itcl::class Tdbc {
        inherit Database

        private common   connector_n    0
        private variable connector
        private variable connector_name
        private variable tdbc_connector
        private variable tdbc_arguments [list -encoding     \
                                              -isolation    \
                                              -readonly     \
                                              -timeout]

        constructor {interface_name args} {eval configure -interface $interface_name $args} {
            set connector_n 0
            set connector   ""

            # I should check this one: we only accept connector
            # interfaces that map into one of tdbc's supported drivers
            #puts "tdbc interface: $interface_name"

            set connector_name [::string tolower $interface_name]
            switch $connector_name {
                "oracle" {
                    set connector_name "odbc"
                } 
                "postgresql" {
                    set connector_name "postgres"
                }
                "sqlite" {
                    set connector_name "sqlite3"
                }
                "mariadb" {
                    set connector_name "mysql"
                }
                "sqlite3" -
                "odbc" -
                "postgres" -
                "mysql" {
                    # OK
                }
                default {
                    return -code error -errorcode invalid_tdbc_driver "Invalid TDBC driver '$connector_name'"
                }
            }

            set interface $connector_name
            $this set_field_formatter ::DIO::formatters::[::string totitle $interface]
            set tdbc_connector "tdbc::${interface}"

            uplevel #0 package require ${tdbc_connector}
        }

        destructor { }

        private method check_connector {} {
            if {$connector == ""} { open }
        }

        public method tdbc_connector {} { return $connector }

        public method open {}  {
            set connector_cmd "${tdbc_connector}::connection create ${tdbc_connector}#$connector_n"
            if {$user != ""} { lappend connector_cmd -user      $user }
            if {$db   != ""} {
                if {$connector_name == "sqlite3"} {
                    lappend connector_cmd $db
                } else {
                    lappend connector_cmd -db $db
                }
            }
            if {$pass != ""} { lappend connector_cmd -password  $pass }
            if {$port != ""} { lappend connector_cmd -port      $port }
            if {$host != ""} { lappend connector_cmd -host      $host }

            if {$clientargs != ""} { lappend connector_cmd {*}$clientargs }

            #puts "evaluating $connector_cmd"
            set connector [eval $connector_cmd]
            incr connector_n
        }

        public method close {} {
            if {$connector == ""} { return }
            $connector close
            set connector ""
        }

        protected method handle_client_arguments {cargs} {
            set clientargs {}
            lmap {k v} $cargs {
                if {[lsearch $k $tdbc_arguments] >= 0} {
                    lappend clientargs $k $v
                }
            }
        }

        public method tdbc_exec {sql sql_values_d {results_v ""}} {

            $this check_connector
            if {[::string index $sql end] == ";"} {set sql [::string range $sql 0 end-1]}

            set is_select [regexp -nocase {^\(*\s*select\s+} $sql]
            set tdbc_statement [$connector prepare $sql]

            #puts "--> $sql_values"
            if {[catch {set tdbc_result [$tdbc_statement execute $sql_values_d]} e errorinfo]} {
                $tdbc_statement close
                return -code error -errorinfo [::list $errorinfo]
            } else {

                # we must store also the TDBC SQL statement as it owns
                # the TDBC results set represented by tdbc_result. Closing
                # a tdbc::statement closes also any active tdbc::resultset
                # owned by it

                set result_obj [$this result TDBC -resultid   $tdbc_result      \
                                                  -statement  $tdbc_statement   \
                                                  -isselect   $is_select        \
                                                  -fields     [::list [$tdbc_result columns]]] 
            }

            # this doesn't work on postgres, you've got to use cmdRows,
            # we need to figure out what to do with this

            set numrows [$result_obj numrows]
            if {$is_select && ($numrows > 0) && ($results_v != "")} {
                upvar 1 $results_v results_o
                set results_o $result_obj
            } else {
                $result_obj destroy
            }
            return $numrows

        }


        # delete -
        #
        #

        method delete {key args} {
            table_check $args

            set sql "DELETE FROM $myTable"
            set sql_values_d [dict create]
            if {[$special_fields_formatter has_special_fields $table]} {
                append sql [build_key_where_clause $myKeyfield $key]
            } else {
                set where_key_value_pairs [lmap field $myKeyfield {
                    set v "${field}=:${field}"
                }]
                set sql "$sql WHERE [join $where_key_value_pairs { AND }]"
                foreach field $myKeyfield k $key { dict set sql_values_d $field $k }
            }

            #puts $sql

            return [$this tdbc_exec $sql $sql_values_d]
        }


        #
        # update - reimplementation of the ::DIO::Database::update
        # method that is supposed to exploit the named arguments
        # feature of TDBC. 
        #
        method update {arrayName args} {
            upvar 1 $arrayName row_a
            $this table_check $args

            # myTable is implicitly set by table_check
            
            $this configure -table $myTable
            set key [makekey row_a $myKeyfield]

            #puts "-> table: $table"
            #puts "-> key: $key"
            #puts "-> keyfield: $myKeyfield"

            set sql_values_d [dict create {*}[::array get row_a]] 
            set fields     [::array names row_a]
            if {[$special_fields_formatter has_special_fields $table]} {

                # the special fields formatter is fundamentally incompatible with
                # TDBC's named arguments mechanism. We resort to the superclass method
                # where a literal SQL statement is built

                set sql     [$this build_update_query row_a $fields $table]
                append sql  [$this build_key_where_clause $myKeyfield $key]

            } else {

                set set_key_value_pairs [lmap field $fields {
                    set v "${field}=:${field}"
                }]
                set where_key_value_pairs [lmap field $myKeyfield {
                    set v "${field}=:${field}"
                }]

                ::lappend where_key_value_pairs "1 = 1"
                set sql [join [::list "UPDATE $table" \
                                      "SET   [join $set_key_value_pairs {, }]" \
                                      "WHERE [join $where_key_value_pairs { AND }]"] " "]

            }

            #puts $sql
            return [$this tdbc_exec $sql $sql_values_d]
        }

        #
        # insert - 
        #
        # overriding method 'insert' as in case of registered special fields
        # we have to give up with the idea of using the named arguments approach
        #
        method insert {table arrayName} {
            upvar 1 $arrayName row_a

            set sql_values_d [dict create {*}[::array get row_a]]
            set fields     [::array names row_a]
            if {[$special_fields_formatter has_special_fields $table]} {
                set sql [build_insert_query row_a [::array names row_a] $table]
            } else {
                set values [lmap field $fields { set v ":${field}" }]

                set sql "INSERT INTO $table ([join $fields {,}]) VALUES ([join $values {,}])"
            }

            #puts $sql
            #puts $sql_values_d
            return [$this tdbc_exec $sql $sql_values_d]

        }

        # fetch
        #
        #

        method fetch {key arrayName args} {
            upvar 1 $arrayName row_a
            table_check $args
            key_check $myKeyfield $key

            $this configure -table $myTable
            set sql_values_d [dict create]
            set sql "SELECT * FROM $myTable"
            if {[$special_fields_formatter has_special_fields $table]} {
                append sql [build_key_where_clause $myKeyfield $key]
            } else {
                set where_key_value_pairs [lmap field $myKeyfield {
                    set v "${field}=:${field}"
                }]
                set sql "$sql WHERE [join $where_key_value_pairs { AND }]"
                foreach field $myKeyfield k $key { 
                    if {$field == ""} { continue }
                    dict set sql_values_d $field $k
                }
            }

            #puts $sql
            set numrows [$this tdbc_exec $sql $sql_values_d result_o]
            if {$numrows > 0} {
                $result_o next -array row_a
                $result_o destroy
            }
            return $numrows
        }

        #
        # exec
        #
        #

        public method exec {sql} {
            $this check_connector

            # tdbc doesn't like ';' at the end of a SQL statement

            if {[::string index $sql end] == ";"} {set sql [::string range $sql 0 end-1]}
            set is_select [regexp -nocase {^\(*\s*select\s+} $sql]

            set tdbc_statement [uplevel 1 $connector prepare [::list $sql]]

            # errorinfo is a public variable of the parent class Database.
            # Not a good object design practice

            if {[catch {set tdbc_result [uplevel 1 $tdbc_statement execute]} errorinfo]} {
                set result_obj [$this result TDBC -error 1 -errorinfo [::list $errorinfo] -isselect false]
            } else {

                # we must store also the TDBC SQL statement as it owns
                # the TDBC results set represented by tdbc_result. Closing
                # a tdbc::statement closes also any active tdbc::resultset
                # owned by it

                set result_obj [$this result TDBC -resultid   $tdbc_result      \
                                                  -statement  $tdbc_statement   \
                                                  -isselect   $is_select        \
                                                  -fields     [::list [$tdbc_result columns]]] 
            }

            return $result_obj
        }

    }

    ::itcl::class TDBCResult {
        inherit Result
        public variable     isselect false
        public variable     statement

        public variable     rowid
        public variable     cached_rows
        public variable     columns

        constructor {args} { 
            eval configure  $args
            set cached_rows {}
            set columns     {}
            set rowid       0
            set rownum      0
            set statement   ""
        }
        destructor {}

        public method destroy {} {
            if {$statement != ""} { $statement close }

            Result::destroy
        }

        public method current_row {} {return $rowid}
        public method cached_results {} {return $cached_rows}

        public method nextrow {} {
            if {[llength $cached_rows] == 0} {
                if {![$resultid nextrow -as lists row]} {
                    return ""
                }
            } else {
                set cached_rows [lassign $cached_rows row]
            }
            incr rowid
            return $row
        }

        public method numrows {} {
            if {$isselect} {

                # this is not scaling well at all but tdbc is not telling
                # the number of columns for a select so must determine it
                # from the whole set of results

                if {[llength $cached_rows] == 0} {
                    set cached_rows [$resultid allrows -as lists -columnsvariable columns]
                }
                return [expr [llength $cached_rows] + $rowid]
            } else {
                return [$resultid rowcount]
            }
        }

    }
}
