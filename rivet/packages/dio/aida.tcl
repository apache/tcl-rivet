# aida.tcl -- agnostic interface to TDBC

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

# $Id$

package require Tcl  8.6
package require Itcl

source [file join [file dirname [info script]] sql.tcl]

namespace eval ::aida {

proc handle {interface args} {
    set obj \#auto
    set first [lindex $args 0]
    if {![lempty $first] && [string index $first 0] != "-"} {

        set args [lassign $args obj]

    }

    #uplevel \#0 package require dio_$interface
    #return [uplevel \#0 ::DIO::$interface $obj $args]
    return [uplevel \#0 ::aida::Aida [Sql $interface] $interface $obj $args]
}

# -- aida database interface class

::itcl::class Aida {

    constructor { sqlobj args } {
        set sql $sqlobj
        eval $this configure $args
    }

    destructor {
        close
    }

    protected method result {backend args} 
    public    method quote {string} {}
    protected method build_select_query {args} { }
    protected method build_insert_query {arrayName fields {myTable ""}} {}
    protected method build_update_query {arrayName fields {myTable ""}} {}
    protected method lassign_array {list arrayName args} {}

    private variable sql
}

    ::itcl::body Aida::build_select_query {args} {
        return [$sqlobj build_select_query {*}$args]
    }


    # -- result
    #
    # returns a return object
    #

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


