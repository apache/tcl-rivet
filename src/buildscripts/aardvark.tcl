# aardvark.tcl -- aardvark make-like system

# $Id$

# Copyright 2001-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package provide aardvark 0.1
source [file join [file dirname [info script]] graph.tcl]

namespace eval aardvark {
    # the graph 'object' we use throughout.
    variable grph
    # possible verbose variable to make output noisy.
    variable vbose
    # we use these to pass information from the AddNode "mini language"
    variable buildinfo
    variable dependencies
    set vbose 0
}
proc aardvark::Verbose { } {
    variable vbose
    set vbose 1
}

# aardvark::Output --
#
#	This command is used to display output from the processing of
#	nodes.
#
# Arguments:
#	type - the type of thing to output.
#	txt - text to be displayed

proc aardvark::Output { type txt } {
    switch -exact -- $type {
	command {
	    puts "Command $txt"
	}
	error {
	    puts "Error: $txt"
	}
	output {
	    puts "Output: $txt"
	}
	node {
	    puts -nonewline "$txt	-> "
	}
	result {
	    puts "Result: $txt"
	}
    }
}

# aardvark::createnode --
#
#	Creates one node, if it doesn't exist, and creates its data.
#
# Arguments:
#	name - node name.
#
# Side Effects:
#	None.

proc aardvark::createnode { name } {
    variable grph
    if { ! [$grph node exists $name] } {
	$grph node insert $name
	$grph node set $name -key buildinfo {sh "" tcl ""}
    }
}


# aardvark::runbuildcommand --
#
#	This command is run when we walk the dependency graph.
#
# Arguments:
#	direction - which way the graph is being walked.
#	graphname - the graph in question.
#	node - the node in question.
#
# Side Effects:
#	Runs the build command.
#
# Results:
#	None.

