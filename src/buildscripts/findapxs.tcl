# $Id$

# Attempt to find the 'apxs' file, from the Apache distribution.

namespace eval findapxs {
    set apxsDirList {
	/usr/local/apache/bin
	/usr/local/etc/apache/bin
	/usr/bin
	/usr/sbin
	/usr/local/bin
	/usr/local/sbin
    }

    proc FindAPXS {{apxs ""}} {
	variable apxsDirList

	if {[string length $apxs]} {
	    if {[file executable $apxs]} { return $apxs }
	}

	set apxs ""
	foreach dir $apxsDirList {
	    if {![file executable [file join $dir apxs]]} { continue }
	    set apxs [file join $dir apxs]
	    break
	}

	if {$apxs == ""} {
	    error "could not find Apache Extension Tool apxs in $apxsDirList\nThe location of apxs can also be specified using the \"-with-apxs\" option on the configure command line."
	}

	return $apxs
    }
}
