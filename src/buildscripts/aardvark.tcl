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
    set grph [ ::struct::graph ]
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
	$grph node set $name -key buildinfo {cmd "" tclcommand ""}
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
	    if { $depmtime > $mtime } {
		set rebuild 1
	    }
	}
    } else {
	set rebuild 1
    }
    
    if { $rebuild == 1} {
	if { $buildinfo(cmd) != "" } {
	    foreach cmd $buildinfo(cmd) {
		set result ""
		puts -nonewline "$node :"
		catch {
		    set cmd [uplevel #0 "subst {$cmd}" ]
		    puts ""
		    puts "\tCommand: $cmd"
		} err
                if { $err != "" } {
		    puts "Command was supposed to be: $cmd"
		    puts "This error occured: $err"
		    continue
		}
		catch {
		    set result [ eval exec $cmd ]
		} err
		if { $err != "" } {
		    puts "\tError: $err"
		    if { [ info exists ::errorCode ] && $::errorCode != "NONE" } {
			puts "\tFatal Error ($::errorCode)!"
			exit 1
		    }
		    break
		}
		
		if { $result != "" } {
		    puts "\tResult: $result"
		}
	    }
	} 
	if { $buildinfo(tclcommand) != "" } {
	    foreach tclcommand $buildinfo(tclcommand) {
		catch {
		    puts -nonewline "$node :"
		    puts ""
		    puts "\tTcl Command: $tclcommand"
		    uplevel #0 $tclcommand
		} err
                if { $err != "" } {
		    puts $err
		}
	    }
	    puts ""
	}
    }
}

# these form the 'syntax' of our mini build language
proc aardvark::command { buildcmd } {
    variable buildinfo
    lappend buildinfo(cmd) $buildcmd
	return ""
}

proc aardvark::tclcommand { tclcommand } {
    variable buildinfo
    lappend buildinfo(tclcommand) $tclcommand
    return ""
}

proc aardvark::depends { deps } {
    variable dependencies
    set dependencies $deps
    return ""
}

proc aardvark::AddNode { name rest } {
    variable grph
    variable dependencies
    variable buildinfo
    set dependencies {}
    array set buildinfo {cmd "" tclcommand ""}
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
    namespace export AddNode Run Verbose command tclcommand depends
}
