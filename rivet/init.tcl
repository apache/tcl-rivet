namespace eval ::Rivet {

    ###
    ## This routine gets called each time a new request comes in.
    ## It sets up the request namespace and creates a global command
    ## to replace the default global.  This ensures that when a user
    ## uses global variables, they're actually contained within the
    ## namespace.  So, everything gets deleted when the request is finished.
    ###
    proc initialize_request {} {
	catch { namespace delete ::request }

	namespace eval ::request { }

	proc ::request::global {args} {
	    foreach arg $args {
		uplevel "::global ::request::$arg"
	    }
	}
    }

    ###
    ## The default error handler for Rivet.  Any time a page runs into an
    ## error, this routine will be called to handle the error information.
    ## If an ErrorScript has been specified, this routine will not be called.
    ###
    proc handle_error {} {
	global errorInfo
	global errorOutbuf

	puts <PRE>
	puts "<HR>$errorInfo<HR>"
	puts "<P><B>OUTPUT BUFFER:</B></P>"
	puts $errorOutbuf
	puts </PRE>
    }

    ###
    ## This routine gets called each time a request is finished.  Any kind
    ## of special cleanup can be placed here.
    ###
    proc cleanup_request {} {
    }

    ###
    ## The main initialization procedure for Rivet.
    ###
    proc init {} {
	global auto_path
	global server

	## Add the rivet-tcl directory to Tcl's auto search path.
	## We insert it at the head of the list because we want any of
	## our procs named the same as Tcl's procs to be overridden.
	## Example: parray
	set tclpath [file join $server(RIVET_DIR) rivet-tcl]
	set auto_path [linsert $auto_path 0 $tclpath]

	## Add the packages directory to the auto_path.
	## If we have a packages$tcl_version directory
	## (IE: packages8.3, packages8.4) append that as well.
	set pkgpath [file join $server(RIVET_DIR) packages]
	lappend auto_path $pkgpath
	lappend auto_path ${pkgpath}-local

	if {[file exists ${pkgpath}$::tcl_version]} {
	    lappend auto_path ${pkgpath}$::tcl_version
	}

	## This will allow users to create proc libraries and tclIndex files
	## in the local directory that can be autoloaded.
	lappend auto_path .
    }

} ;## namespace eval ::Rivet

## Initialize Rivet.
::Rivet::init
