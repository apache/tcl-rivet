###
## This package is a compatibility layer between Rivet and mod_dtcl.
##
## All of the mod_dtcl commands call their Rivet equivalents and return the
## proper responses.
###

# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package provide Dtcl 1.0

proc hgetvars {} {
    uplevel {
        catch {unset VARS}
	load_env ENVS
	load_cookies COOKIES
    }
    set vars [var all]
    foreach {name val} $vars {
	uplevel [list set VARS($name) "$val"]
    }
    unset vars
}

proc hputs {args} {
    set nargs [llength $args]
    if {$nargs < 1 || $nargs > 2} {
	return -code error {wrong # args: should be "hputs ?-error? text"}
    }

    if {$nargs == 2} {
	set string [lindex $args 1]
    } else {
	set string [lindex $args 0]
    }

    puts $string
}

proc hflush {} {
    flush stdout
}

proc dtcl_info {} { }
