# dio_Postgresql.tcl -- Postgres backend.

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
	    if {![lempty $user]} { append info " user=$user" }
	    if {![lempty $pass]} { append info " password=$pass" }
	    if {![lempty $host]} { append info " host=$host" }
	    if {![lempty $port]} { append info " port=$port" }
	    if {![lempty $db]}   { append info " dbname=$db" }

	    if {![lempty $info]} { append command " -conninfo [::list $info]" }

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
	    if {![lempty $offset]} { append sql " OFFSET $offset" }
	    return $sql
	}

	#
	# handle - return the internal database handle, in the postgres
	# case, the postgres connection handle
	#
	method handle {} {
	    if {[info exists conn]} {
		return $conn
	    } else {
		return ""
	    }
	}

	## If they change DBs, we need to close the connection and re-open it.
	public variable db "" {
	    if {[info exists conn]} {
		close
		open
	    }
	}

	public variable interface	"Postgresql"
	private variable conn

    } ; ## ::itcl::class Postgresql

    ::itcl::class PostgresqlResult {
	inherit Result

	constructor {args} {
	    eval configure $args

	    if {[lempty $resultid]} {
		return -code error "No resultid specified while creating result"
	    }

	    set numrows   [pg_result $resultid -numTuples]
	    set fields    [pg_result $resultid -attributes]
	    set errorcode [pg_result $resultid -status]
	    set errorinfo [pg_result $resultid -error]

	    if {$errorcode != "PGRES_COMMAND_OK" \
		    && $errorcode != "PGRES_TUPLES_OK"} { set error 1 }

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
