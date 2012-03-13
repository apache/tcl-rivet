# simpledb.tcl -- provides a simple tcl database.

# Copyright 2003-2004 The Apache Software Foundation

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

package provide simpledb 0.1

namespace eval ::simpledb {
    set oid 0
}

# simpledb::createtable --
#
#	Creates a table and its associated columns.
#
# Arguments:
#	table - name of the table.
#	args - column names.
#
# Side Effects:
#	Creates internal namespace and arrays.
#
# Results:
#	None.

proc simpledb::createtable { table args } {
    namespace eval $table {}
    array set ${table}::cols {}
    # Currently active oids.
    array set ${table}::goodoids {}

    foreach col $args {
	# Each key gets its own namespace.
	namespace eval ${table}::${col} {}
	# In that namespace we have an array that maps oids->data.
	array set ${table}::${col}::data {}
	# And an array that maps data->oids.
	array set ${table}::${col}::values {}
	set ${table}::cols($col) 1
    }
}


# simpledb::deltable --
#
#	Delete table.
#
# Arguments:
#	table - table to delete.
#
# Side Effects:
#	Deletes table namespace.
#
# Results:
#	None.

proc simpledb::deltable { table } {
    namespace delete $table
}


# simpledb::tables --
#
#	Return a list of all tables.
#
# Arguments:
#	None.
#
# Side Effects:
#	None.
#
# Results:
#	A list of all tables that exist in the database.

proc simpledb::tables {} {
    set res {}
    foreach ns [namespace children [namespace current]] {
	lappend res [namespace tail $ns]
    }
    return $res
}


# simpledb::createitem --
#
#	Create an item in the table.
#
# Arguments:
#	table - table name.
#	properties - a list of keys and their corresponding values.
#	Keys must correspond to those listed in 'createtable'.
#
# Side Effects:
#	Creates a new table item.
#
# Results:
#	None.

proc simpledb::createitem { table properties } {
    variable oid
    incr oid
    set ${table}::goodoids($oid) 1
    foreach {col data} $properties {
	set ${table}::${col}::data($oid) $data
	lappend ${table}::${col}::values($data) $oid
    }
    return $oid
}

# simpledb::getitem --
#
#	Fetches an item from the database based on its oid.
#
# Arguments:
#	table - table name.
#	oid - identity of the item to fetch.
#
# Side Effects:
#	None.
#
# Results:
#	Returns information as a list suitable to pass to 'array set'.

proc simpledb::getitem { table oid } {
    foreach col [array names ${table}::cols] {
	lappend res $col [set ${table}::${col}::data($oid)]
    }
    return $res
}


# simpledb::setitem --
#
#	Set the values of given keys.
#
# Arguments:
#	table - table name.
#	oid - item's unique id.
#	properties - list of keys and values.
#
# Side Effects:
#	The old value of the item is lost.
#
# Results:
#	None.

proc simpledb::setitem { table oid properties } {
    upvar $properties props
    foreach {col data} $properties {
	if { [info exists ${table}::${col}::data($oid)] } {
	    set oldval [set ${table}::${col}::data($oid)]
	    set item [lsearch [set ${table}::${col}::values($oldval)] $oid]
	    if { $item >= 0 } {
		set ${table}::${col}::values($oldval) \
		    [lreplace ${table}::${col}::values($oldval) $item $item]
	    }
	    if { [llength [set ${table}::${col}::values($oldval)]] == 0 } {
		unset ${table}::${col}::values($oldval)
	    }
	}

	set ${table}::${col}::data($oid) $data
	lappend ${table}::${col}::values($data) $oid
    }
    return $oid
}


# simpledb::delitem --
#
#	Delete an item from the database.  This is slow because of the
#	lsearch.
#
# Arguments:
#	table - table name.
#	oid - object's unique id.
#
# Side Effects:
#	Deletes item from the database.
#
# Results:
#	None.

proc simpledb::delitem { table oid properties } {
    upvar $properties props

    foreach col [array names ${table}::cols] {
        unset ${table}::${col}::data($oid)
        set item [lsearch ${table}::${col}::values($props($col)) $oid]
        set ${table}::${col}::values($props($col)) \
            [lreplace ${table}::${col}::values($props($col)) $item $item]
    }
    unset ${table}::goodoids($oid)
    return $oid
}


# simpledb::finditems --
#
#	Find items that match the given "properties" - a list of keys
#	and the sought values.  Glob patterns are accepted as
#	'values'.
#
# Arguments:
#	table - table name.
#	propertymatch - list of keys and values to search on.
#
# Side Effects:
#	None.
#
# Results:
#	A list of the id's of matching item.

proc simpledb::finditems { table propertymatch } {
    array set res {}
    foreach {col value} $propertymatch {
        foreach {value oids} [array get ${table}::${col}::values $value] {
            foreach oid $oids {
                if { [info exists res($oid)] } {
                    incr res($oid)
                } else {
                    set res($oid) 1
                }
            }
        }
    }
    set retlist {}
    foreach {oid num} [array get res] {
        if { $res($oid) == [llength $propertymatch] / 2 } {
            lappend retlist $oid
        }
    }
    return $retlist
}


# simpledb::items --
#
#	Fetch all the items from a particular table.
#
# Arguments:
#	table.
#
# Side Effects:
#	None.
#
# Results:
#	A list of lists, with the sublists being key/value lists of
#	column names and their value for the oid in question.

proc simpledb::items {table} {
    set reslist {}
    set collist [array names ${table}::cols]
    foreach oid [array names ${table}::goodoids] {
	set oidlist {}
	foreach col $collist {
	    if { [info exists ${table}::${col}::data($oid)] } {
		lappend oidlist $col [set ${table}::${col}::data($oid)]
	    }
	}
	lappend reslist $oidlist
    }

    return $reslist
}


# simpledb::synctostorage --
#
#	Writes the database to a file.  The storage format, for the
#	moment is Tcl code, which isn't space efficient, but is easy
#	to reload.
#
# Arguments:
#	savefile - file to save database in.
#
# Side Effects:
#	None.
#
# Results:
#	None.

proc simpledb::synctostorage {savefile} {
    set fl [open $savefile w]
    foreach ns [namespace children] {
	# Let's store the goodoids array.
	set collist [array names ${ns}::cols]
	puts $fl "namespace eval $ns \{"
	puts $fl "    array set cols \{ [array get ${ns}::cols] \}"
	puts $fl "    array set goodoids \{ [array get ${ns}::goodoids] \}"
	foreach col $collist {
	    puts $fl "    namespace eval ${col} \{"
	    puts $fl "        array set data [list [array get ${ns}::${col}::data]]"
	    puts $fl "        array set values [list [array get ${ns}::${col}::values]]"
	    puts $fl "    \}"
	}
	puts $fl "\}"
    }
    close $fl
}


# simpledb::syncfromstorage --
#
#	Reloads database from file.
#
# Arguments:
#	savefile - file to read.
#
# Side Effects:
#	Creates database.
#
# Results:
#	None.

proc simpledb::syncfromstorage {savefile} {
    source $savefile
}

