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
#parray response

	if {[lempty $DIO]} {
	    return -code error "You must specify a DIO object"
	}

	if {[lempty $form]} {
	    set form [namespace which [::form #auto -defaults response]]
	}

	set document [env DOCUMENT_NAME]

	if {[info exists response(numResults)] \
	    && ![lempty $response(numResults)]} {
	    set pagesize $response(numResults)
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

    method is_function {name} {
	if {[lsearch $functions $name] > -1} { return 1 }
	if {[lsearch $allfunctions $name] > -1} { return 1 }
	return 0
    }

    method do_cleanup {} {
	## Destroy all the fields created.
	foreach field $allfields { catch { $field destroy } }

	## Destroy the DIO object.
	catch { $DIO destroy }

	## Destroy the form object.
	catch { $form destroy }
    }

    method handle_error {error} {
	puts "<B>An error has occurred processing your request</B>"
	puts "<PRE>"
	puts "$error"
	puts "</PRE>"
    }

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

    ## Figure out which CSS class we want to use.  If class exists, we use
    ## that.  If not, we use default.
    method get_css_class {tag default class} {
	if {[info exists cssArray([string toupper $tag.$class])]} {
	    return $class
	}
	if {[info exists cssArray([string toupper .$class])]} { return $class }
	return $default
    }

    method parse_css_class {class arrayName} {
	if {![info exists cssArray($class)]} { return }
	upvar 1 $arrayName array
	foreach line [split $cssArray($class) \;] {
	    set line [string trim $line]
	    lassign [split $line :] property value
	    set property [string trim [string tolower $property]]
	    set value [string trim $value]
	    set array($property) $value
	}
    }

    method button_image_src {class} {
	set class [string toupper input.$class]
	parse_css_class $class array
	if {![info exists array(image-src)]} { return }
	return $array(image-src)
    }

    method show {} {
	set mode Main
	if {[info exists response(mode)]} { set mode $response(mode) }

	if {![lempty $css]} {
	    puts "<LINK REL=\"stylesheet\" TYPE=\"text/css\" HREF=\"$css\">"
	}

	puts {<TABLE WIDTH="100%" CLASS="DIO">}
	puts "<TR>"
	puts {<TD VALIGN="center" CLASS="DIO">}

	if {$mode != "Main" && $persistentmain} { Main }

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
	foreach field $fields {
	    $field showview
	}
	puts "</TABLE>"
    }

    #
    # showform - emit a form for inserting a new record
    #
    # response(searchBy) will contain whatever was in the "where" field
    # response(query) will contain whatever was in the "is" field
    #
    method showform {} {
	get_field_values array

	set save [button_image_src DIOFormSaveButton]
	set cancel [button_image_src DIOFormCancelButton]

	$form start
	foreach fld [array names hidden] {
	    $form hidden $fld -value $hidden($fld)
        }
	$form hidden mode -value Save
	$form hidden DIODfromMode -value $response(mode)
	$form hidden DIODkey -value [$DIO makekey array]
	puts {<TABLE CLASS="DIOForm">}

	# emit the fields for each field using the showform method
	# of the field.  if they've typed something into the
	# search field and it matches one of the fields in the
	# record (and it should), put that in as the default
	foreach field $fields {
	    if {[info exists response(searchBy)] && $response(searchBy) == [$field text]} {
		if {![$field readonly] && $response(query) != ""} {
		    $field value $response(query)
		}
	    }
	    $field showform
	}
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

    method page_buttons {side} {
	if {$pagesize <= 0} { return }

	if {![info exists response(page)]} { set response(page) 1 }

	set pref DIO$side
	set count [$DIOResult numrows]

	set class [get_css_class TABLE DIONavButtons ${pref}NavButtons]
	puts "<TABLE WIDTH=\"100%\" CLASS=\"$class\">"
	puts "<TR>"
	set class [get_css_class TD DIOBackButton ${pref}BackButton]
	if {$response(page) != 1} {
	    set back [button_image_src $class]

	    puts "<TD ALIGN=LEFT CLASS=\"$class\">"
	    set f [::form #auto -defaults response]
	    $f start
	    foreach fld [array names hidden] {
	        $f hidden $fld -value $hidden($fld)
            }
	    $f hidden mode
	    $f hidden query
	    $f hidden searchBy
	    $f hidden sortBy
	    $f hidden numResults
	    $f hidden page -value [expr $response(page) - 1]
	    set class [get_css_class INPUT DIOBackButton ${pref}BackButton]
	    if {![lempty $back]} {
		$f image back -src $back -class $class
	    } else {
		$f submit back -value "Back" -class $class
	    }
	    $f end
	    $f destroy
	    puts "</TD>"
	} else {
	    puts "<TD CLASS=\"$class\">&nbsp;</TD>"
	}

	## As long as count == pagesize, we fetched exactly the limit of
	## records.  This may sometimes lead to a next button when there
	## are actually no more records if the number of records is a clean
	## divisible of pagesize.

	set class [get_css_class TD DIONextButton ${pref}NextButton]
	if {$count == $pagesize} {
	    set next [button_image_src $class]

	    puts "<TD ALIGN=RIGHT CLASS=\"$class\">"
	    set f [::form #auto -defaults response]
	    $f start
	    foreach fld [array names hidden] {
	        $f hidden $fld -value $hidden($fld)
            }
	    $f hidden mode
	    $f hidden query
	    $f hidden searchBy
	    $f hidden sortBy
	    $f hidden numResults
	    $f hidden page -value [expr $response(page) + 1]
	    set class [get_css_class INPUT DIONextButton ${pref}NextButton]
	    if {![lempty $next]} {
		$f image next -src $next -class $class
	    } else {
		$f submit next -value "Next" -class $class
	    }
	    $f end
	    $f destroy
	    puts "</TD>"
	} else {
	    puts "<TD CLASS=\"$class\">&nbsp;</TD>"
	}
	puts "</TR>"
	puts "</TABLE>"
    }


    method rowheader {} {
	set fieldList $fields
	if {![lempty $rowfields]} { set fieldList $rowfields }

	set rowcount 0

	puts <P>

	if {$topnav} { page_buttons Top }

	puts {<TABLE BORDER WIDTH="100%" CLASS="DIORowHeader">}
	puts "<TR CLASS=DIORowHeader>"
	foreach field $fieldList {
	    set text [$field text]
	    ## If sorting is turned off, or this field is not in the
	    ## sortfields, we don't display the sort option.
	    if {!$allowsort || \
		(![lempty $sortfields] && [lsearch $sortfields $field] < 0)} {
		set html $text
	    } else {
		set html {<A HREF="}
		append html "$document?mode=$response(mode)"
		foreach var {query searchBy numResults} {
		    if {[info exists response($var)]} {
			append html "&$var=$response($var)"
		    }
		}
	        foreach fld [array names hidden] {
	            append html "&$fld=$hidden($fld)"
                }
		append html "&sortBy=$field\">$text</A>"
	    }
	    set class [get_css_class TH DIORowHeader DIORowHeader-$field]
	    puts "<TH CLASS=\"$class\">$html</TH>"
	}

	puts {<TH CLASS="DIORowHeaderFunctions">Functions</TH>}
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

	if {![lempty $rowfunctions]} {
	    puts "<TD NOWRAP CLASS=\"DIORowFunctions$alt\">"
	    set f [::form #auto]
	    $f start
	    foreach fld [array names hidden] {
	        $f hidden $fld -value $hidden($fld)
            }
	    $f hidden query -value [$DIO makekey a]
	    $f select mode -values $rowfunctions -class DIORowFunctionSelect$alt
	    $f submit submit -value "Go" -class DIORowFunctionButton$alt
	    $f end
	    puts "</TD>"
	}

	puts "</TR>"
    }

    method rowfooter {} {
	puts "</TABLE>"

	if {$bottomnav} { page_buttons Bottom }
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
#puts "storing"
#parray array
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

	foreach var [array names array] {
#puts "set_field_values: $var value $array($var)<br>"
	    if {[catch { $var value $array($var) } result] == 1} {
#puts "error: $result<br>"
	    }
	}
    }

    method get_field_values {arrayName} {
	upvar 1 $arrayName array

	foreach field $allfields {
#puts "set array($field) [$field value]<br>"
	    set array($field) [$field value]
	}
    }

    method DisplayRequest {query} {
	set DIOResult [eval $DIO search $query]

	if {[$DIOResult numrows] <= 0} {
	    puts "Could not find any matching records."
	    return
	}

	rowheader

	$DIOResult forall -array a {
	    showrow a
	}

	rowfooter

	$DIOResult destroy
	set DIOResult ""
    }

    method Main {} {
	puts "<TABLE BORDER=0 WIDTH=100% CLASS=DIOForm><TR>"

	puts "<TD CLASS=DIOForm ALIGN=CENTER VALIGN=MIDDLE>"
	puts "<BR/>"
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
	    	$f submit submit -value "List All" -class DIORowFunctionButton
	    	$f end
	    }
	}
	puts "</TD>"

	puts "<TD CLASS=DIOForm VALIGN=MIDDLE>"
	$form start

	foreach fld [array names hidden] {
	    $form hidden $fld -value $hidden($fld)
        }

	$form select mode -values $selfunctions -class DIOMainFunctionsSelect

	set useFields $fields
	if {![lempty $searchfields]} { set useFields $searchfields }

        puts "where"
	$form select searchBy -values [pretty_fields $useFields] \
	    -class DIOMainSearchBy
        puts "is"
	$form text query -value "" -class DIOMainQuery
	$form submit submit -value "GO" -class DIOMainSubmitButton
	puts "</TD></TR>"

	if {![lempty $numresults]} {
	    puts "<TR><TD CLASS=DIOForm>Results per page: "
	    $form select numResults -values $numresults -class DIOMainNumResults
	    puts "</TD></TR>"
	}

	$form end
	puts "</TABLE>"
    }

    method sql_order_by_syntax {} {
	if {[info exists response(sortBy)] && ![lempty $response(sortBy)]} {
	    return " ORDER BY $response(sortBy)"
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
	set searchField $FieldTextMap($response(searchBy))	

	set query "-$searchField $response(query)"

	append query [sql_order_by_syntax]
	append query [sql_limit_syntax]

	DisplayRequest $query
    }

    method List {} {
	set query [sql_order_by_syntax]
	append query [sql_limit_syntax]

	DisplayRequest $query
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

	if {[lempty $text]} { set text [pretty [split $name _]] }
    }

    destructor {

    }

    method destroy {} {
	::itcl::delete object $this
    }

    method get_css_class {tag default class} {
	return [$display get_css_class $tag $default $class]
    }

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

    method showview {} {
	puts "<TR>"

	set class [get_css_class TD DIOViewHeader DIOViewHeader-$name]
	puts "<TD CLASS=\"$class\">$text:</TD>"

	set class [get_css_class TD DIOViewField DIOViewField-$name]
	puts "<TD CLASS=\"$class\">$value</TD>"

	puts "</TR>"
    }

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

::itcl::class ::DIODisplayField_boolean {
    inherit ::DIODisplayField

    constructor {args} {eval configure $args} {
	eval configure $args
    }

    method add_true_value {string} {
	lappend trueValues $string
    }

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

    method showview {} {
	puts "<TR>"

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

    method boolean_value {} {
	set val [string tolower $value]
	if {[lsearch -exact $values $val] > -1} { return 1 }
	return 0
    }
    
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


