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

    # -- result
    #
    # returns a return object
    #

    ::itcl::class Result {args} { 

        public variable resultid    ""
        public variable fields      ""
        public variable rowid       0
        public variable numrows     0
        public variable error       0
        public variable errorcode   0
        public variable errorinfo   ""
        public variable autocache   1

        constructor {args} {
            eval configure $args
        }

        destructor { }

        method destroy {} {
            ::itcl::delete object $this
        }

        protected method configure_variable {varName string}

    }

    #
    # configure_variable - given a variable name and a string, if the
    # string is empty return the variable name, otherwise set the
    # variable to the string.
    #
    ::itcl::body Result::configure_variable {varName string} {
        if {[lempty $string]} { return [$this cget -$varName] }
        $this configure -$varName $string
    }


}


