# dio.tcl -- implements a database abstraction layer.

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



catch {package require Tclx}
package require Itcl
set auto_path [linsert $auto_path 0 [file dirname [info script]]]

namespace eval ::DIO {

proc handle {interface args} {
    set obj \#auto
    set first [lindex $args 0]
    if {![lempty $first] && [string index $first 0] != "-"} {
	set obj  [lindex $args 0]
	set args [lreplace $args 0 0]
    }
    uplevel \#0 package require dio_$interface
    return [uplevel \#0 ::DIO::$interface $obj $args]
}

##
# DATABASE CLASS
##
::itcl::class Database {
    constructor {args} {
	eval configure $args
    }

    destructor {
	close
    }

    #
    # result - generate a new DIO result object for the specified database
    # interface, with key-value pairs that get configured into the new
    # result object.
    #
    protected method result {interface args} {
	return [eval uplevel \#0 ::DIO::${interface}Result \#auto $args]
    }

    #
    # quote - given a string, return the same string with any single
    #  quote characters preceded by a backslash
    #
    method quote {string} {
	regsub -all {'} $string {\'} string
	return $string
    }

    #
    # build_select_query - build a select query based on given arguments,
    #  which can include a table name, a select statement, switches to
    # turn on boolean AND or OR processing, and possibly
    # some key-value pairs that cause the where clause to be
    # generated accordingly
    #
    protected method build_select_query {args} {

	set bool AND
	set first 1
	set req ""
	set myTable $table
	set what "*"

	# for each argument passed us...
	# (we go by integers because we mess with the index based on
	#  what we find)
	for {set i 0} {$i < [llength $args]} {incr i} {
	    # fetch the argument we're currently processing
	    set elem [lindex $args $i]

	    switch -- [::string tolower $elem] {
		"-and" { 
		    # -and -- switch to AND-style processing
		    set bool AND 
		}

		"-or"  { 
		    # -or -- switch to OR-style processing
		    set bool OR 
		}

		"-table" { 
		    # -table -- identify which table the query is about
		    set myTable [lindex $args [incr i]] 
		}

		"-select" {
		    # -select - 
		    set what [lindex $args [incr i]]
		}

		default {
		    # it wasn't -and, -or, -table, or -select...

		    # if the first character of the element is a dash,
		    # it's a field name and a value

		    if {[::string index $elem 0] == "-"} {
			set field [::string range $elem 1 end]
			set elem [lindex $args [incr i]]

			# if it's the first field being processed, append
			# WHERE to the SQL request we're generating
			if {$first} {
			    append req " WHERE"
			    set first 0
			} else {
			    # it's not the first variable in the comparison
			    # expression, so append the boolean state, either
			    # AND or OR
			    append req " $bool"
			}

			# convert any asterisks to percent signs in the
			# value field
			regsub -all {\*} $elem {%} elem

			# if there is a percent sign in the value
			# field now (having been there originally or
			# mapped in there a moment ago),  the SQL aspect 
			# is appended with a "field LIKE value"

			if {[::string first {%} $elem] != -1} {
			    append req " $field LIKE '[quote $elem]'"
		        } elseif {[regexp {^([<>]) *([0-9.]*)$} $elem _ fn val]} {
			    # value starts with <, or >, then space, 
			    # and a something
			    append req " $field$fn$val"
		        } elseif {[regexp {^([<>]=) *([0-9.]*)$} $elem _ fn val]} {
			    # value starts with <= or >=, space, and something.
			    append req " $field$fn$val"
			} else {
			    # otherwise it's a straight key=value comparison
			    append req " $field='[quote $elem]'"
			}

			continue
		    }
		    append req " $elem"
		}
	    }
	}
	return "select $what from $myTable $req"
    }

    #
    # build_insert_query -- given an array name, a list of fields, and
    # possibly a table name, return a SQL insert statement inserting
    # into the named table (or the object's table variable, if none
    # is specified) for all of the fields specified, with their values
    # coming from the array
    #
    protected method build_insert_query {arrayName fields {myTable ""}} {
	upvar 1 $arrayName array

	if {[lempty $myTable]} { set myTable $table }
	foreach field $fields {
	    if {![info exists array($field)]} { continue }
	    append vars "$field,"
	    append vals "'[quote $array($field)]',"
	}
	set vals [::string range $vals 0 end-1]
	set vars [::string range $vars 0 end-1]
	return "insert into $myTable ($vars) VALUES ($vals)"
    }

    #
    # build_update_query -- given an array name, a list of fields, and
    # possibly a table name, return a SQL update statement updating
    # the named table (or using object's table variable, if none
    # is named) for all of the fields specified, with their values
    # coming from the array
    #
    # note that after use a where clause still neds to be added or
    # you might update a lot more than you bargained for
    #
    protected method build_update_query {arrayName fields {myTable ""}} {
	upvar 1 $arrayName array
	if {[lempty $myTable]} { set myTable $table }
	foreach field $fields {
	    if {![info exists array($field)]} { continue }
	    append string "$field='[quote $array($field)]',"
	}
	set string [::string range $string 0 end-1]
	return "update $myTable SET $string"
    }

    #
    # lassign_array - given a list, an array name, and a variable number
    # of arguments consisting of variable names, assign each element in
    # the list, in turn, to elements corresponding to the variable
    # arguments, into the named array.  From TclX.
    #
    protected method lassign_array {list arrayName args} {
	upvar 1 $arrayName array
	foreach elem $list field $args {
	    set array($field) $elem
	}
    }

    #
    # configure_variable - given a variable name and a string, if the
    # string is empty return the variable name, otherwise set the
    # variable to the string.
    #
    protected method configure_variable {varName string} {
	if {[lempty $string]} { return [cget -$varName] }
	configure -$varName $string
    }

    #
    # build_where_key_clause - given a list of one or more key fields and 
    # a corresponding list of one or more key values, construct a
    # SQL where clause that boolean ANDs all of the key-value pairs 
    # together.
    #
    protected method build_key_where_clause {myKeyfield myKey} {
	## If we're not using multiple keyfields, just return a simple
	## where clause.
	if {[llength $myKeyfield] < 2} {
	    return " WHERE $myKeyfield = '[quote $myKey]'"
	}

	# multiple fields, construct it as a where-and
	set first 1
	set req ""
	foreach field $myKeyfield key $myKey {
	    if {$first} {
		append req " WHERE $field='[quote $key]'"
		set first 0
	    } else {
		append req " AND $field='[quote $key]'"
	    }
	}
	return $req
    }

    ##
    ## makekey -- Given an array containing a key-value pairs and
    # an optional  list of key fields (we use the object's keyfield
    # if none is specified)...
    #
    # if we're doing auto keys, create and return a new key,
    # otherwise if it's a single key, just return its value
    # from the array, else if it's multiple keys, return all their
    # values as a list
    ##
    method makekey {arrayName {myKeyfield ""}} {
	if {[lempty $myKeyfield]} { set myKeyfield $keyfield }
	if {[lempty $myKeyfield]} {
	    return -code error "No -keyfield specified in object"
	}
	upvar 1 $arrayName array

	## If we're not using multiple keyfields, we want to check and see
	## if we're using auto keys.  If we are, create a new key and
	## return it.  If not, just return the value of the single keyfield
	## in the array.
	if {[llength $myKeyfield] < 2} {
	    if {$autokey} {
		set array($myKeyfield) [$this nextkey]
	    } else {
		if {![info exists array($myKeyfield)]} {
		    return -code error \
			"${arrayName}($myKeyfield) does not exist"
		}
	    }
	    return $array($myKeyfield)
	}

	## We're using multiple keys.  Return a list of all the keyfield
	## values.
	foreach field $myKeyfield {
	    if {![info exists array($field)]} {
		return -code error "$field does not exist in $arrayName"
	    }
	    lappend key $array($field)
	}
	return $key
    }

    method destroy {} {
    	::itcl::delete object $this
    }

    #
    # string - execute a SQL request and only return a string of one row.
    #
    method string {req} {
	set res [exec $req]
	set val [$res next -list]
	$res destroy
	return $val
    }

    #
    # list - execute a request and return a list of the first element of each 
    # row returned.
    #
    method list {req} {
	set res [exec $req]
	set list ""
	$res forall -list line {
	    lappend list [lindex $line 0]
	}
	$res destroy
	return $list
    }

    #
    # array - execute a request and setup an array containing elements
    # with the field names as the keys and the first row results as
    # the values
    #
    method array {req arrayName} {
	upvar 1 $arrayName $arrayName
	set res [exec $req]
	set ret [$res next -array $arrayName]
	$res destroy
	return $ret
    }

    #
    # forall - execute a SQL select and iteratively fill the named array 
    # with elements named with the matching field names, containing the 
    # matching values, executing the specified code body for each, in turn.
    #
    method forall {req arrayName body} {
	upvar 1 $arrayName $arrayName

	set res [exec $req]

	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}

        set ret [$res numrows]
	$res forall -array $arrayName {
	    uplevel 1 $body
        }
	$res destroy
	return $ret
    }

    #
    # table_check - internal method to populate the data array with
    # a -table element containing the table name, a -keyfield element
    # containing the key field or list of key fields, and a list of
    # key-value pairs to get set into the data table.
    #
    # afterwards, it's an error if -table or -keyfield hasn't somehow been
    # determined.
    #
    protected method table_check {list {tableVar myTable} {keyVar myKeyfield}} {
	upvar 1 $tableVar $tableVar $keyVar $keyVar
	set data(-table) $table
	set data(-keyfield) $keyfield
	::array set data $list

	if {[lempty $data(-table)]} {
	    return -code error "-table not specified in DIO object"
	}
	if {[lempty $data(-keyfield)]} {
	    return -code error "-keyfield not specified in DIO object"
	}

	set $tableVar $data(-table)
	set $keyVar   $data(-keyfield)
    }

    #
    # key_check - given a list of key fields and a list of keys, it's
    # an error if there aren't the same number of each, and if it's
    # autokey, there can't be more than one key.
    #
    protected method key_check {myKeyfield myKey} {
	if {[llength $myKeyfield] < 2} { return }
	if {$autokey} {
	    return -code error "Cannot have autokey and multiple keyfields"
	}
	if {[llength $myKeyfield] != [llength $myKey]} {
	    return -code error "Bad key length."
	}
    }

    #
    # fetch - given a key (or list of keys) an array name, and some
    # extra key-value arguments like -table and -keyfield, fetch
    # the key into the array
    #
    method fetch {key arrayName args} {
	table_check $args
	key_check $myKeyfield $key
	upvar 1 $arrayName $arrayName
	set req "select * from $myTable"
	append req [build_key_where_clause $myKeyfield $key]

	set res [$this exec $req]
	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}
	set return [expr [$res numrows] > 0]
	$res next -array $arrayName
	$res destroy
	return $return
    }

    #
    # store - given an array containing key-value pairs and optional
    # arguments like -table and -keyfield, insert or update the
    # corresponding table entry.
    #
    method store {arrayName args} {
	table_check $args
	upvar 1 $arrayName $arrayName $arrayName array
	if {[llength $myKeyfield] > 1 && $autokey} {
	    return -code error "Cannot have autokey and multiple keyfields"
	}

	set key [makekey $arrayName $myKeyfield]
	set req "select * from $myTable"
	append req [build_key_where_clause $myKeyfield $key]
	set res [exec $req]
	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}
	set numrows [$res numrows]
	set fields  [$res fields]
	$res destroy

	if {$numrows} {
	    set req [build_update_query array $fields $myTable]
	    append req [build_key_where_clause $myKeyfield $key]
	} else {
	    set req [build_insert_query array $fields $myTable]
	}

	set res [exec $req]
	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}
	$res destroy
	return 1
    }

