# dio_Sqlite.tcl -- DIO interface for sqlite

# Copyright 2002-2024 The Apache Software Foundation

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

package require DIO         1.2
package provide dio_Sqlite  1.2

namespace eval DIO {
    variable sqlite_seq -1

    catch { ::itcl::delete class Sqlite }

    ::itcl::class Sqlite {
        inherit Database

        private variable dbcmd      ""
	    #public  variable interface	"Sqlite"

        constructor {args} {eval configure -interface Sqlite $args} {
            if {[catch {package require sqlite}] && \
                [catch {package require sqlite3}]} {

                return -code error "No Sqlite Tcl package available"

            }
            eval configure $args
        }

        destructor {
            close
        }

        method open {} {
            variable ::DIO::sqlite_seq
            if {$dbcmd != ""} { return }
            set dbcmd dbcmd[incr sqlite_seq]
            ::sqlite3 $dbcmd $db
            set dbcmd [namespace which $dbcmd]
        }

        method close {} {
            catch { $dbcmd close }
        }

        public method create_field_formatter {} {
            set special_fields_formatter [::DIO::formatters::Sqlite3 ::DIO::formatters::#auto]
        }

        method exec {req} {
            open

            if {[$dbcmd complete $req] == 0} {
                append req ";"
                if {[$dbcmd complete $req] == 0} {
                    return -code error "Incomplete SQL"
                }
            }

            set obj [::DIO::SqliteResult #auto -request $req -dbcmd $dbcmd -select 0]
            # If it's a select statement, defer caching of results.
            # if {[regexp {^[^[:graph:]]*([[:alnum:]]*)} $req _ word]} 
            if {[::string range [::string trim $req] 0 5] == "select"} {
                $obj configure -select 1
            } else {
                # Actually perform the query
                $obj cache
            }
            return [namespace which $obj]
        }

        method list {req} {
            open

            set result ""
            $dbcmd eval $req a {
                lappend result $a([lindex $a(*) 0])
            }
            return $result
        }

        method sql_limit_syntax {limit {offset ""}} {
            set sql " LIMIT $limit"
            if {![::rivet::lempty $offset]} { append sql " OFFSET $offset" }
            return $sql
        }

        ## If they change DBs, we need to close the database. It'll be reopened
        ## on the first exec
        public variable db "" {
            if {"$dbcmd" != ""} {
                close
                set dbcmd ""
            }
        }

        #
        # quote - given a string, return the same string with any single
        #  quote characters preceded by a backslash
        #
        method quote {a_string} {
            regsub -all {'} $a_string {''} a_string
            return $a_string
        }

    }

    catch { ::itcl::delete class SqliteResult }

    # Not inheriting Result because there's just too much stuff that needs
    # to be re-done when you're deferring execution
    ::itcl::class SqliteResult {
        constructor {args} {
            eval configure $args

            if {"$request" == "--"} {
                return -code error "No SQL code provided for result"
            }

            if {"$dbcmd" == "--"} {
                return -code error "No SQLite DB command provided"
            }
        }

        destructor {
            clear
        }

        method clear {} {
            set cache {}
            set cache_loaded 0
        }

        method destroy {} {
            ::itcl::delete object $this
        }

        method resultid {args} {
            if [llength $args] {
                set resultid [lindex $args 0]
            }
            if ![info exists resultid] {
                return $request
            }
            return $resultid
        }

        method numrows {args} {
            if [llength $args] {
                set numrows [lindex $args 0]
            }
            if ![info exists numrows] {
                if ![load_cache] { return 0 }
            }
            return $numrows
        }

        method fields {args} {
            if [llength $args] {
                set fields [lindex $args 0]
            }
            if ![info exists fields] {
                if ![load_cache] { return {} }
            }
            return $fields
        }

        method errorcode {args} {
            if [llength $args] {
                set errorcode [lindex $args 0]
            }
            if ![info exists errorcode] {
                check_ok
            }
            return $errorcode
        }

        method error {args} {
            if [llength $args] {
                set error [lindex $args 0]
            }
            if ![info exists error] {
                check_ok
            }
            return $error
        }

        method errorinfo {args} {
            if [llength $args] {
                set errorinfo [lindex $args 0]
            }
            if ![info exists errorinfo] {
                check_ok
            }
            if $error {
                return $errorinfo
            }
            return ""
        }

        method autocache {args} {
            if [llength $args] {
                set autocache $args
}
            return $autocache
        }

        method cache {} {
            load_cache
        }

        protected method load_cache {} {
            if {$error_exists} { return 0 }
            if {$cache_loaded} { return 1 }
            if [catch {
                set numrows 0

                set cache {}

                # Doing a loop here because it's the only way to get the fields

                $dbcmd eval $request a {
                    incr numrows
                    set names $a(*)
                    set row {}
                    foreach field $names {
                        lappend row $a($field)
                    }
                    lappend cache $row
                }

                # some SQL commands actually don't run the script 
                # argument of method 'eval'. We therefore rely on
                # the method 'changes' which returns the number
                # of affected rows
                # (http://www.sqlite.org/tclsqlite.html#changes)

                if {($numrows == 0) && ($select == 0)} {
                    set numrows [$dbcmd changes]
                }

                if {[info exists names] && ![info exists fields]} {
                    set fields $names
                } else {
                    set fields ""
                }
            } err] {
                return [check_ok 1 $err]
            }
            set cache_loaded 1

            return [check_ok 0]
        }

        method forall {type varname body} {
            upvar 1 $varname var
            if $cache_loaded {
                foreach row $cache {
                    setvar $type var $row
                    uplevel 1 $body
                }
            } else {
                set numrows 0
                $dbcmd eval $request a {
                    incr numrows
                    set names $a(*)
                    set row {}
                    foreach field $names {
                        lappend row $a($field)
                    }
                    if $autocache {
                        lappend cache $row
                    }
                    if ![info exists fields] {
                        set fields $names
                    }
                    setvar $type var $row
                    uplevel 1 $body
                }
                if $autocache {
                    set cache_loaded 1
                    check_ok 0
                }
            }
        }

        method next {type varname} {
            if ![load_cache] { return -1 }
            if {$rowid + 1 > $numrows} { return -1 }
            upvar 1 $varname var
            incr rowid
            setvar $type var [lindex $cache $rowid]
            return $rowid
        }

        protected method setvar {type varname row} {
            upvar 1 $varname var
            switch -- $type {
                -list {
                    set var $row
                }
                -array {
                    foreach name $fields value $row {
                        set var($name) $value
                    }
                }
                -keyvalue {
                    set var {}
                    foreach name $fields value $row {
                        lappend var -$name $value
                    }
                }
                default {
                    return -code error "Unknown type $type"
                }
            }
        }

        protected method check_ok {{val -1} {info ""}} {
            if {$error_checked} { return [expr !$error] }
            if {$val < 0} {
                set val [catch {$dbcmd onecolumn $request} info]
            }
            set error $val
            set errorcode $val
            set error_checked 1
            set error_exists $val
            if {$val > 0} {
                set errorinfo $info
            } else {
                set rowid -1
            }

            return [expr !$val]
        }

        public variable autocache 0
        public variable error
        public variable errorcode
        public variable errorinfo
        public variable fields
        public variable numrows
        public variable resultid
        public variable rowid -1
        public variable select 0

        public variable request "--"
        public variable dbcmd "--"

        protected variable cache
        protected variable cache_loaded 0
        protected variable error_checked 0
        protected variable error_exists 0
    } ; ## ::itcl::class SqliteResult
}
