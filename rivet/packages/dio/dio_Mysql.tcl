# -- dio_Mysql.tcl -- Mysql backend.

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

package require DIO       1.2
package provide dio_Mysql 1.2

namespace eval DIO {
    ::itcl::class Mysql {
        inherit Database

        constructor {args} {eval configure -interface Mysql $args} {
            if {   [catch {package require Mysqltcl}]   \
                && [catch {package require mysqltcl}]   \
                && [catch {package require mysql}]} {
                return -code error "No MySQL Tcl package available"
            }

            eval configure $args

            if {[::rivet::lempty $db]} {
                if {[::rivet::lempty $user]} {
                    set user $::env(USER)
                }
                set db $user
            }

        }

        destructor {
            close
        }

        method open {} {
            set command "mysqlconnect"

            if {![::rivet::lempty $user]} { lappend command -user $user }
            if {![::rivet::lempty $pass]} { lappend command -password $pass }
            if {![::rivet::lempty $port]} { lappend command -port $port }
            if {![::rivet::lempty $host]} { lappend command -host $host }
            #if {![::rivet::lempty $encoding]} { lappend command -encoding $encoding }

            if {$clientargs != ""} {
                set command [lappend command {*}$clientargs]
            }

            #puts stderr "evaluating $command"

            if {[catch $command error]} { return -code error $error }

            set conn $error

            if {![::rivet::lempty $db]} { mysqluse $conn $db }
        }

        method close {} {
            if {![info exists conn]} { return }
            catch {mysqlclose $conn}
            unset conn
        }

        method exec {req} {
            if {![info exists conn] || ![mysqlping $conn]} { open }

            set cmd mysqlexec
#
#           if {[::string tolower [lindex $req 0]] == "select"} { set cmd mysqlsel }
#           select is a 6 characters word, so let's see if the query is a select
#
            set q [::string trim $req]

#           set q [::string tolower $q]
#           set q [::string range $q 0 5]
#           if {[::string match select $q]} { set cmd mysqlsel }

            if {[regexp -nocase {^\(*\s*select\s+} $q]} { set cmd mysqlsel }

            set errorinfo ""
            if {[catch {$cmd $conn $req} error]} {
                set errorinfo $error
                set obj [result Mysql -error 1 -errorinfo [::list $error]]
                return $obj
            }
            if {[catch {mysqlcol $conn -current name} fields]} { set fields "" }
            set obj [result Mysql -resultid   $conn               \
                                  -numrows    [::list $error]     \
                                  -fields     [::list $fields]]
            return $obj
        }

        method lastkey {} {
            if {![info exists conn] || ![mysqlping $conn]} { return }
            return [mysqlinsertid $conn]
        }

        method quote {string} {
            if {![catch {mysqlquote $string} result]} { return $result }
            regsub -all {'} $string {\'} string
            return $string
        }

        method sql_limit_syntax {limit {offset ""}} {
            if {[::rivet::lempty $offset]} {
                return " LIMIT $limit"
            }
            return " LIMIT [expr $offset - 1],$limit"
        }

        method handle {} {
            if {![info exists conn] || ![mysqlping $conn]} { open }
            return $conn
        }

        public variable db "" {
            if {[info exists conn] && [mysqlping $conn]} {
                mysqluse $conn $db
            }
        }

        protected method handle_client_arguments {cargs} {

            # we assign only the accepted options

            set clientargs {}

            foreach {a v} $cargs {

                if {($a == "-encoding") || \
                    ($a == "-localfiles") || \
                    ($a == "-ssl") || \
                    ($a == "-sslkey") || \
                    ($a == "-sslcert") || \
                    ($a == "-sslca") || \
                    ($a == "-sslcapath") || \
                    ($a == "-sslcipher") || \
                    ($a == "-socket")} {
                    lappend clientargs $a $v
                }

            }
        }

        #public  variable interface "Mysql"
        private variable conn

    } ; ## ::itcl::class Mysql

    ::itcl::class MysqlResult {
        inherit Result

        constructor {args} {
            eval configure $args
        }

        destructor {

        }

        method nextrow {} {
            return [mysqlnext $resultid]
        }
        
    } ; ## ::itcl::class MysqlResult

}