    #
    # insert - a pure insert, without store's somewhat clumsy
    # efforts to see if it needs to be an update rather than
    # an insert -- this shouldn't require fields, it's broken
    #
    method insert {arrayName fields args} {
	table_check $args
	upvar 1 $arrayName $arrayName $arrayName array
	set req [build_insert_query array $fields $myTable]

	set res [exec $req]
	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}
	$res destroy
	return 1
    }

    #
    # delete - delete matching record from the specified table
    #
    method delete {key args} {
	table_check $args
	set req "delete from $myTable"
	append req [build_key_where_clause $myKeyfield $key]

	set res [exec $req]
	if {[$res error]} {
	    set errinf [$res errorinfo]
	    $res destroy
	    return -code error "Got '$errinf' executing '$req'"
	}

	set return [$res numrows]
	$res destroy
	return $return
    }

    #
    # keys - return all keys in a tbale
    #
    method keys {args} {
	table_check $args
	set req "select * from $myTable"
	set obj [$this exec $req]

	set keys ""
	$obj forall -array a {
	    lappend keys [makekey a $myKeyfield]
	}
	$obj destroy

	return $keys
    }

    #
    # search - construct and execute a SQL select statement using
    # build_select_query style and return the result handle.
    #
    method search {args} {
	set req [eval build_select_query $args]
	return [exec $req]
    }

    #
    # count - return a count of the specified (or current) table.
    #
    method count {args} {
	table_check $args
	return [string "select count(*) from $myTable"]
    }

    ##
    ## These are methods which should be defined by each individual database
    ## interface class.
    ##
    method open    {args} {}
    method close   {args} {}
    method exec    {args} {}
    method nextkey {args} {}
    method lastkey {args} {}

    ##
    ## Functions to get and set public variables.
    ##
    method interface {{string ""}} { configure_variable interface $string }
    method errorinfo {{string ""}} { configure_variable errorinfo $string }
    method db {{string ""}} { configure_variable db $string }
    method table {{string ""}} { configure_variable table $string }
    method keyfield {{string ""}} { configure_variable keyfield $string }
    method autokey {{string ""}} { configure_variable autokey $string }
    method sequence {{string ""}} { configure_variable sequence $string }
    method user {{string ""}} { configure_variable user $string }
    method pass {{string ""}} { configure_variable pass $string }
    method host {{string ""}} { configure_variable host $string }
    method port {{string ""}} { configure_variable port $string }

    public variable interface	""
    public variable errorinfo	""

    public variable db		""
    public variable table	""
    public variable sequence	""

    public variable user	""
    public variable pass	""
    public variable host	""
    public variable port	""

    public variable keyfield	"" {
	if {[llength $keyfield] > 1 && $autokey} {
	    return -code error "Cannot have autokey and multiple keyfields"
	}
    }

    public variable autokey	0 {
	if {[llength $keyfield] > 1 && $autokey} {
	    return -code error "Cannot have autokey and multiple keyfields"
	}
    }

} ; ## ::itcl::class Database

