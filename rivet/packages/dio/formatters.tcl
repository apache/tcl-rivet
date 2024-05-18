# formatters.tcl -- connector for tdbc, the Tcl database abstraction layer
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

namespace eval DIO::formatters {

    #
    # quote - given a string, return the same string with any single
    #  quote characters preceded by a backslash
    #
    proc quote {a_string} {
        regsub -all {'} $a_string {\'} a_string
        return $a_string
    }

    # ::itcl::class FieldFormatter
    #
    # we devolve the role of special field formatter to this
    # class. By design this is more sensible with respect to
    # the current approach of having such method in subclasses
    # because it allows to reuse the functionality of this
    # class in other DBMS connector. 

    # this class must be subclassed for each database type

    ::itcl::class RootFormatter {

        private variable special_fields [dict create]


        public method register {table_name field_name ftype} {
            dict set special_fields $table_name $field_name $ftype
        }

        public method build {table_name field_name val convert_to} {
            if {[dict exists $special_fields $table_name $field_name]} {
                set field_type [dict get $special_fields $table_name $field_name]

                if {[catch {
                    set field_value [$this $field_type $table_name $field_name $val $convert_to]
                } e einfo]} {
                    return "'[quote $val]'"
                }

                return $field_value
            } else {
                return "'[quote $val]'"
            }
        }

    } ; ## ::itcl::class FieldFormatter

    ::itcl::class Mysql {
        inherit RootFormatter

        public method DATE {table_name field_name val convert_to} {
            set secs [clock scan $val]
            set my_val [clock format $secs -format {%Y-%m-%d}]
            return "DATE_FORMAT('$my_val','%Y-%m-%d')"
        }

        public method DATETIME {table_name field_name val convert_to} {
            set secs [clock scan $val]
            set my_val [clock format $secs -format {%Y-%m-%d %T}]
            return "DATE_FORMAT('$my_val','%Y-%m-%d %T')"
        }

        public method NOW {table_name field_name val convert_to} {

		    # we try to be coherent with the original purpose of this method whose
		    # goal is endow the class with a uniform way to handle timestamps. 
		    # E.g.: Package session expects this case to return a timestamp in seconds
		    # so that differences with timestamps returned by [clock seconds]
		    # can be done and session expirations are computed consistently.
		    # (Bug #53703)

            switch $convert_to {
                SECS {
                    if {[::string compare $val "now"] == 0} {

#                       set     secs    [clock seconds]
#                       set     my_val  [clock format $secs -format {%Y%m%d%H%M%S}]
#                       return  $my_val

                        return [clock seconds]

                    } else {
                        return  "UNIX_TIMESTAMP($field_name)"
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

                    #set my_val [clock format $secs -format {%Y-%m-%d %T}]
                    return "FROM_UNIXTIME('$secs')"
                }
            }
        }

        public method NULL {table_name field_name val convert_to} {
            if {[::string toupper $val] == "NULL"} {
                return $val
            } else {
                return "'[quote $val]'"
            }
        }

    }




} ; ## namespace eval DIO

package provide dio::formatters 1.0
