# rivet_ncgi.tcl -- Rivet ncgi compatibility layer

# $Id$

package provide ncgi 1.0

package require fileutil
package require http

namespace eval ncgi {
}

# ::ncgi::parse --
#
#	Mostly a no-op for Rivet, although it loads up the ::env
#	variable.
#
# Arguments:
#	None.
#
# Side Effects:
#	Modifies ::env environment to include stuff from the request.
#
# Results:
#	None.

proc ::ncgi::parse {} {
    load_env ::env
}

# ::ncgi::value --
#
#	Returns the value of a 'cgi' variable.
#
# Arguments:
#	key - variable name.
#	default - default value should it not exist.
#
# Side Effects:
#	None.
#
# Results:
#	The value of the specified variable, or {} if it is empty.

proc ::ncgi::value {key {default {}}} {
    if { [var exists $key] } {
	return [var get $key]
    } else {
	return $default
    }
}

# ::ncgi::encode --
#
#	HTML encode a string.
#
# Arguments:
#	string - text to encode.
#
# Side Effects:
#	None.
#
# Results:
#	Encoded string.

proc ::ncgi::encode {string} {
    return [http::formatQuery $string]
}

# ::ncgi::importFile --
#
#	See the ncgi documentation.

proc ::ncgi::importFile {cmd var {filename {}}} {
    switch -exact -- $cmd {
	-server {
	    if { $filename == {} } {
		set filename [::fileutil::tempfile ncgi]
	    }
	    upload save $var $filename
	    return $filename
	}
	-client {
	    return [upload filename $var]
	}
	-type {
	    return [upload type]
	}
	-data {
	    return [upload data $var]
	}
	default {
	    error "Unknown subcommand to ncgi::import_file: $cmd"
	}
    }
}

# ::ncgi::empty --
#
#	Returns 1 if the CGI variable in question is not set, 0 if it
#	is set.
#
# Arguments:
#	name - variable name.
#
# Side Effects:
#	None.
#
# Results:
#	1 or 0.

proc ::ncgi::empty {name} {
    expr {! [var exists $name]}
}

# ::ncgi::::redirect --
#
#	Generate a redirect.
#
# Arguments:
#	uri - new URL to go to.
#
# Side Effects:
#	Must be done before puts statements.
#
# Results:
#	None.

proc ::ncgi::::redirect {uri} {
    headers redirect $uri
}