# dio_Mysql.tcl -- Mysql backend.

# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $Id$

package provide dio_Mysql 0.1

namespace eval DIO {
    ::itcl::class Mysql {
	inherit Database

	constructor {args} {eval configure $args} {
	    if {[catch {package require Mysqltcl}] \
		    && [catch {package require mysql}]} {
		return -code error "No MySQL Tcl package available"
	    }

	    eval configure $args

	    if {[lempty $db]} {
		if {[lempty $user]} {
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

	    if {![lempty $user]} { lappend command -user $user }
	    if {![lempty $pass]} { lappend command -password $pass }
	    if {![lempty $port]} { lappend command -port $port }
	    if {![lempty $host]} { lappend command $host }

	    if {[catch $command error]} { return -code error $error }

	    set conn $error

	    if {![lempty $db]} { mysqluse $conn $db }
	}

	method close {} {
	    if {![info exists conn]} { return }
	    catch {mysqlclose $conn}
	    unset conn
	}

	method exec {req} {
	    if {![info exists conn]} { open }

	    set cmd mysqlexec
	    if {[::string tolower [lindex $req 0]] == "select"} { set cmd mysqlsel }

	    set errorinfo ""
	    if {[catch {$cmd $conn $req} error]} {
		set errorinfo $error
		set obj [result Mysql -error 1 -errorinfo [::list $error]]
		return $obj
	    }
	    if {[catch {mysqlcol $conn -current name} fields]} { set fields "" }
	    set obj [result Mysql -resultid $conn \
			 -numrows [::list $error] -fields [::list $fields]]
	    return $obj
	}

	method lastkey {} {
	    if {![info exists conn]} { return }
	    return [mysqlinsertid $conn]
	}

	method quote {string} {
	    if {![catch {mysqlquote $string} result]} { return $result }
	    regsub -all {'} $string {\'} string
	    return $string
	}

	method sql_limit_syntax {limit {offset ""}} {
	    if {[lempty $offset]} {
		return " LIMIT $limit"
	    }
	    return " LIMIT [expr $offset - 1],$limit"
	}

	method handle {} {
	    if {![info exists conn]} { open }

	    return $conn
	}

	public variable db "" {
	    if {[info exists conn]} {
		mysqluse $conn $db
	    }
	}

	public variable interface	"Mysql"
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
