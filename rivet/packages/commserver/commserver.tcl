package provide commserver 0.1

namespace eval ::commserver {
    # This has to be the same as in server.tcl.
    set Port 35100
}

proc ::commserver::start {} {
    variable Port
    # Attempt to launch server.
    exec [info nameofexecutable] [file join [file dirname [info script]] server.tcl] $Port &
}
