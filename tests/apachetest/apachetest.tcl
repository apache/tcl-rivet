# $Id$

# Tcl based Apache test suite, by David N. Welton <davidw@dedasys.com>

# This test suite provides a means to create configuration files, and
# start apache with user-specified options.  All it needs to run is
# the name of the Apache executable, which must, however, be compiled
# with the right options.

package provide apachetest 0.1

namespace eval apachetest {

    # Associate module names with their internal names.
    array set module_assoc {
	mod_log_config	  config_log_module
	mod_mime		mime_module
	mod_negotiation	 negotiation_module
	mod_dir			 dir_module
	mod_access	      access_module
	mod_auth		auth_module
    }
    # name of the apache binary, such as /usr/sbin/httpd
    variable binname ""
    # this file should be in the same directory this script is.
    variable templatefile [file join [file dirname [info script]] \
			       template.conf.tcl]
}

# start - start the server in the background with 'options' and then
# run 'code'.

proc apachetest::start { options code } {
    variable serverpid 0
    variable binname

    # There has got to be a better way to do this, aside from waiting.
    after 200
    set serverpid [eval exec $binname -X -f \
		       "[file join [pwd] server.conf]" $options &]
    after 100
    puts "Apache started as PID $serverpid"
    if { ! [catch {
	uplevel $code
    } err] } {
	puts $err
    }
    after 100
    exec kill $serverpid
}

# startserver - start the server with 'options'.

proc apachetest::startserver { options } {
    variable binname
    eval exec $binname -X -f \
	"[file join [pwd] server.conf]" $options
}

# getbinname - get the name of the apache binary, and check to make
# sure it's ok.  The user should supply this parameter.

proc apachetest::getbinname { argv } {
    variable binname
    set binname [lindex $argv 0]
    if { $binname == "" || ! [file exists $binname] } {
	error "Please supply the full name and path of the Apache executable on the command line!"
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
    set bin [ open [list | "$binname" -V ] r ]
    set options [ read $bin ]
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

# compare what's compiled in with what we need

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

# dump out a config
# outfile is the file to write to.
# extra is for extra config things we want to tack on.

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
