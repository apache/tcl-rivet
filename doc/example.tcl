# we have complete access to the interpreter here, so it is best to
# run per-page code in a namespace, just like we do with .rvt pages.

proc getcode { filename } {
    set fl [ open $filename r ]
    set sourcecode [ read $fl ]
    close $fl
    regsub -all "&" "$sourcecode" "\\&amp;" sourcecode
    regsub -all "<" "$sourcecode" "\\&lt;" sourcecode
    regsub -all ">" "$sourcecode" "\\&gt;" sourcecode
    return $sourcecode
}

if { ! [ info exists header ] } {
    set header {
	<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
	<html>
	<head>
	<title>.tcl example</title>
	</head>
	<body bgcolor="#ffffff">
    }
}

if { ! [ info exists footer ] } {
    set footer {
	</body>
	</html>
    }
}

namespace eval request {
    hgetvars
    puts $header
    puts {
	<p>This is an example of a .tcl file being processed in Rivet</p>
	<p>Here is the source code:</p>
	<hr>
	<pre>
    }
    puts [ getcode $ENVS(SCRIPT_FILENAME) ]
    puts {
	</pre>
	<hr>
    }
    puts $footer
}