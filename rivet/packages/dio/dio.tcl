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

    protected method result {interface args} {
	return [eval uplevel \#0 ::DIO::${interface}Result \#auto $args]
    }

    method quote {string} {
	regsub -all {'} $string {\'} string
	return $string
    }

    protected method build_select_query {args} {
	set bool AND
	set first 1
	set req ""
	set myTable $table
	for {set i 0} {$i < [llength $args]} {incr i} {
	    set elem [lindex $args $i]

	    switch -- [::string tolower $elem] {
		"-and" { set bool AND }
		"-or"  { set bool OR }

		"-table" { set myTable [lindex $args [incr i]] }

		default {
		    if {[::string index $elem 0] == "-"} {
			set field [::string range $elem 1 end]
			set elem [lindex $args [incr i]]

			if {$first} {
			    append req " WHERE"
			    set first 0
			} else {
			    append req " $bool"
			}
			regsub -all {\*} $elem {%} elem
			if {[::string first {%} $elem] != -1} {
			    append req " $field LIKE '[quote $elem]'"
			} else {
			    append req " $field='[quote $elem]'"
			}

			continue
		    }
		    append req " $elem"
		}
	    }
	}
	return "select * from $myTable $req"
    }

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

    protected method lassign_array {list arrayName args} {
	upvar 1 $arrayName array
	foreach elem $list field $args {
	    set array($field) $elem
	}
    }

    protected method configure_variable {varName string} {
	if {[lempty $string]} { return [cget -$varName] }
	configure -$varName $string
    }

    protected method build_key_where_clause {myKeyfield myKey} {
	## If we're not using multiple keyfields, just return a simple
	## where clause.
	if {[llength $myKeyfield] < 2} {
	    return " WHERE $myKeyfield = '[quote $myKey]'"
	}

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
    ## Given an array of values, return a key that would be used in this
    ## database to store this array.
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

    ###
    ## Execute a request and only return a string of the row.
    ###
    method string {req} {
	set res [exec $req]
	set val [$res next -list]
	$res destroy
	return $val
    }

    ###
    ## Execute a request and return a list of the first element of each row.
    ###
    method list {req} {
	set res [exec $req]
	set list ""
	$res forall -list line {
	    lappend list [lindex $line 0]
	}
	$res destroy
	return $list
    }

    ###
    ## Execute a request and setup an array with the row fetched.
    ###
    method array {req arrayName} {
	upvar 1 $arrayName $arrayName
	set res [exec $req]
	set ret [$res next -array $arrayName]
	$res destroy
	return $ret
    }

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

    protected method key_check {myKeyfield myKey} {
	if {[llength $myKeyfield] < 2} { return }
	if {$autokey} {
	    return -code error "Cannot have autokey and multiple keyfields"
	}
	if {[llength $myKeyfield] != [llength $myKey]} {
	    return -code error "Bad key length."
	}
    }

    method fetch {key arrayName args} {
	table_check $args
	key_check $myKeyfield $key
	upvar 1 $arrayName $arrayName
	set req "select * from $myTable"
	append req [build_key_where_clause $myKeyfield $key]
	set res [$this exec $req]
	if {[$res error]} {
	    $res destroy
	    return 0
	}
	set return [expr [$res numrows] > 0]
	$res next -array $arrayName
	$res destroy
	return $return
    }

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
	    $res destroy
	    return 0
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
	set return [expr [$res error] == 0]
	$res destroy
	return $return
    }

    method delete {key args} {
	table_check $args
	set req "delete from $myTable"
	append req [build_key_where_clause $myKeyfield $key]
	set res [exec $req]
	set return [expr [$res error] == 0]
	$res destroy
	return $return
    }

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

    method search {args} {
	set req [eval build_select_query $args]
	return [exec $req]
    }

    method count {args} {
	table_check $args
	return [string "select count(*) from $myTable"]
    }

    ##
    ## These are methods which should be defined by each individual database
    ## class.
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

::itcl::class Result {
    constructor {args} {
	eval configure $args
    }

    destructor { }

    method destroy {} {
	::itcl::delete object $this
    }

    protected method configure_variable {varName string} {
	if {[lempty $string]} { return [cget -$varName] }
	configure -$varName $string
    }

    protected method lassign_array {list arrayName args} {
	upvar 1 $arrayName array
	foreach elem $list field $args {
	    set array($field) $elem
	}
    }

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
