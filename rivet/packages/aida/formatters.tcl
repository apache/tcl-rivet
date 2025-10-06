# formatters.tcl -- agnostic interface to database access special
#                   fields formatters
#
# Copyright 2025 The Apache Tcl Team
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

package require TclOO

namespace eval ::aida::formatters {

    ::oo::class create RootFormatter {

        variable SpecialFields [dict create]

        classmethod quote {a_string} {
            regsub -all {'} $a_string {\'} a_string
            return $a_string
        }

        method register {table_name field_name ftype} {
            dict set special_fields $table_name $field_name $ftype
        }

        method build {table_name field_name val convert_to} {
            if {[dict exists $special_fields $table_name $field_name]} {
                set field_type [dict get $special_fields $table_name $field_name]

                if {[catch {
                    set field_value [$this $field_type $field_name $val $convert_to]
                } e einfo]} {
                    set field_value "'[quote $val]'"
                }

                return $field_value
            } else {
                return "'[quote $val]'"
            }
        }

    } ; ## ::oo::class RootFormatter

}
