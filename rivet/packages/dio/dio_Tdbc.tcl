# dio_Tdbc.tcl -- Tdbc compatibility layer
#
# Copyright 2000-2005 The Apache Software Foundation
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# DIO compatibility layer with Tdbc
# 
# $Id$ 
#

package provide dio_Tdbc 0.1

namespace eval DIO {


    ::itcl::class Tdbc {
        inherit Database

        private variable dbhandle
        public  variable interface "Tdbc"
        private common   conncnt    0

        public variable backend "" {

            if {$backend == "mysql"} {

                package require tdbc::mysql

            } elseif {$backend == "postgres"} {

                package require tdbc::postgres

            } elseif {$backend == "sqlite3"} {

                package require tdbc::sqlite3

            } elseif {$backend == "odbc"} {

                package require tdbc::odbc

            } elseif {$backend == ""} {

                return -code error "DIO Tdbc needs a backend be specified"

            } else {

                return -code error "backend '$backend' not supported"

            }

        }

# -- destructor 
#
#

        constructor {args} { eval configure $args } {
            if {[catch {package require tdbc}]} {

                return -code error "No Tdbc package available"

            }

            eval configure $args

            if {[lempty $db]} {
                if {[lempty $user]} {
                    set user $::env(USER)
                }
                set db $user
            }
        }

        destructor { close }

# --close
#
# we take inspiration from the DIO_Mysql class for handling
# the basic connection data

        public method close {} {
            if {![info exists dbhandle]} { return }
            catch { $dbhandle close }            

            unset dbhandle
        }

# -- open
#
# Opening a connection with this class means that the member
# variable specifying the backend was properly set
#

        public method open {} {
            if {$backend == ""} {
                return -code error "no backend set"
            }
            set command [::list ::tdbc::${backend}::connection create tdbc[incr conncnt]]

            if {![lempty $user]} { lappend command -user $user }
            if {![lempty $pass]} { lappend command -password $pass }
            if {![lempty $port]} { lappend command -port $port }
            if {![lempty $host]} { lappend command -host $host }
            if {![lempty $db]}   { lappend command -database $db }

            if {[catch {
                set dbhandle [eval $command]
            } e]} { return -code error $e }


            return -code ok
        }

# -- exec
#
# sql code central method. A statement object
# is created from the sql string and then executed
#

        public method exec {sql} {

            if {![info exists dbhandle]} { $this open }

            set sqlstat [$dbhandle prepare $sql]

            if {[catch {set res [$sqlstat execute]} err]} {
                set obj [result Tdbc -error 1 -errorinfo $err]
            } else {
                set obj [result Tdbc -resultid $res           \
                                     -sqlstatement $sqlstat   \
                                     -numrows [$res rowcount] \
                                     -fields  [::list [$res columns]]]
            }

            #$res nextlist cols
            #puts "rows: [$res rowcount]"
            #puts "cols: $cols"

            return $obj
        }

# -- execute
#
#  extended version of the standard DIO method exec that
# makes room for an extra argument storing the dictionary
# of variables to be substituted in the SQL statement
#

        public method execute {sql {substitute_d ""}} {

            if {![info exists dbhandle]} { $this open }

            set sqlstat [$dbhandle prepare $sql]
            if {$substitute_d != ""} {
                set cmd [list $sqlstat execute $substitude_d]
            } else {
                set cmd [list $sqlstat execute]
            }


            if {[catch {set res [eval $cmd]} err} {
                set obj [result Tdbc    -error 1 -errorinfo $err]
            } else {
                set obj [result Tdbc    -resultid $res           \
                                        -numrows [$res rowcount] \
                                        -fields  [$res columns]]
            }

            $sqlstat close
            return $obj
        }


# -- handle
#
# accessor to the internal connection handle. 
#

        public method handle {} {
            return $dbhandle
        }

    }

# 
# -- Class TdbcResult
#
# Class wrapping a Tdbc resultset object and adapting it
# to the DIO Results interface
#

    ::itcl::class TdbcResult {
        inherit Result

        public variable sqlstatement

        constructor {args} {
            eval configure $args
        }

        destructor {
            catch {$sqlstatement close}
        }

# -- nextrow
#
# Returns the list of values selected by a SQL command.
# Values appear in the list with the same order of 
# the columns names returned by the 'columns' object command
#

        public method nextrow {} {
            $resultid nextlist v
            return $v
        }

    }
}
