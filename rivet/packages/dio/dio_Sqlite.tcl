# dio_Sqlite.tcl -- DIO interface for sqlite

# Copyright 2004 The Apache Software Foundation

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

package provide dio_Sqlite 0.1

namespace eval DIO {
    ::itcl::class Sqlite {
	inherit Database

	private variable dbcmd ""
	private variable dbname sqlitedb
	constructor {args} {eval configure $args} {
	    package require sqlite
	    eval configure $args
	}

	destructor {
	    catch { $dbcmd close }
	}

	method open {} {
	    ::sqlite [::itcl::scope dbcmd] $db
	    set db [::itcl::scope dbcmd]
	}

	method close {} {
	    catch { $dbcmd close }
	}

	method exec {req} {
	    if { $dbcmd == "" } {
		open
	    }

	    if { [catch {$dbcmd eval $req} result] } {
		return -code error $result
	    }
	    set errorInfo ""

	    set obj [result sqlite -resultid $result]
	    if {[$obj error]} {
		set errorinfo [$obj errorinfo]
	    }
	    return $obj
	}

	method nextkey {} {
	    return [$this string "select nextval( '$sequence' )"]
	}

	method lastkey {} {
	    return [$this string "select last_value from $sequence"]
	}

	## If they change DBs, we need to close the connection and re-open it.
	public variable db "" {
	    if {[info exists conn]} {
		close
		open
	    }
	}

    }



# THIS BIT IS UNFINISHED XXX.

    ::itcl::class SqliteResult {
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