# $Id$

# commserver --

# This forks off an external server process with 'comm' loaded in it,
# for use as an IPC system.

package provide commserver 0.1

namespace eval ::commserver {
    set Port 35100
}

proc ::commserver::start {} {
    variable Port
    # Attempt to launch server.
    exec [info nameofexecutable] [file join [file dirname [info script]] server.tcl] $Port &
}
