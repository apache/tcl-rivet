# aardvark make-like system
# $Id$

# Copyright (c) 2001-2003 Apache Software Foundation.  All Rights
# reserved.

# See the LICENSE file for licensing terms.

package provide aardvark 0.1
package require struct

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
	    puts -nonewline "$txt -> "
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
    if { ! [ $grph node exists $name ] } {
	$grph node insert $name
	$grph node set $name -key buildinfo {sh "" tcl ""}
    }
}

# the command that gets run when we walk the graph.
proc aardvark::runbuildcommand { direction graphname node } {
    variable grph
    set rebuild 0
    set mtime 0
    set deps [ $grph nodes -out $node ]
    array set buildinfo [ $grph node get $node -key buildinfo ]

    # check file time
    if { [ file exists $node ] } {
	set mtime [ file mtime $node ]
    }

    # rebuild if dependencies are newer than file
    if { [ llength $deps ] > 0 } {
	foreach dep $deps {
	    if { [ file exists $dep ] } {
		set depmtime [ file mtime $dep ]
	    } else {
		set depmtime 0
	    }
	    if { $depmtime >= $mtime } {
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

# these are the commands of our mini build language

# Adds a shell command to be executed.
proc aardvark::sh { args } {
    variable buildinfo
    set arg [join $args]
    lappend buildinfo(cmds) [list sh $arg]
    return ""
}

# Adds a Tcl command to be evaluated.
proc aardvark::tcl { args } {
    variable buildinfo
    set arg [join $args]
    lappend buildinfo(cmds) [list tcl $arg]
    return ""
}

# Adds a file dependency.
proc aardvark::depends { args } {
    variable dependencies
    set dependencies [join $args]
    return ""
}

# Add a node to the dependency tree.
proc aardvark::AddNode { name rest } {
    variable grph
    variable dependencies
    variable buildinfo

    if { ! [info exists grph] } {
	# Lazy graph creation.
	set grph [ ::struct::graph::graph ]
    }

    set dependencies {}
    array set buildinfo {cmds ""}
    set self $name
    catch {
	uplevel #0 $rest
    } err
    if { $err != "" } {
	puts "Error: $err"
    }
    createnode $name
    $grph node set $name -key buildinfo [ array get buildinfo ]
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

namespace eval aardvark {
    namespace export AddNode Run Verbose sh tcl depends Nodes
}