#
# DIO Result object
#
::itcl::class Result {
    constructor {args} {
	eval configure $args
    }

    destructor { }

    method destroy {} {
	::itcl::delete object $this
    }

    #
    # configure_variable - given a variable name and a string, if the
    # string is empty return the variable name, otherwise set the
    # variable to the string.
    #
    protected method configure_variable {varName string} {
	if {[lempty $string]} { return [cget -$varName] }
	configure -$varName $string
    }

    #
    # lassign_array - given a list, an array name, and a variable number
    # of arguments consisting of variable names, assign each element in
    # the list, in turn, to elements corresponding to the variable
    # arguments, into the named array.  From TclX.
    #
    protected method lassign_array {list arrayName args} {
	upvar 1 $arrayName array
	foreach elem $list field $args {
	    set array($field) $elem
	}
    }

    #
    # seek - set the current row ID (our internal row cursor, if you will)
    # to the specified row ID
    #
    method seek {newrowid} {
	set rowid $newrowid
    }

    method cache {{size "all"}} {
	set cacheSize $size
	if {$size == "all"} { set cacheSize $numrows }

	## Delete the previous cache array.
	catch {unset cacheArray}

	set autostatus $autocache
	set currrow    $rowid
	set autocache 1
	seek 0
	set i 0
	while {[next -list list]} {
	    if {[incr i] >= $cacheSize} { break }
	}
	set autocache $autostatus
	seek $currrow
	set cached 1
    }

    #
    # forall -- walk the result object, executing the code body over it
    #
    method forall {type varName body} {
	upvar 1 $varName $varName
	set currrow $rowid
	seek 0
	while {[next $type $varName]} {
	    uplevel 1 $body
	}
	set rowid $currrow
	return
    }

    method next {type {varName ""}} {
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

    method resultid {{string ""}} { configure_variable resultid $string }
    method fields {{string ""}} { configure_variable fields $string }
    method rowid {{string ""}} { configure_variable rowid $string }
    method numrows {{string ""}} { configure_variable numrows $string }
    method error {{string ""}} { configure_variable error $string }
    method errorcode {{string ""}} { configure_variable errorcode $string }
    method errorinfo {{string ""}} { configure_variable errorinfo $string }
    method autocache {{string ""}} { configure_variable autocache $string }

    public variable resultid	""
    public variable fields	""
    public variable rowid	0
    public variable numrows	0
    public variable error	0
    public variable errorcode	0
    public variable errorinfo	""
    public variable autocache	1

    protected variable cached		0
    protected variable cacheSize	0
    protected variable cacheArray

} ; ## ::itcl::class Result

} ; ## namespace eval DIO

package provide DIO 1.0
