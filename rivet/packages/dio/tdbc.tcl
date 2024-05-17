# tdbc.tcl -- connector for tdbc, the Tcl database abstraction layer
#
# Copyright 2024 The Apache Software Foundation
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
#


package require DIO
package require tdbc
package provide dio_Tdbc 0.2

namespace eval DIO {
    ::itcl::class Tdbc {
        inherit Database

        private variable connector_n
        private variable connector
        private variable tdbc_arguments [list -encoding     \
                                              -isolation    \
                                              -readonly     \
                                              -timeout]

        constructor {interface_name args} {eval configure {*}$args} {
            set connector_n 0
            set connector   ""

            # I should check this one: we only accept connector
            # interfaces that map into tdbc's
            set interface "tdbc::[::string tolower $interface_name]"

            package require ${interface}
        }

        destructor { }

        private method check_connector {} {
            if {$connector == ""} { open }
        }

        public method open {}  {
            set connector_cmd "${interface}::connection create $interface#$connector_n"
            if {$user != ""} { lappend connector_cmd -user $user }
            if {$db   != ""} { lappend connector_cmd -db   $db }
            if {$pass != ""} { lappend connector_cmd -password $pass }
            if {$port != ""} { lappend connector_cmd -port $port }
            if {$host != ""} { lappend connector_cmd -host $host }

            if {$clientargs != ""} { lappend connector_cmd {*}$clientargs }

            puts "evaluating $connector_cmd"

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
                if {[lsearch $k $tdbcarguments] >= 0} {
                    lappend clientargs $k $v
                }
            }
        }

        public method exec {sql} {
            $this check_connector

            # tdbc doesn't like ';' at the end of a SQL statement

            if {[::string index $sql end] == ";"} {set sql [::string range $sql 0 end-1]}
            set is_select [regexp -nocase {^\(*\s*select\s+} $sql]

            set sql_st [$connector prepare $sql]

            # errorinfo is a public variable of the
            # parent class Database. Not a good
            # object design practice

            if {[catch {set tdbc_result [$sql_st execute]} errorinfo]} {
                return [$this result TDBC -error 1 -errorinfo $errorinfo -isselect false]
            } else {

                # we must store also the TDBC SQL statement as it owns
                # the TDBC results set represented by tdbc_result. Closing
                # a tdbc::statement closes also any active tdbc::resultset
                # owned by it

                set result_obj [$this result TDBC -resultid   $tdbc_result  \
                                                  -statement  $sql_st       \
                                                  -isselect   $is_select    \
                                                  -fields     [::list [$tdbc_result columns]]] 
            }
        }


    }

    ::itcl::class TDBCResult {
        inherit Result
        public variable     isselect false
        public variable     statement

        public variable     rowid
        public variable     cached_rows
        public variable     columns

        private variable    rownum

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
                set row [lindex $cached_rows $rowid]
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
                    set rownum $rowid
                    set cached_rows [$resultid allrows -as lists -columnsvariable columns]
                    set rowid 0
                }
                return [expr [llength $cached_rows] + $rownum]
            } else {
                return [$resultid rowcount]
            }
        }

    }
}
