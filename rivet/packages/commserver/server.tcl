# This is a server that is detached from the main Apache process, in
# order to provide inter-process comunication via tcllib's comm
# package.

# $Id$

# TODO:
# Add some code for serializing variables between sessions.
# Possibilities for keeping sync'ed include: catching signals and
# shutting down gracefully, or periodically saving to disk.

package require comm

set Port [lindex $argv 0]
if { [catch {
    comm::comm config -port $Port
} err] } {
    # Ok, something failed.  This should mean that another copy is
    # already running.
    puts stderr "Could not launch commserver on port $Port, exiting"
} else {
    puts stderr "Launched commserver on port $Port"
    vwait forever
}
