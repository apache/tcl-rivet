# commserver.tcl --

# This forks off an external server process with 'comm' loaded in it,
# for use as an IPC system.

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

# $Id$

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