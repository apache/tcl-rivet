# server.tcl --

# This is a server that is detached from the main Apache process, in
# order to provide inter-process comunication via tcllib's comm
# package.

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

# TODO:
# Add some code for serializing variables between sessions.
# Possibilities for keeping sync'ed include: catching signals and
# shutting down gracefully, or periodically saving to disk.

package require comm

if {![info exists argv]} { return }

set Port [lindex $argv 0]
if { [catch {
    comm::comm config -port $Port
} err] } {
    # Ok, something failed.  This should mean that another copy is
    # already running.
    puts stderr "Could not launch commserver on port $Port, exiting"
    exit 1
} else {
    puts stderr "Launched commserver on port $Port"
    vwait forever
}
