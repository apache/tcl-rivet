# $Id$

# Tcl-based Apache test suite, by David N. Welton <davidw@dedasys.com>

# This test suite provides a means to create configuration files, and
# start apache with user-specified options.  All it needs to run is
# the name of the Apache executable, which must, however, be compiled
# with the right options.

package require Tclx
package provide apachetest 0.1

namespace eval apachetest {

    set debug 0

    # name of the apache binary, such as /usr/sbin/httpd
    variable binname ""
    # this file should be in the same directory this script is.
    variable templatefile [file join [file dirname [info script]] \
			       template.conf.tcl]
}

# apachetest::need_modules --
#
#	Tell the test suite which modules we *need* to have.  The test
#	suite will then check to see if these are either 1) compiled
#	into the server or 2) loaded in its configuration file.
#
# Arguments:
#	args
#
# Side Effects:
#	Sets up the rest of the apachetest tools to find the needed
#	modules.
#
# Results:
#	None.

proc apachetest::need_modules { modlist } {
    variable module_assoc
    foreach module_pair $modlist {
	set module_assoc([lindex $module_pair 0]) [lindex $module_pair 1]
    }
}

# apachetest::connect --
#
#	Attempt to open a socket to the web server we are using in our
#	tests.  Try for 10 seconds before giving up.
#
# Arguments:
#	None.

proc apachetest::connect { } {
    set starttime [clock seconds]
    set diff 0
    # We try for 10 seconds.
    while { $diff < 10 } {
	if { ! [catch {
	    set sk [socket localhost 8081]
	} err]} {
	    close $sk
	    return
	}
	set diff [expr {[clock seconds] - $starttime}]
    }
}

# apachetest::start --
#
#	Start the web server in the background.  After running the
#	script specified, stop the server.
#
# Arguments:
#	options - command line options to pass to the web server.
#	code - code to run.
#
# Side Effects:
#	Runs 'code' in the global namespace.
#
# Results:
#	None.

proc apachetest::start { options code } {
    variable serverpid 0
    variable binname
    variable debug

    # There has got to be a better way to do this, aside from waiting.
    set serverpid [eval exec  $binname -X -f \
		       [file join [pwd] server.conf] $options >& apachelog.txt &]

    apachetest::connect
    if { $debug > 0 } {
	puts "Apache started as PID $serverpid"
    }
    if { [catch {
	uplevel $code
    } err] } {
	puts $err
    }
    # Kill and wait are the only reasons we need TclX.
    kill $serverpid
    catch {
	set waitres [wait $serverpid]
	if { $debug > 0 } {
	    puts $waitres
	}
    }
}

# startserver - start the server with 'options'.

proc apachetest::startserver { args } {
    variable binname
    variable debug
    if { [catch {
	if { $debug } {
	    puts "$binname -X -f [file join [pwd] server.conf] [concat $args]"
	}
	set serverpid [eval exec $binname -X -f \
			   "[file join [pwd] server.conf]" [concat $args]]
    } err] } {
	puts "$err"
    }
}

# getbinname - get the name of the apache binary, and check to make
# sure it's ok.  The user should supply this parameter.

proc apachetest::getbinname { argv } {
    variable binname
    set binname [lindex $argv 0]
    if { $binname == "" || ! [file exists $binname] } {
	error "Please supply the full name and path of the Apache executable on the command line."
    }
    return $binname
}

# get the modules that are compiled into Apache directly, and return
# the XXX_module name.  Check also for the existence of mod_so, which
# we need to load the shared object in the directory above...

proc apachetest::getcompiledin { binname } {
    variable module_assoc
    set bin [open [list | "$binname" -l] r]
    set compiledin [read $bin]
    close $bin
    set modlist [split $compiledin]
    set compiledin [list]
    set mod_so_present 0
    foreach entry $modlist {
	if { [regexp {(.*)\.c$} $entry match modname] } {
	    if { $modname == "mod_so" } {
		set mod_so_present 1
	    }
	    if { [info exists module_assoc($modname)] } {
		lappend compiledin $module_assoc($modname)
	    }
	}
    }
    if { $mod_so_present == 0 } {
	error "We need mod_so in Apache to run these tests"
    }
    return $compiledin
}

# find the httpd.conf file

proc apachetest::gethttpdconf { binname } {
    set bin [open [list | "$binname" -V] r]
    set options [read $bin]
    close $bin
    regexp {SERVER_CONFIG_FILE="(.*?)"} "$options" match filename
    if { ! [file exists $filename] } {
	# see if we can find something by combining HTTP_ROOT + SERVER_CONFIG_FILE
	regexp {HTTPD_ROOT="(.*?)"} "$options" match httpdroot
	set completename [file join $httpdroot $filename]
	if { ! [file exists $completename] } {
	    error "neither '$filename' or '$completename' exists"
	}
	return $completename
    }
    return $filename
}

# if we need to load some modules, find out how to do it from the
# 'real' (the one installed on the system) conf file, with this proc

proc apachetest::getloadmodules { conffile needtoget } {
    set fl [open $conffile r]
    set confdata [read $fl]
    close $fl
    set loadline [list]
    foreach mod $needtoget {
	if { ! [regexp -line "^.*?(LoadModule\\s+$mod\\s+.+)\$"\
		    $confdata match line] } {
	    error "No LoadModule line for $mod!"
	} else {
	    lappend loadline $line
	}
    }
    return [join $loadline "\n"]
}

# Compare what's compiled in with what we need.

proc apachetest::determinemodules { binname } {
    variable module_assoc
    set compiledin [lsort [getcompiledin $binname]]
    set conffile [gethttpdconf $binname]

    foreach {n k} [array get module_assoc] {
	lappend needed $k
    }
    set needed [lsort $needed]

    set needtoget [list]
    foreach mod $needed {
	if { [lsearch $compiledin $mod] == -1 } {
	    lappend needtoget $mod
	}
    }
    if { $needtoget == "" } {
	return ""
    } else {
	return [getloadmodules $conffile $needtoget]
    }
}

# apachetest::makeconf --
#
#	Creates a config file and writes it to disk.
#
# Arguments:
#	outfile - the file to create/write to.
#	extra - extra config options to add.
#
# Side Effects:
#	Creates a new config file.
#
# Results:
#	None.

proc apachetest::makeconf { outfile {extra ""} } {
    variable binname
    variable templatefile
    set CWD [pwd]

    # replace with determinemodules
    set LOADMODULES [determinemodules $binname]

    set fl [open [file join . $templatefile] r]
    set template [read $fl]
    append template $extra
    close $fl

    set out [subst $template]

    set of [open $outfile w]
    puts $of "$out"
    close $of
}
