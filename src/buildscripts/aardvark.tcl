# aardvark make-like system
# $Id$

# Copyright (c) 2001 Apache Software Foundation.  All Rights reserved.

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
    # create a graph to use.
    set grph [ ::struct::graph::graph ]
}
proc aardvark::Verbose { } {
    variable vbose
    set vbose 1
}

# creates one node, if it doesn't exist, and creates it's data
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
	foreach cmd $buildinfo(cmds) {
	    if { [lindex $cmd 0] == "sh" } {
		set sh [join [lrange $cmd 1 end]]
		set result ""
		puts -nonewline "$node :"
		catch {
		    set sh [uplevel \#0 "subst {$sh}" ]
		    puts ""
		    puts "\tCommand: $sh"
		} err
		if { $err != "" } {
		    puts "Sh was supposed to be: $sh"
		    puts "This error occured: $err"
		    continue
		}
		if { [info exists ::errorCode] } {
		    unset ::errorCode
		}
		set fd [eval [list open "| $sh" r]]
		catch {
		    close $fd
		} err
		if { $err != "" } {
		    puts "Output: $err"
		}
		if { [info exists ::errorCode] && $::errorCode != "NONE" } {
		    puts "\tFatal Error ($::errorCode) ($::errorInfo)!"
		    break
		}

		if { $result != "" } {
		    puts "\tResult: $result"
		}
	    }
	    if { [lindex $cmd 0] == "tcl" } {
		set tcl [join [lrange $cmd 1 end]]
		catch {
		    puts -nonewline "$node :"
		    puts ""
		    puts "\tTcl Command: $tcl"
		    uplevel \#0 $tcl
		} err
		if { $err != "" } {
		    puts $err
		}
		puts ""
	    }
	}
    }
}

# these are the commands of our mini build language

# Adds a shell command to be executed.
proc aardvark::sh { buildcmd } {
    variable buildinfo
    lappend buildinfo(cmds) [list sh $buildcmd]
    return ""
}

# Adds a Tcl command to be evaluated.
proc aardvark::tcl { tclcommand } {
    variable buildinfo
    lappend buildinfo(cmds) [list tcl $tclcommand]
    return ""
}

# Adds a file dependency.
proc aardvark::depends { deps } {
    variable dependencies
    set dependencies $deps
    return ""
}

# Add a node to the dependency tree.
proc aardvark::AddNode { name rest } {
    variable grph
    variable dependencies
    variable buildinfo
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

proc aardvark::Run { } {
    global ::argv
    variable grph
    set start [ lindex $::argv 0 ]
    if { $start != "" } {
	$grph walk $start -order post -command runbuildcommand
    } else {
	$grph walk all -order post -command runbuildcommand
    }
}

namespace eval aardvark {
    namespace export AddNode Run Verbose sh tcl depends
}
