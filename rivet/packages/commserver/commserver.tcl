# $Id$

# commserver --

# This forks off an external server process with 'comm' loaded in it,
# for use as an IPC system.

package provide commserver 0.1

namespace eval ::commserver {
    set Port 35100
    set scriptlocation [info script]
    set wait 5
}

proc ::commserver::start {} {
    variable Port
    variable scriptlocation
    variable wait
    # Attempt to launch server.
    exec [info nameofexecutable] \
	[file join [file dirname $scriptlocation] server.tcl] $Port &

    set starttime [clock seconds]
    # If we don't get a connection in $wait seconds, abort.
    while { [clock seconds] - $starttime < $wait } {
	if { ![catch {
	    comm::comm send $Port {
		puts stderr "Commserver server started on $Port"
	    }
	}] } {
	    return
	}
    }
    error "Connection to $Port failed after $wait seconds of trying."
}