proc aardvark::runbuildcommand { direction graphname node } {
    variable grph
    set rebuild 0
    set mtime 0
    set deps [$grph nodes -out $node]
    array set buildinfo [$grph node get $node -key buildinfo]

    # check file time
    if { [file exists $node] } {
	set mtime [file mtime $node]
    }

    # rebuild if dependencies are newer than file
    if { [llength $deps] > 0 } {
	foreach dep $deps {
	    if { [file exists $dep] } {
		set depmtime [file mtime $dep]
	    } else {
		set depmtime 0
	    }
	    if { $depmtime > $mtime } {
		set rebuild 1
	    }
	}
    } else {
	set rebuild 1
    }

    if { $rebuild == 1 && [info exists buildinfo(cmds)] } {
	Output node "$node"
	foreach cmd $buildinfo(cmds) {
	    if { [lindex $cmd 0] == "sh" } {
		set sh [join [lrange $cmd 1 end]]
		set result ""
		if { [catch {
		    set sh [uplevel \#0 "subst {$sh}" ]
		} err] } {
		    set doerr 1
		} else {
		    set doerr 0
		}
		Output command "(sh): $sh"
		if { $doerr } {
		    Output error "$err"
		    continue
		}
		if { [info exists ::errorCode] } {
		    unset ::errorCode
		}
		set fd [eval [list open "| $sh" w]]
		if { [catch {
		    close $fd
		} err] } {
		    # If there is an errorcode, that means that it is
		    # really a problem, and not just something comming
		    # out on stderr.
		    if { [info exists ::errorCode] && \
			     $::errorCode != "NONE" } {
			if { $err != "" } {
			    Output error "$err"
			}
			error "$cmd"
		    } else {
			if { $err != "" } {
			    Output result "$err"
			}
		    }
		}

		if { $result != "" } {
		    Output result "$result"
		}
	    }
	    if { [lindex $cmd 0] == "tcl" } {
		set tcl [join [lrange $cmd 1 end]]
		catch {
		    Output command "(tcl): $tcl"
		    uplevel \#0 $tcl
		} err
		if { $err != "" } {
		    Output result  $err
		}
	    }
	}
    }
}

# These are the commands of our mini build language.

# aardvark::sh --
#
#	Adds a shell command to be run.
#
# Arguments:
#	The command and its arguments.
#
# Side Effects:
#	Adds it to the processing instructions for the node.
#
# Results:
#	None.

proc aardvark::sh { args } {
    variable buildinfo
    set arg [join $args]
    lappend buildinfo(cmds) [list sh $arg]
    return ""
}

# aardvark::tcl --
#
#	Adds a tcl command to be evaluated.
#
# Arguments:
#	The tcl script.
#
# Side Effects:
#	Adds the script to the processing instructions for the node.
#
# Results:
#	None.

proc aardvark::tcl { args } {
    variable buildinfo
    set arg [join $args]
    lappend buildinfo(cmds) [list tcl $arg]
    return ""
}

# aardvark::depends --
#
#	Adds dependencies for the node.
#
# Arguments:
#	A list of dependencies.
#
# Side Effects:
#	The node will depend on the nodes listed to be run before it
#	can be run.
#
# Results:
#	None.

proc aardvark::depends { args } {
    variable dependencies
    set dependencies [join $args]
    return ""
}

# aardvark::AddNode --
#
#	Adds a node, its dependencies and processing instructions to
#	the graph.
#
# Arguments:
#	name - name of the node.
#	body - the script to add dependencies, build instructions and
#	so on.
#
# Side Effects:
#	Creates a node in the dependency graph with the associated
#	build instructions.
#
# Results:
#	None.

proc aardvark::AddNode { name body } {
    variable grph
    variable dependencies
    variable buildinfo

    if { ! [info exists grph] } {
	# Lazy graph creation.
	set grph [::struct::graph::graph]
    }

    set dependencies {}
    array set buildinfo {cmds ""}
    set self $name
    catch {
	uplevel #0 $body
    } err
    if { $err != "" } {
	puts "Error: $err"
    }
    createnode $name
    $grph node set $name -key buildinfo [array get buildinfo]
    foreach dep $dependencies {
	#	    puts -nonewline "$dep/"
	createnode $dep
	$grph arc insert $name $dep
    }
}

# aardvark::Nodes --
#
#	Return a list of all the nodes.
#
# Arguments:
#	None.
#
# Results:
#	List of all the nodes.

proc aardvark::Nodes { } {
    variable grph
    return [$grph nodes]
}


# aardvark::Run --
#
#	Run the build process.
#
# Arguments:
#	None.
#
# Side Effects:
#	Process the list of nodes, running the build commands
#	associated with each one.
#
# Results:
#	None.

proc aardvark::Run { } {
    variable grph
    set start [lindex $::argv 0]
    if { [catch {
	if { $start != "" } {
	    $grph walk $start -order post -command runbuildcommand
	} else {
	    puts [$grph walk all -order post -command runbuildcommand]
	}
    } err] } {
	puts "Compilation failed on command \"$err\""
    }
    $grph destroy
    unset grph
}

# aardvark::getconfigs --
#
#	Fetch configuration settings from a file, defaulting to
#	configs.tcl.  These are generated by the configure.tcl and
#	configure.in.tcl scripts.
#
# Arguments:
#	None.
#
# Side Effects:
#	Creates variables in the current namespace from variables in
#	the configs.tcl file.
#
# Results:
#	None.

proc aardvark::getconfigs { {filename configs.tcl} } {
    catch {namespace delete ::configs}
    if { [catch {source $filename}] } {
	puts stderr "You must run ./configure.tcl before running ./make.tcl"
	exit 1
    }

    # Take any options that are passed to us on the command line, in
    # order to be able to override ./configure options.
    foreach arg [lrange $::argv 1 end] {
	# set var and val
	foreach {var val} [split $arg =] {}
	set ::configs::$var $val
    }

    foreach var [info vars ::configs::*] {
	set var [namespace tail $var]
	uplevel [list set $var [set ::configs::$var]]
    }


}

namespace eval aardvark {
    namespace export AddNode Run Verbose sh tcl depends Nodes getconfigs
}
