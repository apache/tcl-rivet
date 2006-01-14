#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

# Tcl configure-like system.

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

# ./configure.tcl -help for options.

# $Id: configure.tcl,v 1.8 2004/09/16 01:08:52 karl Exp $

source [file join [file dirname [info script]] buildscripts buildscripts.tcl]

namespace eval configure {
    array set errors {}
    set useroptions {}
    array set optionvars {}
    set verbose 0
}

namespace eval configs {}

# configure::ProcessOptions --
#
#	This is called to process the command line options to configure.
#
# Arguments:
#	None.
#
# Side Effects:
#	Sets up the 'optionvars' array, which links command line
#	options with the variable names they set.  Exits on error.
#
# Results:
#	None.


proc configure::ProcessOptions {} {
    global argv argv0
    variable useroptions
    variable optionvars
    variable verbose
    set optionvars(prefix) PREFIX
    set optionvars(enable-symbols) DEBUGSYMBOLS
    set options {
	{prefix.arg ""		"prefix - where to install"}
	{enable-symbols		"enable debugging symbols"}
	{verbose		"verbose output"}
    }
    set options [concat $options $useroptions]
    set usage "options:"
    if { [ catch {
	array set params [::cmdline::getoptions argv $options $usage]
    } err] } {
	errorexit "$err"
    }
    # Process each option.
    foreach {key val} [array get params] {
	if { [info exists optionvars($key)] } {
	    set ::configs::[set optionvars($key)] $val
	}
    }
    if { $params(verbose) } {
	set verbose 1
    }
}


# configure::AddOption --
#
#	Add an option to the ./configure command line parser.
#
# Arguments:
#	-flag flagname
#	-var  variable name
#	-desc description of the option
#	-arg  Set this flag if the option takes an argument.
#	-default  Default value for the argument.
#
# Side Effects:
#	Set up 'useroptions' to be used in ProcessOptions.
#
# Results:
#	None.

proc configure::AddOption {args} {
    variable useroptions
    variable optionvars

    set options {
	{flag.arg ""   	"name of configure flag"}
	{var.arg ""	"name of variable name to store info in"}
	{desc.arg ""	"description of option"}
	{arg		"Use this if the flag takes an argument"}
	{default.arg ""	"If arg is 1, default argument"}
    }
    set usage "[info level 0]:"
    if { [ catch {
	array set params [::cmdline::getoptions args $options $usage]
    } err] } {
	puts "$err"
	exit 1
    }
    if { $params(flag) == "" } {
	puts "-flag option takes an argument"
	exit 1
    }
    if { $params(var) == "" } {
	errorexit "-var option takes an argument"
    }
    if { $params(desc) == "" } {
	errorexit "-desc option takes an argument"
    }
    if { $params(arg) } {
	lappend useroptions [list "$params(flag).arg" $params(default)\
				 $params(desc)]
    } else {
	if { $params(default) } {
	    errorexit "-default invalid without -arg option"
	}
	lappend useroptions [list "$params(flag)" $params(desc)]
    }

    set optionvars($params(flag)) $params(var)
}


# configure::test --
#
#	Test something in the environment, and set a variable
#	accordingly.  Note that the 'body' code is run
#
# Arguments:
#	varname - name of the variable to set.
#	body - code to execute.  The return value of the last command
#	is the value for the variable
#
# Side Effects:
#	Creates variables in the ::configs:: namespace.  Populates the
#	'errors' array with failed configuration tests.
#
# Results:
#	None.

proc configure::test {varname body} {
    global errorInfo

    variable errors
    variable verbose

    if { ! $verbose } {
	puts -nonewline "."
	flush stdout
    }
    if { [info exists ::configs::${varname}] && \
	     [set ::configs::[set varname]] != "" } {
	# It already exists - it was probably passed on the command
	# line.
	return
    }

    set val ""
    set oldvars [lsort [info vars ::configs::*]]
    if { [catch {set val [namespace eval ::configs $body]} err] } {
	# Exit on error.
	puts stderr ""
	puts stderr "Error in $varname test: $err"

	# comment this line in if you need help debugging the error that
	# happened:
	#puts stderr "$errorInfo"

	exit 1
    } else {
	set ::configs::${varname} $val
    }
    set newvars [lsort [info vars ::configs::*]]
    if { $verbose } {
	puts "$varname	=	$val"
    }
    # Clean up temporary variables.
    foreach var $newvars {
	if { [lsearch $oldvars $var] < 0 } {
	    if { $var != "::configs::$varname" } {
		unset $var
	    }
	}
    }
}


# configure::assert --
#
#	Exits if an assertion is false.  We use this to make sure the
#	environment meets specific criterion.
#
# Arguments:
#	name - useful for looking it up, doesn't really matter.
#	body - code to test.
#
# Side Effects:
#	Exits on failure.
#
# Results:
#	None.

proc configure::assert {name body} {
    set val 0
    set errmsg ""
    if { [catch {set val [namespace eval ::configs $body]} err] } {
	set errmsg "$name assertion error: $err"
    } elseif { $val == 0 } {
	set errmsg "$name assertion false: $body"
    } else {
	return
    }
    puts stderr ""
    puts stderr $errmsg
    exit 1
}

