# dio_Postgresql.tcl -- Postgres backend.

# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#   http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package require DIO
package provide dio_Postgresql 0.1

namespace eval DIO {
    ::itcl::class Postgresql {
    inherit Database

    constructor {args} {eval configure $args} {
        package require Pgtcl
        set_conn_defaults
        eval configure $args
    }

    destructor {
        close
    }

    ## Setup our variables with the default conninfo from Postgres.
    private method set_conn_defaults {} {
        foreach list [pg_conndefaults] {
        set var [lindex $list 0]
        set val [lindex $list end]
        switch -- $var {
            "dbname" { set db $val }
            default  { set $var $val }
        }
        }
    }

    method open {} {
        set command "pg_connect"

        set info ""
        if {![::rivet::lempty $user]} { append info " user=$user" }
        if {![::rivet::lempty $pass]} { append info " password=$pass" }
        if {![::rivet::lempty $host]} { append info " host=$host" }
        if {![::rivet::lempty $port]} { append info " port=$port" }
        if {![::rivet::lempty $db]}   { append info " dbname=$db" }

        if {![::rivet::lempty $info]} { append command " -conninfo [::list $info]" }

        if {[catch $command error]} { return -code error $error }

        set conn $error
    }

    method close {} {
        if {![info exists conn]} { return }
        pg_disconnect $conn
        unset conn
    }

    method exec {req} {
        if {![info exists conn]} { open }

        set command pg_exec
        if {[catch {$command $conn $req} result]} { return -code error $result }

        set errorinfo ""
        set obj [result Postgresql -resultid $result]
        if {[$obj error]} { set errorinfo [$obj errorinfo] }
        return $obj
    }

    method nextkey {} {
        return [$this string "select nextval( '$sequence' )"]
    }

    method lastkey {} {
        return [$this string "select last_value from $sequence"]
    }

    method sql_limit_syntax {limit {offset ""}} {
        set sql " LIMIT $limit"
        if {![::rivet::lempty $offset]} { append sql " OFFSET $offset" }
        return $sql
    }

    #
    # handle - return the internal database handle, in the postgres
    # case, the postgres connection handle
    #
    method handle {} {
        if {![info exists conn]} { open }
        return $conn
    }

    method makeDBFieldValue {table_name field_name val {convert_to {}}} {
        if {[info exists specialFields(${table_name}@${field_name})]} {
            switch $specialFields(${table_name}@${field_name}) {
                DATE {
                    set secs [clock scan $val]
                    set my_val [clock format $secs -format {%Y-%m-%d}]
                    return "'$my_val'"
                }
                DATETIME {
                    set secs [clock scan $val]
                    set my_val [clock format $secs -format {%Y-%m-%d %T}]
                    return "'$my_val'"
                }
                NOW {
                    switch $convert_to {

                        # we try to be coherent with the original purpose of this method whose
                        # goal is to provide to the programmer a uniform way to handle timestamps. 
                        # E.g.: Package session expects this case to return a timestamp in seconds
                        # so that differences with timestamps returned by [clock seconds]
                        # can be done and session expirations are computed consistently.
                        # (Bug #53703)

                        SECS {
                            if {[::string compare $val "now"] == 0} {
#                   set secs    [clock seconds]
#                   set my_val  [clock format $secs -format {%Y%m%d%H%M%S}]
#                   return  $my_val
                                return [clock seconds]
                            } else {
                                return  "extract(epoch from $field_name)"
                            }
                        }
                        default {
                            if {[::string compare $val, "now"] == 0} {
                                set secs [clock seconds]
                            } else {
                                set secs [clock scan $val]
                            }

                            # this is kind of going back and forth from the same 
                            # format,

                            return "'[clock format $secs -format {%Y-%m-%d %T}]'"
                        }
                    }
                }
                default {
                    # no special code for that type!!
                    return [pg_quote $val]
                }
            }
        } else {
                return [pg_quote $val]
        }
    }


    ## If they change DBs, we need to close the connection and re-open it.
    public variable db "" {
        if {[info exists conn]} {
            close
            open
        }
    }

    public variable interface   "Postgresql"
    private variable conn

    } ; ## ::itcl::class Postgresql

    #
    # PostgresqlResult object -- superclass of ::DIO::Result object
    #
    #
    ::itcl::class PostgresqlResult {
        inherit Result

        constructor {args} {
            eval configure $args

            if {[::rivet::lempty $resultid]} {
                return -code error "No resultid specified while creating result"
            }

            set numrows   [pg_result $resultid -numTuples]
            set fields    [pg_result $resultid -attributes]
            set errorcode [pg_result $resultid -status]
            set errorinfo [pg_result $resultid -error]

            # if numrows is zero, see if cmdrows returned anything and if it
            # did, put that in in place of numrows, hiding a postgresql
            # idiosyncracy from DIO
            if {$numrows == 0} {
                set cmdrows [pg_result $resultid -cmdTuples]
                if {$cmdrows != ""} {
                    set numrows $cmdrows
                }
            }

            if {$errorcode != "PGRES_COMMAND_OK" && \
                $errorcode != "PGRES_TUPLES_OK"} { set error 1 }

            ## Reconfigure incase we want to overset the default values.
            eval configure $args
        }

        destructor {
            pg_result $resultid -clear
        }

        method clear {} {
            pg_result $resultid -clear
        }

        method nextrow {} {
            if {$rowid >= $numrows} { return }
            return [pg_result $resultid -getTuple $rowid]
        }

    } ; ## ::itcl::class PostgresqlResult

}
