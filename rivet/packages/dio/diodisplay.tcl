package require Itcl
package require DIO
catch { package require form }

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
    }

    destructor {
	if {$cleanup} { do_cleanup }
    }

    method destroy {} {
	::itcl::delete object $this
    }

    method configure_variable {varName string} {
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
	$DIO destroy

	## Destroy the form object.
	$form destroy
    }

    method handle_error {error} {
	global errorCode
	global errorInfo

	puts "<PRE>"
	puts "ERROR: $errorInfo"
	puts "</PRE>"
    }

    method show {} {
	set mode Main
	if {[info exists response(mode)]} { set mode $response(mode) }
	if {![is_function $mode]} {
	    puts "In-valid function"
	    abort_page
	    return
	}
	catch $mode error

	if {$cleanup} { destroy }

	if {![lempty $error]} { $errorhandler $error }
    }

    method showview {} {
	puts "<TABLE>"
	foreach field $fields {
	    $field showview
	}
	puts "</TABLE>"
    }

    method showform {} {
	get_field_values array

	$form start
	$form hidden DIODfromMode -value $response(mode)
	$form hidden DIODkey -value [$DIO makekey array]
	puts "<TABLE>"
	foreach field $fields {
	    $field showform
	}
	puts "</TABLE>"

	puts "<TABLE>"
	puts "<TR>"
	puts "<TD>"
	$form submit mode -value "Save"
	puts "</TD>"
	puts "<TD>"
	$form submit mode -value "Cancel"
	puts "</TD>"
	puts "</TR>"
	puts "</TABLE>"
    }

    method showrow {arrayName} {
	upvar 1 $arrayName a

	set fieldList $fields
	if {![lempty $rowfields]} { set fieldList $rowfields }

	puts "<TR>"
	foreach field $fieldList {
	    if {![info exists a($field)]} {
		puts "<TD></TD>"
	    } else {
		puts "<TD>$a($field)</TD>"
	    }
	}

	if {![lempty $rowfunctions]} {
	    puts "<TD NOWRAP>"
	    set f [::form #auto]
	    $f start
	    $f hidden query -value [$DIO makekey a]
	    $f select mode -values $rowfunctions
	    $f submit submit -value "Go"
	    $f end
	    puts "</TD>"
	}

	puts "</TR>"
    }

    method page_buttons {} {
	if {$pagesize <= 0} { return }

	if {![info exists response(page)]} { set response(page) 0 }

	puts "<TABLE WIDTH=\"$rowwidth\">"
	puts "<TR>"
	if {$response(page) != 0} {
	    puts "<TD ALIGN=LEFT>"
	    set f [::form #auto -defaults response]
	    $f start
	    $f hidden mode
	    $f hidden query
	    $f hidden sortBy
	    $f hidden page -value [expr $response(page) - 1]
	    $f submit submit -value "Back"
	    $f end
	    puts "</TD>"
	} else {
	    puts "<TD></TD>"
	}
	if {[$DIOResult numrows] >= $pagesize} {
	    puts "<TD ALIGN=RIGHT>"
	    set f [::form #auto -defaults response]
	    $f start
	    $f hidden mode
	    $f hidden query
	    $f hidden sortBy
	    $f hidden page -value [expr $response(page) + 1]
	    $f submit submit -value "Next"
	    $f end
	    puts "</TD>"
	} else {
	    puts "<TD></TD>"
	}
	puts "</TR>"
	puts "</TABLE>"
    }


    method rowheader {} {
	set fieldList $fields
	if {![lempty $rowfields]} { set fieldList $rowfields }

	puts <P>

	if {$topnav} { page_buttons }

	puts "<TABLE BORDER WIDTH=\"$rowwidth\">"
	puts "<TR>"
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
		append html "&query=$response(query)"
		append html "&sortBy=$field\">$text</A>"
	    }
	    puts "<TD><B>$html</TD>"
	}

	puts "<TD><CENTER><B>Functions</TD>"
	puts "</TR>"
    }

    method rowfooter {} {
	puts "</TABLE>"

	if {$bottomnav} { page_buttons }
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

	eval $class $name -name $name -form $form $args
	set FieldTextMap([$name text]) $name
    }

    method fetch {key arrayName} {
	upvar 1 $arrayName $arrayName
	return [$DIO fetch $key $arrayName]
    }

    method store {arrayName} {
	upvar 1 $arrayName $arrayName
	return [$DIO store $arrayName]
    }

    method delete {key} {
	return [$DIO delete $key]
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
	    catch { $var value $array($var) }
	}
    }

    method get_field_values {arrayName} {
	upvar 1 $arrayName array

	foreach field $allfields {
	    set array($field) [$field value]
	}
    }

    method DisplayRequest {req} {
	set DIOResult [$DIO exec $req]

	if {[$DIOResult numrows] <= 0} {
	    puts "Could not find any matching records."
	    abort_page
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
	$form start

	puts "<TABLE>"

	puts "<TR>"
	puts "<TD>Functions:</TD>"
	puts "<TD>"
	$form select mode -values $functions
	puts "</TD>"
	puts "</TR>"

	set useFields $fields
	if {![lempty $searchfields]} { set useFields $searchfields }

	puts "<TR>"
	puts "<TD>Search By:</TD>"
	puts "<TD>"
	$form select searchBy -values [pretty_fields $useFields]
	puts "</TD>"
	puts "</TR>"

	puts "<TR>"
	puts "<TD>Query:</TD>"
	puts "<TD>"
	$form text query
	puts "</TD>"
	puts "</TR>"

	puts "<TR>"
	puts "<TD COLSPAN=2>"
	$form submit submit
	puts "</TD>"
	puts "</TR>"

	puts "</TABLE>"
    }

    method sql_order_by_syntax {} {
	if {[info exists response(sortBy)] && ![lempty $response(sortBy)]} {
	    return " ORDER BY $response(sortBy)"
	}
    }

    method sql_limit_syntax {} {
	if {$pagesize <= 0} { return }

	set offset ""
	if {[info exists response(page)]} {
	    set offset [expr $response(page) * $pagesize]
	}
	return [$DIO sql_limit_syntax $pagesize $offset]
    }
	

    method Search {} {
	set searchField $FieldTextMap($response(searchBy))	
	set table [$DIO table]

	set req "SELECT * FROM $table
		WHERE $searchField LIKE '[$DIO quote $response(query)]'"

	append req [sql_order_by_syntax]

	append req [sql_limit_syntax]

	DisplayRequest $req
    }

    method List {} {
	set table [$DIO table]

	set req "SELECT * FROM $table"

	append req [sql_order_by_syntax]

	append req [sql_limit_syntax]

	DisplayRequest $req
    }

    method Add {} {
	showform
    }

    method Edit {} {
	if {![fetch $response(query) array]} {
	    puts "That record does not exist in the database."
	    abort_page
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
	    abort_page
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
	    abort_page
	    return
	}

	if {!$confirmdelete} {
	    DoDelete
	    return
	}

	puts "<CENTER>"
	puts "<TABLE>"
	puts "<TR>"
	puts "<TD COLSPAN=2>"
	puts "Are you sure you want to delete this record from the database?"
	puts "</TD>"
	puts "</TR>"
	puts "<TR>"
	puts "<TD>"
	puts "<CENTER>"
	set f [::form #auto]
	$f start
	$f hidden mode -value DoDelete
	$f hidden query -value $response(query)
	$f submit submit -value Yes
	$f end
	puts "</TD>"
	puts "<TD>"
	puts "<CENTER>"
	set f [::form #auto]
	$f start
	$f submit submit -value No
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
	    abort_page
	    return
	}

	set_field_values array

	showview
    }

    proc Cancel {} {
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

    method DIO {{string ""}} { configure_variable DIO $string }
    method DIOResult {{string ""}} { configure_variable DIOResult $string }
    method errorhandler {{string ""}} {configure_variable errorhandler $string }

    method title {{string ""}} { configure_variable title $string }
    method functions {{string ""}} { configure_variable functions $string }
    method pagesize {{string ""}} { configure_variable pagesize $string }
    method form {{string ""}} { configure_variable form $string }
    method cleanup {{string ""}} { configure_variable cleanup $string }
    method confirmdelete {{string ""}} {
	configure_variable confirmdelete $string
    }

    method allowsort {{string ""}} { configure_variable allowsort $string }
    method sortfields {{string ""}} { configure_variable sortfields $string }
    method background {{string ""}} { configure_variable background $string }
    method border {{string ""}} { configure_variable border $string }
    method bordercolor {{string ""}} { configure_variable bordercolor $string }
    method topnav {{string ""}} { configure_variable topnav $string }
    method bottomnav {{string ""}} { configure_variable bottomnav $string }

    method rowfunctions {{string ""}} {configure_variable rowfunctions $string}
    method rowwidth {{string ""}} {configure_variable rowwidth $string}

    public variable DIO		 ""
    public variable DIOResult	 ""
    public variable errorhandler "handle_error"

    public variable title	 ""
    public variable fields	 ""
    public variable searchfields ""
    public variable functions	 "Search List Add Edit Delete Details"
    public variable pagesize	 25
    public variable form	 ""
    public variable cleanup	 1
    public variable confirmdelete 1

    public variable allowsort	 1
    public variable sortfields	 ""
    public variable background	 "white"
    public variable border	 0
    public variable bordercolor	 "black"
    public variable topnav	 1
    public variable bottomnav	 1

    public variable rowfields	 ""
    public variable rowfunctions "Details Edit Delete"
    public variable rowwidth	 "100%"

    private variable response
    private variable document	 ""
    private variable allfields    ""
    private variable FieldTextMap
    private variable allfunctions {
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

    method pretty {string} {
	set words ""
	foreach w $string {
	    lappend words \
		[string toupper [string index $w 0]][string range $w 1 end]
	}
	return [join $words " "]
    }

    method configure_variable {varName string} {
	if {[lempty $string]} { return [set $varName] }
	configure -$varName $string
    }

    method showview {} {
	puts "<TR>"
	puts "<TD><B>$text</B>:</TD>"
	puts "<TD>$value</TD>"
	puts "</TR>"
    }

    method showform {} {
	puts "<TR>"
	puts "<TD ALIGN=RIGHT><B>$text</B>:</TD>"
	puts "<TD>"
	if {$readonly} {
	    puts "$value"
	} else {
	    eval $form $type $name -value [list $value] $formargs
	}
	puts "</TD>"
	puts "</TR>"
    }

    method form  {{string ""}} { configure_variable form $string }
    method formargs  {{string ""}} { configure_variable formargs $string }
    method name  {{string ""}} { configure_variable name $string }
    method text  {{string ""}} { configure_variable text $string }
    method type  {{string ""}} { configure_variable type $string }
    method value {{string ""}} { configure_variable value $string }
    method readonly {{string ""}} { configure_variable readonly $string }

    public variable form		""
    public variable formargs		""
    public variable name		""
    public variable text		""
    public variable value		""
    public variable type		"text"
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
	puts "<TD ALIGN=RIGHT><B>$text</B>:</TD>"
	puts "<TD>"
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
	puts "<TD><B>$text</B>:</TD>"
	puts "<TD>"
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