# Helper procedures for findtclconfig

proc configure::RelToExec {exe cf} {
    return [file join [file dirname [file dirname $exe]] lib $cf]
}

proc configure::RelToExec2 {cf} {
    return [file join [info library] $cf]
}


# configure::findtclconfig --
#
#	Finds the tclConfig.sh of the tclsh running.  This is included
#	here, because we can use that file to find out just how Tcl
#	itself was compiled that gives us most of the information we
#	need about compiling extensions.
#
# Arguments:
#	None.
#
# Side Effects:
#	None.
#
# Results:
#	Returns the location of the tclConfig.sh file.

proc configure::findtclconfig {} {
    set exec [file tail [info nameofexecutable]]
    # If we're running tclsh...
    if {[string match -nocase "*tclsh*" $exec]} {
	set cf [RelToExec [info nameofexecutable] tclConfig.sh]
	if {[file readable $cf]} {
	    return $cf
	} else {
	    set cf [RelToExec2 tclConfig.sh]
	    if {[file readable $cf]} {
		return $cf
	    }
	}
    }
    # If tcl_pkgPath is available, look there...
    global tcl_pkgPath
    if {[info exists tcl_pkgPath]} {
	foreach libdir $tcl_pkgPath {
	    if {[file readable [file join $libdir tclConfig.sh]]} {
		return [file join $libdir tclConfig.sh]
	    }
	}
    }
    # Not in usual places, go searching for tclsh...
    set candshells [list]
    if {[regsub -nocase wish $exec tclsh shell]} {
	lappend candshells $shell
    }
    lappend candshells tclsh[package provide Tcl]
    lappend candshells tclsh[join [split [package provide Tcl] .] ""] foreach shell $candshells {
	set shell [auto_execok $shell]
	if {[string length $shell]} {
	    set cf [RelToExec $shell tclConfig.sh]
	    if {[file readable $cf]} {
		return $cf
	    }
	}
    }
    error "tclConfig.sh not found"
}


# configure::parsetclconfig --
#
#	Parses the tclConfig.sh to extract the values contained
#	therein.
#
# Arguments:
#	config - tclConfig.sh file.
#
# Side Effects:
#	Uses the names of the variables in tclConfig.sh to create
#	variables with the same names in the configs namespace.
#
# Results:
#	None.

proc configure::parsetclconfig { config } {
    set fl [open $config r]
    while { ! [eof $fl] } {
	gets $fl line
	if { [string index $line 0] != "#" } {
	    set firstEquals [string first "=" $line]
	    if {$firstEquals > 0} {
	        set var [string range $line 0 [expr $firstEquals - 1]]
		set rawval [string range $line [expr $firstEquals + 1] end]
		set val ""
		if {[catch {
		    set val [namespace eval ::configs \
		        [list subst [string trim $rawval ']]]
		} err] == 1} {
		    puts "error: $err"
		    puts "  line was: $line"
		    puts "  emitting empty variable for: $var"
		    puts "  please make it right in configs.tcl or figure out the bug"
		    puts ""
		}
# uncomment to see all the config variables with their inferred values
#puts "set ::configs::[set var] $val"
		set ::configs::[set var] $val
	    }
	}
    }
}


# configure::lib_has_function --
#
#	Attempts to determine if a C library provides a function. Idea
#	copied from autoconf.  Maybe there is a better way to do
#	it?
#
# Arguments:
#	library - library name (without leading -l).
#	function - C function name.
#
# Side Effects:
#	None.
#
# Results:
#	Returns -l$library on success, empty string on failure.

proc configure::lib_has_function { library function } {
    set retval ""
    set program {
	int main() {
	    %s();
	    return 0;
	}
    }
    set program [format $program $function]
    set fl [open testconfig.c w]
    puts $fl $program
    close $fl
    if { ! [catch {
	exec cc -o testconfig testconfig.c -l$library
    } err] } {
	set retval "-l$library"
    }
    eval file delete [glob testconfig*]
    return $retval
}


# configure::writeconfigs --
#
#	Write variables from ::configs:: namespace to a file,
#	defaulting to configs.tcl.
#
# Arguments:
#	filename - where to write the variables.
#
# Side Effects:
#	None.
#
# Results:
#	None.

proc configure::writeconfigs { {filename configs.tcl} } {
    set fl [open $filename w]
    puts $fl "namespace eval ::configs {}"
    foreach var [lsort [info vars ::configs::*]] {
	puts $fl "set $var {[set $var]}"
    }
    close $fl
}

# Prints an error messages and exits.

proc configure::errorexit { msg } {
    puts stderr $msg
    exit 1
}

puts "Configuring"

# Here is where we actually read in the user's tests.
source [file join [file dirname [info script]] configure.in.tcl]

puts "done."

# If there were errors, report them.
set errlist [array get ::configure::errors]
if { [llength $errlist] > 0 } {
    puts "Errors:"
    foreach {var val} [lsort [array get ::configure::errors]] {
	puts "$var = $val"
    }
}

# Everything ok.  Write the config file.
configure::writeconfigs configs.tcl

