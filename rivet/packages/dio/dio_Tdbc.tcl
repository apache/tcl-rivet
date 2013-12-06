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
# $Id: $ 
#

package provide dio_Tdbc 0.1

namespace eval DIO {


    ::itcl::class Tdbc {
        inherit Database

        private variable dbhandle
        private common   conncnt    0

        public varabile backend "" {

            if {![string equal $backend "mysql"] && \
                ![string equal $backend "postgres"] && \
                ![string equal $backend "sqlite3"] && \
                ![string equal $backend "odbc"]} {

                return -code error "backend '$backend' not supported"

            }

        }

        constructor {args} { eval configure $args } {
            if {[catch {package require tdbc}]} {

                return -code error "No Tdbc package available"

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
            set command [list ::tdbc::mysql::connection create [incr conncnt]]

            if {![lempty $user]} { lappend command -user $user }
            if {![lempty $pass]} { lappend command -password $pass }
            if {![lempty $port]} { lappend command -port $port }
            if {![lempty $host]} { lappend command -host $host }

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

            if {[catch {set res [$sqlstat execute]} err} {
                set obj [result Tdbc -error 1 -errorinfo $err]
            } else {
                set obj [result Tdbc -resultid $res           \
                                     -numrows [$res rowcount] \
                                     -fields  [$res columns]]
            }

            $sqlstat destroy
            return $obj
        }

# -- execute
#
#  extended version of the standard DIO method exec that
# makes room for an extra argument storing the dictionary
# of variables to be substituted in the SQL statement

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

            $sqlstat destroy
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

# -- TdbcResult
#
# Basically a wrapper around a Tdbc resultset object
#

    ::itcl::class TdbcResult {
        inherit Result

        constructor {args} {
            eval configure $args
        }

        public method nextrow {} {

            $resultid nextlist v
            return $v

        }

    }
}
