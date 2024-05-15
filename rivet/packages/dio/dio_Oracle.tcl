# dio_Mysql.tcl -- Mysql backend.

# Copyright 2006 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $Id: dio_Oracle.tcl 265421 2004-10-29 20:17:54Z karl $

package provide dio_Oracle 0.1

namespace eval DIO {
    ::itcl::class Oracle {
	inherit Database

	constructor {args} {eval configure $args} {
	    if {[catch {package require Oratcl}]} {
            return -code error "No Oracle Tcl package available"
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
	    set command "::oralogon"

	    if {![::rivet::lempty $user]} { append command " $user" }
	    if {![::rivet::lempty $pass]} { append command "/$pass" }
	    if {![::rivet::lempty $host]} { append command "@$host" }
	    if {![::rivet::lempty $port]} { append command -port $port }

	    if {[catch $command error]} { return -code error $error }

	    set conn $error

	    if {![::rivet::lempty $db]} { 
	    	# ??? mysqluse $conn $db 
	    }
	}

	method close {} {
	    if {![info exists conn]} { return }
	    catch {::oraclose $conn}
	    unset conn
	}

	method exec {req} {
	    if {![info exists conn]} { open }

	    set _cur [::oraopen $conn]
	    set cmd ::orasql
	    set is_select 0
	    if {[::string tolower [lindex $req 0]] == "select"} {
            set cmd ::orasql
            set is_select 1
	    }
	    set errorinfo ""
#puts "ORA:$is_select:$req:<br>"
	    if {[catch {$cmd $_cur $req} error]} {
#puts "ORA:error:$error:<br>"
            set errorinfo $error
            catch {::oraclose $_cur}
            set obj [result $interface -error 1 -errorinfo [::list $error]]
            return $obj
	    }
	    if {[catch {::oracols $_cur name} fields]} { set fields "" }
	    ::oracommit $conn
	    set my_fields $fields
	    set fields [::list]
	    foreach field $my_fields {
            set field [::string tolower $field]
            lappend fields $field
	    }
	    set error [::oramsg $_cur rows]
	    set res_cmd "result"
	    lappend res_cmd $interface -resultid $_cur 
	    lappend res_cmd -numrows [::list $error] -fields [::list $fields]
	    lappend res_cmd -fetch_first_row $is_select
	    set obj [eval $res_cmd]
	    if {!$is_select} {
            ::oraclose $_cur
	    }
	    return $obj
	}

	method lastkey {} {
	    if {![info exists conn]} { return }
	    return [mysqlinsertid $conn]
	}

	method quote {string} {
	    regsub -all {'} $string {\'} string
	    return $string
	}

	method sql_limit_syntax {limit {offset ""}} {
	    # temporary
	    return ""
	    if {[::rivet::lempty $offset]} {
		return " LIMIT $limit"
	    }
	    return " LIMIT [expr $offset - 1],$limit"
	}

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
				return "to_date('$my_val', 'YYYY-MM-DD')"
			}
			DATETIME {
			  	set secs [clock scan $val]
				set my_val [clock format $secs -format {%Y-%m-%d %T}]
				return "to_date('$my_val', 'YYYY-MM-DD HH24:MI:SS')"
			}
			NOW {
			    switch $convert_to {
				SECS {
				    if {[::string compare $val "now"] == 0} {
					set secs [clock seconds]
				    	set my_val [clock format $secs -format {%Y%m%d%H%M%S}]
					return $my_val
				    } else {
                                        return "($field_name - to_date('1970-01-01')) * 86400"
				    	#return "to_char($field_name, 'YYYYMMDDHH24MISS')"
				    }
				}
				default {
				    if {[::string compare $val "now"] == 0} {
					set secs [clock seconds]
				    } else {
					set secs [clock scan $val]
				    }
				    set my_val [clock format $secs -format {%Y-%m-%d %T}]
				    return "to_date('$my_val', 'YYYY-MM-DD HH24:MI:SS')"
				}
			    }
			}
			default {
			  	# no special cod for that type!!
				return "'[quote $val]'"
			  }
			}
		} else {
			return "'[quote $val]'"
		}
	}

	public variable db "" {
	    if {[info exists conn]} {
		    mysqluse $conn $db
	    }
	}

	public variable interface	"Oracle"
	private variable conn
	private variable _cur

    } ; ## ::itcl::class Mysql

    ::itcl::class OracleResult {
	inherit Result

	public variable fetch_first_row 0
	private variable _data ""
	private variable _have_first_row 0

	constructor {args} {
	    eval configure $args
	    if {$fetch_first_row} {
		if {[llength [nextrow]] == 0} {
			set _have_first_row 0
			numrows 0
		} else {
			set _have_first_row 1
			numrows 1
		}
	    }
	    set fetch_first_row 0
	}

	destructor {
		if {[string length $resultid] > 0} {
			catch {::oraclose $resultid}
		}
	}

	method nextrow {} {
	    if {[string length $resultid] == 0} {
		return [::list]
	    }
	    if {$_have_first_row} {
		set _have_first_row 0
		return $_data
	    }
	    set ret [::orafetch $resultid -datavariable _data]
	    switch $ret {
	    0 {
		return $_data
	      }
	    1403 {
		::oraclose $resultid
		set resultid ""
		return [::list]
	      }
	    default {
		# FIXME!! have to handle error here !!
		return [::list]
	      }
	    }
	}
    } ; ## ::itcl::class OracleResult

}
