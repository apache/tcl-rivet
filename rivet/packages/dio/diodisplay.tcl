# diodisplay.tcl --

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
#
# $Id$
#

package require Itcl
package require DIO
package require form

package provide DIODisplay 1.0

catch { ::itcl::delete class DIODisplay }

::itcl::class ::DIODisplay {
    constructor {args} {
	eval configure $args
	load_response

	if {[lempty $DIO]} {
	    return -code error "You must specify a DIO object"
	}

	if {[lempty $form]} {
	    set form [namespace which [::form #auto -defaults response]]
	}

	set document [env DOCUMENT_NAME]

	if {[info exists response(num)] \
	    && ![lempty $response(num)]} {
	    set pagesize $response(num)
	}

	read_css_file
    }

    destructor {
	if {$cleanup} { do_cleanup }
    }

    method destroy {} {
	::itcl::delete object $this
    }

    #
    # configvar - a convenient helper for creating methods that can
    #  set and fetch one of the object's variables
    #
    method configvar {varName string} {
	if {[lempty $string]} { return [set $varName] }
	configure -$varName $string
    }

    #
    # is_function - return true if name is known to be a function
    # such as Search List Add Edit Delete Details Main Save DoDelete Cancel
    # etc.
    #
    method is_function {name} {
	if {[lsearch $functions $name] > -1} { return 1 }
	if {[lsearch $allfunctions $name] > -1} { return 1 }
	return 0
    }

    #
    # do_cleanup - clean up our field subobjects, DIO objects, forms, and the 
    # like.
    #
    method do_cleanup {} {
	## Destroy all the fields created.
	foreach field $allfields { catch { $field destroy } }

	## Destroy the DIO object.
	catch { $DIO destroy }

	## Destroy the form object.
	catch { $form destroy }
    }

    #
    # handle_error - emit an error message
    #
    method handle_error {error} {
	puts "<B>An error has occurred processing your request</B>"
	puts "<PRE>"
	puts "$error"
	puts ""
	puts "$::errorInfo"
	puts "</PRE>"
    }

    #
    # read_css_file - parse and read in a CSS file so we can
    #  recognize CSS info and emit it in appropriate places
    #
    method read_css_file {} {
	if {[lempty $css]} { return }
	if {[catch {open [virtual_filename $css]} fp]} { return }
	set contents [read $fp]
	close $fp
	array set tmpArray $contents
	foreach class [array names tmpArray] {
	    set cssArray([string toupper $class]) $tmpArray($class)
	}
    }

    #
    # get_css_class - figure out which CSS class we want to use.  
    # If class exists, we use that.  If not, we use default.
    #
    method get_css_class {tag default class} {

	# if tag.class exists, use that
	if {[info exists cssArray([string toupper $tag.$class])]} {
	    return $class
	}

	# if .class exists, use that
	if {[info exists cssArray([string toupper .$class])]} { 
	    return $class 
	}

	# use the default
	return $default
    }

    #
    # parse_css_class - given a class and the name of an array, parse
    # the named CSS class (read from the style sheet) and return it as
    # key-value pairs in the named array.
    #
    method parse_css_class {class arrayName} {

	# if we don't have an entry for the specified glass, give up
	if {![info exists cssArray($class)]} { 
	    return
        }

	# split CSS entry on semicolons, for each one...
	upvar 1 $arrayName array
	foreach line [split $cssArray($class) \;] {

	    # trim leading and trailing spaces
	    set line [string trim $line]

	    # split the line on a colon into property and value
	    lassign [split $line :] property value

	    # map the property to space-trimmed lowercase and
	    # space-trim the value, then store in the passed array
	    set property [string trim [string tolower $property]]
	    set value [string trim $value]
	    set array($property) $value
	}
    }

    #
    # button_image_src - return the value of the image-src element in
    # the specified class (from the CSS style sheet), or an empty
    # string if there isn't one.
    #
    method button_image_src {class} {
	set class [string toupper input.$class]
	parse_css_class $class array
	if {![info exists array(image-src)]} { 
	    return 
	}
	return $array(image-src)
    }

    # state - return a list of name-value pairs that represents the current
    # state of the query, which can be used to properly populate links
    # outside DIOdisplay.
    method state {} {
	set state {}
	foreach fld {mode query by how sort num page} {
	    if [info exists response($fld)] {
		lappend state $fld $response($fld)
	    }
	}
	return $state
    }

    method show {} {

	# if there's a mode in the response array, use that, else set
	# mode to Main
	set mode Main
	if {[info exists response(mode)]} {
	    set mode $response(mode)
	}

	# if there is a style sheet defined, emit HTML to reference it
	if {![lempty $css]} {
	    puts "<LINK REL=\"stylesheet\" TYPE=\"text/css\" HREF=\"$css\">"
	}

	# put out the table header
	puts {<TABLE WIDTH="100%" CLASS="DIO">}
	puts "<TR>"
	puts {<TD VALIGN="center" CLASS="DIO">}

	# if mode isn't Main and persistentmain is set (the default),
	# run Main
	if {$mode != "Main" && $persistentmain} { 
	    Main 
	}

	if {![is_function $mode]} {
	    puts "In-valid function"
	    return
	}

	if {[catch "$this $mode" error]} {
	    handle_error $error
	}

	puts "</TD>"
	puts "</TR>"
	puts "</TABLE>"

	if {$cleanup} { destroy }
    }

    method showview {} {
	puts {<TABLE CLASS="DIOView">}
	set row 0
	foreach field $fields {
	    $field showview [lindex {"" "Alt"} $row]
	    set row [expr 1 - $row]
	}
	puts "</TABLE>"
    }

    #
    # showform_prolog - emit a form for inserting a new record
    #
    # response(by) will contain whatever was in the "where" field
    # response(query) will contain whatever was in the "is" field
    #
    method showform_prolog {{args ""}} {
	get_field_values array

	eval $form start $args
	foreach fld [array names hidden] {
	    $form hidden $fld -value $hidden($fld)
        }
	$form hidden mode -value Save
	$form hidden DIODfromMode -value $response(mode)
	$form hidden DIODkey -value [$DIO makekey array]
	puts {<TABLE CLASS="DIOForm">}
    }

    method showform_epilog {} {
	set save [button_image_src DIOFormSaveButton]
	set cancel [button_image_src DIOFormCancelButton]

	puts "</TABLE>"

	puts "<TABLE>"
	puts "<TR>"
	puts "<TD>"
	if {![lempty $save]} {
	    $form image save -src $save -class DIOFormSaveButton
	} else {
	    $form submit save.x -value "Save" -class DIOFormSaveButton
	}
	puts "</TD>"
	puts "<TD>"
	if {![lempty $cancel]} {
	    $form image cancel -src $cancel -class DIOFormSaveButton
	} else {
	    $form submit cancel.x -value "Cancel" -class DIOFormCancelButton
	}
	puts "</TD>"
	puts "</TR>"
	puts "</TABLE>"

	$form end
    }

    #
    # showform - emit a form for inserting a new record
    #
    # response(by) will contain whatever was in the "where" field
    # response(query) will contain whatever was in the "is" field
    #
    method showform {} {
	showform_prolog

	# emit each field
	foreach field $fields {
	    showform_field $field
	}

	showform_epilog
    }

    # showform_field - emit the form field for the specified field using 
    # the showform method of the field.  If the user has typed something 
    # into the search field and it matches the fields being emitted,
    # use that value as the default
    #
    method showform_field {field} {
	if {[info exists response(by)] && $response(by) == [$field text]} {
	    if {![$field readonly] && $response(query) != ""} {
		$field value $response(query)
	    }
	}
	$field showform
    }

    method page_buttons {end {count 0}} {
	if {$pagesize <= 0} { return }

	if {![info exists response(page)]} { set response(page) 1 }

	set pref DIO$end
	if {!$count} {
	  set count [$DIOResult numrows]
	}

	set pages [expr ($count + $pagesize - 1) / $pagesize]

	if {$pages <= 1} {
	  return
	}

	set first [expr $response(page) - 4]
	if {$first > $pages - 9} {
	  set first [expr $pages - 9]
	}
        if {$first > 1} {
	  lappend pagelist 1 1
	  if {$first > 3} {
	    lappend pagelist ".." 0
	  } elseif {$first > 2} {
	    lappend pagelist 2 2
	  }
	} else {
	  set first 1
	}
	set last [expr $response(page) + 4]
	if {$last < 9} {
	  set last 9
	}
	if {$last > $pages} {
	  set last $pages
	}
	for {set i $first} {$i <= $last} {incr i} {
	  lappend pagelist $i $i
	}
	if {$last < $pages} {
	  if {$last < $pages - 2} {
	    lappend pagelist ".." 0
	  } elseif {$last < $pages - 1} {
	    incr last
	    lappend pagelist $last $last
	  }
	  lappend pagelist $pages $pages
	}

	foreach {n p} $pagelist {
	  if {$p == 0 || $p == $response(page)} {
	    lappend navbar $n
	  } else {
	    set html {<A HREF="}
	    append html "$document?mode=$response(mode)"
	    foreach var {query by how sort num} {
	      if {[info exists response($var)]} {
	      append html "&$var=$response($var)"
	      }
	    }
	    foreach fld [array names hidden] {
	      append html "&$fld=$hidden($fld)"
            }
	    append html "&page=$p\">$n</A>"
	    lappend navbar $html 
	  }
	}

	if {"$end" == "Bottom"} {
	  puts "<BR/>"
	}
	set class [get_css_class TABLE DIONavButtons ${pref}NavButtons]
	puts "<TABLE WIDTH=\"100%\" CLASS=\"$class\">"
	puts "<TR>"
        puts "<TD>"
	if {"$end" == "Top"} {
	  puts "$count rows, go to page"
	} else {
	  puts "Go to page"
	}
	foreach link $navbar {
	  puts "$link&nbsp;"
	}
	puts "</TD>"
	if {"$end" == "Top" && $pages>10} {
	  set f [::form #auto]
	  $f start
	  foreach fld [array names hidden] {
	      $f hidden $fld -value $hidden($fld)
          }
	  foreach fld {mode query by how sort num} {
	    if [info exists response($fld)] {
	      $f hidden $fld -value $response($fld)
	    }
	  }
	  puts "<TD ALIGN=RIGHT>"
	  puts "Jump directly to"
	  $f text page -size 4 -value $response(page)
	  $f submit submit -value "Go"
	  puts "</TD>"
	  $f end
	}
	puts "</TR>"
	puts "</TABLE>"
	if {"$end" == "Top"} {
	  puts "<BR/>"
	}
    }


    method rowheader {{total 0}} {
	set fieldList $fields
	if {![lempty $rowfields]} { set fieldList $rowfields }

	set rowcount 0

	puts <P>

	if {$topnav} { page_buttons Top $total }

	puts {<TABLE BORDER WIDTH="100%" CLASS="DIORowHeader">}
	puts "<TR CLASS=DIORowHeader>"
	foreach field $fieldList {
	    set text [$field text]
	    set sorting $allowsort
	    ## If sorting is turned off, or this field is not in the
	    ## sortfields, we don't display the sort option.
	    if {$sorting && ![lempty $sortfields]} {
		if {[lsearch $sortfields $field] < 0} {
		    set sorting 0
	        }
	    }
	    if {$sorting && [info exists response(sort)]} {
		if {"$response(sort)" == "$field"} {
		    set sorting 0
	        }
	    }

	    if {!$sorting} {
		set html $text
	    } else {
		set html {<A HREF="}
		append html "$document?mode=$response(mode)"
		foreach var {query by how num} {
		    if {[info exists response($var)]} {
			append html "&$var=$response($var)"
		    }
		}
	        foreach fld [array names hidden] {
	            append html "&$fld=$hidden($fld)"
                }
		append html "&sort=$field\">$text</A>"
	    }
	    set class [get_css_class TH DIORowHeader DIORowHeader-$field]
	    puts "<TH CLASS=\"$class\">$html</TH>"
	}

	if {![lempty $rowfunctions] && "$rowfunctions" != "-"} {
	  puts {<TH CLASS="DIORowHeaderFunctions">Functions</TH>}
        }
	puts "</TR>"
    }

    method showrow {arrayName} {
	upvar 1 $arrayName a

	incr rowcount
	set alt ""
	if {$alternaterows && ![expr $rowcount % 2]} { set alt Alt }

	set fieldList $fields
	if {![lempty $rowfields]} { set fieldList $rowfields }

	puts "<TR>"
	foreach field $fieldList {
	    set class [get_css_class TD DIORowField$alt DIORowField$alt-$field]
	    set text ""
	    if {[info exists a($field)]} {
	        set text $a($field)
		if [info exists filters($field)] {
		    set text [$filters($field) $text]
		}
	    }
	    if ![string length $text] {
		set text "&nbsp;"
	    }
	    puts "<TD CLASS=\"$class\">$text</TD>"
	}

	if {![lempty $rowfunctions] && "$rowfunctions" != "-"} {
	    set f [::form #auto]
	    puts "<TD NOWRAP CLASS=\"DIORowFunctions$alt\">"
	    $f start
	    foreach fld [array names hidden] {
	        $f hidden $fld -value $hidden($fld)
            }
	    $f hidden query -value [$DIO makekey a]
	    if {[llength $rowfunctions] > 1} {
	      $f select mode -values $rowfunctions -class DIORowFunctionSelect$alt
	      $f submit submit -value "Go" -class DIORowFunctionButton$alt
	    } else {
	      set func [lindex $rowfunctions 0]
	      $f hidden mode -value $func
	      $f submit submit -value $func -class DIORowFunctionButton$alt
	    }
	    puts "</TD>"
	    $f end
	}

	puts "</TR>"
    }

    method rowfooter {{total 0}} {
	puts "</TABLE>"

	if {$bottomnav} { page_buttons Bottom $total }
    }

    ## Define a new function.
    method function {name} {
	lappend allfunctions $name
    }

    ## Define a field in the object.
    method field {name args} {
	import_keyvalue_pairs data $args
	lappend fields $name
	lappend allfields $name

	set class DIODisplayField
	if {[info exists data(type)]} {
	    if {![lempty [::itcl::find classes *DIODisplayField_$data(type)]]} {
		set class DIODisplayField_$data(type)
	    }

	}

	eval $class $name -name $name -display $this -form $form $args
	set FieldTextMap([$name text]) $name
    }

    method fetch {key arrayName} {
	upvar 1 $arrayName $arrayName
	set result [$DIO fetch $key $arrayName]
	set error  [$DIO errorinfo]
	if {![lempty $error]} { return -code error $error }
	return $result
    }

    method store {arrayName} {
	upvar 1 $arrayName array
	set result [$DIO store array]
	set error  [$DIO errorinfo]
	if {![lempty $error]} { return -code error $error }
	return $result
    }

    method delete {key} {
	set result [$DIO delete $key]
	set error  [$DIO errorinfo]
	if {![lempty $error]} { return -code error $error }
	return $result
    }

    method pretty_fields {list} {
	foreach field $list {
	    lappend fieldList [$field text]
	}
	return $fieldList
    }

    method set_field_values {arrayName} {
	upvar 1 $arrayName array

	# for all the elements in the specified array, try to invoke
	# the element as an object, invoking the method "value" to
	# set the value to the specified value
	foreach var [array names array] {
	    #if {[catch { $var value $array($var) } result] == 1} {}
	    if {[catch { $var configure -value $array($var) } result] == 1} {
	    }
	}
    }

    method get_field_values {arrayName} {
	upvar 1 $arrayName array

	foreach field $allfields {

            # for some reason the method for getting the value doesn't
	    # work for boolean values, which inherit DIODisplayField,
	    # something to do with configvar
	    #set array($field) [$field value]
	    set array($field) [$field cget -value]
	}
    }

    method DisplayRequest {query} {
	set DIOResult [eval $DIO search -select "count(*)" $query]
	if [$DIOResult numrows] {
	  $DIOResult next -array a
	  set total $a(count)
	} else {
	  set total 0
	}
	$DIOResult destroy
	set DIOResult ""

	append query [sql_order_by_syntax]
	append query [sql_limit_syntax]
	set DIOResult [eval $DIO search $query]

	if {[$DIOResult numrows] <= 0} {
	    puts "Could not find any matching records."
	    $DIOResult destroy
	    set DIOResult ""
	    return
	}

	rowheader $total

	$DIOResult forall -array a {
	    showrow a
	}

	rowfooter $total

	$DIOResult destroy
	set DIOResult ""
    }

    method Main {} {
	puts "<TABLE BORDER=0 WIDTH=100% CLASS=DIOForm><TR>"

	set selfunctions {}
	foreach f $functions {
	    if {"$f" != "List"} {
	        lappend selfunctions $f
	    } else {
	    	set f [::form #auto]
	    	$f start
	    	foreach fld [array names hidden] {
	        	$f hidden $fld -value $hidden($fld)
            	}
	    	$f hidden mode -value "List"
	    	$f hidden query -value ""
	        puts "<TD CLASS=DIOForm ALIGN=CENTER VALIGN=MIDDLE WIDTH=0%>"
	    	$f submit submit -value "Show All" -class DIORowFunctionButton
		puts "</TD>"
	    	$f end
	    }
	}

	puts "<TD CLASS=DIOForm VALIGN=MIDDLE WIDTH=100%>"
	$form start
	puts "&nbsp;"

	foreach fld [array names hidden] {
	    $form hidden $fld -value $hidden($fld)
        }

        if {[llength $selfunctions] > 1} {
	  $form select mode -values $selfunctions -class DIOMainFunctionsSelect
          puts "where"
	} else {
	  puts "Where"
	}

	set useFields $fields
	if {![lempty $searchfields]} { set useFields $searchfields }

	$form select by -values [pretty_fields $useFields] \
	    -class DIOMainSearchBy

	if [string match {[Ss]earch} $selfunctions] {
	  $form select how -values {"=" "<" "<=" ">" ">="}
	} else {
          puts "is"
	}

        if [info exists response(query)] {
	  $form text query -value $response(query) -class DIOMainQuery
	} else {
	  $form text query -value "" -class DIOMainQuery
	}

        if {[llength $selfunctions] > 1} {
	  $form submit submit -value "GO" -class DIOMainSubmitButton
	} else {
	  $form hidden mode -value $selfunctions
	  $form submit submit -value $selfunctions -class DIOMainSubmitButton
	}
	puts "</TD></TR>"

	if {![lempty $numresults]} {
	    puts "<TR><TD CLASS=DIOForm>Results per page: "
	    $form select num -values $numresults -class DIOMainNumResults
	    puts "</TD></TR>"
	}

	$form end
	puts "</TABLE>"
    }

    method sql_order_by_syntax {} {
	if {[info exists response(sort)] && ![lempty $response(sort)]} {
	    return " ORDER BY $response(sort)"
	}

	if {![lempty $defaultsortfield]} {
	    return " ORDER BY $defaultsortfield"
	}
    }

    method sql_limit_syntax {} {
	if {$pagesize <= 0} { return }

	set offset ""
	if {[info exists response(page)]} {
	    set offset [expr ($response(page) - 1) * $pagesize]
	}
	return [$DIO sql_limit_syntax $pagesize $offset]
    }
	

    method Search {} {
	set searchField $FieldTextMap($response(by))	

	set what $response(query)
	if {[info exists response(how)] && [string length $response(how)]} {
	  set what "$response(how)$what"
	}

	DisplayRequest "-$searchField $what"
    }

    method List {} {
	DisplayRequest ""
    }

    method Add {} {
	showform
    }

    method Edit {} {
	if {![fetch $response(query) array]} {
	    puts "That record does not exist in the database."
	    return
	}

	set_field_values array

	showform
    }

    ##
    ## When we save, we want to set all the fields' values and then get
    ## them into a new array.  We do this because we want to clean any
    ## unwanted variables out of the array but also because some fields
    ## have special handling for their values, and we want to make sure
    ## we get the right value.
    ##
    method Save {} {
	if {[info exists response(cancel.x)]} {
	    Cancel
	    return
	}

	## We need to see if the key exists.  If they are adding a new
	## entry, we just want to see if the key exists.  If they are
	## editing an entry, we need to see if they changed the keyfield
	## while editing.  If they didn't change the keyfield, there's no
	## reason to check it.
	if {$response(DIODfromMode) == "Add"} {
	    set key [$DIO makekey response]
	    fetch $key a
	} else {
	    set key $response(DIODkey)
	    set newkey [$DIO makekey response]

	    ## If we have a new key, and the newkey doesn't exist in the
	    ## database, we are moving this record to a new key, so we
	    ## need to delete the old key.
	    if {$key != $newkey} {
		if {![fetch $newkey a]} {
		    delete $key
		}
	    }
	}

	if {[array exists a]} {
	    puts "That record already exists in the database."
	    return
	}

	set_field_values response
	get_field_values storeArray
	store storeArray
	headers redirect $document
    }

    method Delete {} {
	if {![fetch $response(query) array]} {
	    puts "That record does not exist in the database."
	    return
	}

	if {!$confirmdelete} {
	    DoDelete
	    return
	}

	puts "<CENTER>"
	puts {<TABLE CLASS="DIODeleteConfirm">}
	puts "<TR>"
	puts {<TD COLSPAN=2 CLASS="DIODeleteConfirm">}
	puts "Are you sure you want to delete this record from the database?"
	puts "</TD>"
	puts "</TR>"
	puts "<TR>"
	puts {<TD ALIGN="center" CLASS="DIODeleteConfirmYesButton">}
	set f [::form #auto]
	$f start
	foreach fld [array names hidden] {
	    $f hidden $fld -value $hidden($fld)
        }
	$f hidden mode -value DoDelete
	$f hidden query -value $response(query)
	$f submit submit -value Yes -class DIODeleteConfirmYesButton
	$f end
	puts "</TD>"
	puts {<TD ALIGN="center" CLASS="DIODeleteConfirmNoButton">}
	set f [::form #auto]
	$f start
	foreach fld [array names hidden] {
	    $f hidden $fld -value $hidden($fld)
        }
	$f submit submit -value No -class "DIODeleteConfirmNoButton"
	$f end
	puts "</TD>"
	puts "</TR>"
	puts "</TABLE>"
	puts "</CENTER>"
    }

    method DoDelete {} {
	delete $response(query)

	headers redirect $document
    }

    method Details {} {
	if {![fetch $response(query) array]} {
	    puts "That record does not exist in the database."
	    return
	}

	set_field_values array

	showview
    }

    method Cancel {} {
	headers redirect $document
    }

    ###
    ## Define variable functions for each variable.
    ###

    method fields {{list ""}} {
	if {[lempty $list]} { return $fields }
	foreach field $list {
	    if {[lsearch $allfields $field] < 0} {
		return -code error "Field $field does not exist."
	    }
	}
	set fields $list
    }

    method searchfields {{list ""}} {
	if {[lempty $list]} { return $searchfields }
	foreach field $list {
	    if {[lsearch $allfields $field] < 0} {
		return -code error "Field $field does not exist."
	    }
	}
	set searchfields $list
    }

    method rowfields {{list ""}} {
	if {[lempty $list]} { return $rowfields }
	foreach field $list {
	    if {[lsearch $allfields $field] < 0} {
		return -code error "Field $field does not exist."
	    }
	}
	set rowfields $list
    }

    method filter {field {value ""}} {
	if [string length $value] {
	    set filters($field) [uplevel 1 [list namespace which $value]]
	} else {
	    if [info exists filters($field)] {
		return $filters($field)
	    } else {
		return ""
	    }
	}
    }

    method hidden {name {value ""}} {
	if [string length $value] {
	    set hidden($name) $value
	} else {
	    if [info exists hidden($name)] {
		return $hidden($name)
	    } else {
		return ""
	    }
	}
    }

    method DIO {{string ""}} { configvar DIO $string }
    method DIOResult {{string ""}} { configvar DIOResult $string }

    method title {{string ""}} { configvar title $string }
    method functions {{string ""}} { configvar functions $string }
    method pagesize {{string ""}} { configvar pagesize $string }
    method form {{string ""}} { configvar form $string }
    method cleanup {{string ""}} { configvar cleanup $string }
    method confirmdelete {{string ""}} { configvar confirmdelete $string }

    method css {{string ""}} { configvar css $string }
    method persistentmain {{string ""}} { configvar persistentmain $string }
    method alternaterows {{string ""}} { configvar alternaterows $string }
    method allowsort {{string ""}} { configvar allowsort $string }
    method sortfields {{string ""}} { configvar sortfields $string }
    method topnav {{string ""}} { configvar topnav $string }
    method bottomnav {{string ""}} { configvar bottomnav $string }
    method numresults {{string ""}} { configvar numresults $string }
    method defaultsortfield {{string ""}} { configvar defaultsortfield $string }

    method rowfunctions {{string ""}} { configvar rowfunctions $string }

    ## OPTIONS ##

    public variable DIO		 ""
    public variable DIOResult	 ""

    public variable title	 ""
    public variable fields	 ""
    public variable searchfields ""
    public variable functions	 "Search List Add Edit Delete Details"
    public variable pagesize	 25
    public variable form	 ""
    public variable cleanup	 1
    public variable confirmdelete 1

    public variable css			"diodisplay.css" {
	if {![lempty $css]} {
	    catch {unset cssArray}
	    read_css_file
	}
    }

    public variable persistentmain	1
    public variable alternaterows	1
    public variable allowsort		1
    public variable sortfields		""
    public variable topnav		1
    public variable bottomnav		1
    public variable numresults		""
    public variable defaultsortfield	""

    public variable rowfields	 ""
    public variable rowfunctions "Details Edit Delete"

    public variable response
    public variable cssArray
    public variable document	 ""
    public variable allfields    ""
    public variable FieldTextMap
    public variable allfunctions {
	Search
	List
	Add
	Edit
	Delete
	Details
	Main
	Save
	DoDelete
	Cancel
    }

    private variable rowcount
    private variable filters
    private variable hidden

} ; ## ::itcl::class DIODisplay

catch { ::itcl::delete class ::DIODisplayField }

#
# DIODisplayField object -- defined for each field we're displaying
#
::itcl::class ::DIODisplayField {

    constructor {args} {
	## We want to simulate Itcl's configure command, but we want to
	## check for arguments that are not variables of our object.  If
	## they're not, we save them as arguments to the form when this
	## field is displayed.
	import_keyvalue_pairs data $args
	foreach var [array names data] {
	    if {![info exists $var]} {
		lappend formargs -$var $data($var)
	    } else {
		set $var $data($var)
	    }
	}

	# if text (field description) isn't set, prettify the actual
	# field name and use that
	if {[lempty $text]} { set text [pretty [split $name _]] }
    }

    destructor {

    }

    method destroy {} {
	::itcl::delete object $this
    }

    #
    # get_css_class - ask the parent DIODIsplay object to look up
    # a CSS class entry
    #
    method get_css_class {tag default class} {
	return [$display get_css_class $tag $default $class]
    }

    #
    # get_css_tag -- set tag to select or textarea if type is select
    # or textarea, else to input
    #
    method get_css_tag {} {
	switch -- $type {
	    "select" {
		set tag select
	    }
	    "textarea" {
		set tag textarea
	    }
	    default {
		set tag input
	    }
	}
    }

    #
    # pretty -- prettify a list of words by uppercasing the first letter
    #  of each word
    #
    method pretty {string} {
	set words ""
	foreach w $string {
	    lappend words \
		[string toupper [string index $w 0]][string range $w 1 end]
	}
	return [join $words " "]
    }

    method configvar {varName string} {
	if {[lempty $string]} { return [set $varName] }
	configure -$varName $string
    }

    #
    # showview - emit a table row of either DIOViewRow, DIOViewRowAlt,
    # DIOViewRow-fieldname (this object's field name), or 
    # DIOViewRowAlt-fieldname, a table data field of either
    # DIOViewHeader or DIOViewHeader-fieldname, and then a
    # value of class DIOViewField or DIOViewField-fieldname
    #
    method showview {{alt ""}} {
	set class [get_css_class TR DIOViewRow$alt DIOViewViewRow$alt-$name]
	puts "<TR CLASS=\"$class\">"

	set class [get_css_class TD DIOViewHeader DIOViewHeader-$name]
	puts "<TD CLASS=\"$class\">$text:</TD>"

	set class [get_css_class TD DIOViewField DIOViewField-$name]
	puts "<TD CLASS=\"$class\">$value</TD>"

	puts "</TR>"
    }

    #
    # showform -- like showview, creates a table row and table data, but
    # if readonly isn't set, emits a form field corresponding to the type
    # of this field
    #
    method showform {} {
	puts "<TR>"

	set class [get_css_class TD DIOFormHeader DIOFormHeader-$name]
	puts "<TD CLASS=\"$class\">$text:</TD>"

	set class [get_css_class TD DIOFormField DIOFormField-$name]
	puts "<TD CLASS=\"$class\">"
	if {$readonly} {
	    puts "$value"
	} else {
	    set tag [get_css_tag]
	    set class [get_css_class $tag DIOFormField DIOFormField-$name]

	    if {$type == "select"} {
		$form select $name -values $values -class $class
	    } else {
		eval $form $type $name -value [list $value] $formargs -class $class
	    }
	}
	puts "</TD>"
	puts "</TR>"
    }

    # methods that give us method-based access to get and set the
    # object's variables...
    method display  {{string ""}} { configvar display $string }
    method form  {{string ""}} { configvar form $string }
    method formargs  {{string ""}} { configvar formargs $string }
    method name  {{string ""}} { configvar name $string }
    method text  {{string ""}} { configvar text $string }
    method type  {{string ""}} { configvar type $string }
    method value {{string ""}} { configvar value $string }
    method readonly {{string ""}} { configvar readonly $string }

    public variable display		""
    public variable form		""
    public variable formargs		""

    # values - for fields of type "select" only, the values that go in
    # the popdown (or whatever) selector
    public variable values              ""

    # name - the field name
    public variable name		""

    # text - the description text for the field. if not specified,
    #  it's constructed from a prettified version of the field name
    public variable text		""

    # value - the default value of the field
    public variable value		""

    # type - the data type of the field
    public variable type		"text"

    # readonly - if 1, we don't allow the value to be changed
    public variable readonly		0

} ; ## ::itcl::class DIODisplayField

catch { ::itcl::delete class ::DIODisplayField_boolean }

#
# DIODisplayField_boolen -- superclass of DIODisplayField that overrides
# a few methods to specially handle booleans
#
::itcl::class ::DIODisplayField_boolean {
    inherit ::DIODisplayField

    constructor {args} {eval configure $args} {
	eval configure $args
    }

    method add_true_value {string} {
	lappend trueValues $string
    }

    #
    # showform -- emit a form field for a boolean
    #
    method showform {} {
	puts "<TR>"

	set class [get_css_class TD DIOFormHeader DIOFormHeader-$name]
	puts "<TD CLASS=\"$class\">$text:</TD>"

	set class [get_css_class TD DIOFormField DIOFormField-$name]
	puts "<TD CLASS=\"$class\">"
	if {$readonly} {
	    if {[boolean_value]} {
		puts $true
	    } else {
		puts $false
	    }
	} else {
	    if {[boolean_value]} {
		$form default_value $name $true
	    } else {
		$form default_value $name $false
	    }
	    eval $form radiobuttons $name \
		-values [list "$true $false"] $formargs
	}
	puts "</TD>"
	puts "</TR>"
    }

    #
    # showview -- emit a view for a boolean
    #
    method showview {{alt ""}} {
	set class [get_css_class TR DIOViewRow$alt DIOViewRow$alt-$name]
	puts "<TR CLASS=\"$class\">"

	set class [get_css_class TD DIOViewHeader DIOViewHeader-$name]
	puts "<TD CLASS=\"$class\">$text:</TD>"

	set class [get_css_class TD DIOViewField DIOViewField-$name]
	puts "<TD CLASS=\"$class\">"
	if {[boolean_value]} {
	    puts $true
	} else {
	    puts $false
	}
	puts "</TD>"

	puts "</TR>"
    }

    #
    # boolean_value -- return 1 if value is found in the values list, else 0
    #
    method boolean_value {} {
	set val [string tolower $value]
	if {[lsearch -exact $values $val] > -1} { return 1 }
	return 0
    }

    method value {{string ""}} { configvar value $string }

    public variable true	"Yes"
    public variable false	"No"
    public variable values	"1 y yes t true on"

    public variable value "" {
	if {[boolean_value]} {
	    set value $true
	} else {
	    set value $false
	}
    }

} ; ## ::itcl::class ::DIODisplayField_boolean


