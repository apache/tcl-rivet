#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

proc getbinname { } {
    global argv
    set binname [lindex $argv 0]
    if { $binname == "" || ! [file exists $binname] } {
	puts stderr "Please supply the full name and path of the Apache executable on the command line!"
	exit 1
    }
    return $binname
}

set binname [ getbinname ]

source makeconf.tcl
makeconf $binname server.conf

switch -exact [lindex $argv 1] {
    withconfigs {
	foreach {option val} {
	    {} {}
	    RivetServerConf {GlobalInitScript "source tclconf.tcl"}
	    RivetServerConf {ChildInitScript "source tclconf.tcl"}
	    RivetServerConf {ChildExitScript "source tclconf.tcl"}
	    RivetServerConf {BeforeScript "source tclconf.tcl"}
	    RivetServerConf {AfterScript "source tclconf.tcl"}
	    RivetServerConf {ErrorScript "source tclconf.tcl"}
	    RivetServerConf {CacheSize 20}
	    RivetServerConf {UploadDirectory /tmp/}
	    RivetServerConf {UploadMaxSize 2000}
	    RivetServerConf {UploadFilesToVar yes}
	    RivetServerConf {SeparateVirtualInterps yes}
	} {
	    set apachepid [exec $binname -X -f "[file join [pwd] server.conf]" -c "$option $val" &]
	    set oput [exec [file join . rivet.test]]
	    puts $oput
	    exec kill $apachepid
	}
    } 
    startserver {
	set apachepid [exec $binname -X -f "[file join [pwd] server.conf]" &]
    }
    default {
	set apachepid [exec $binname -X -f "[file join [pwd] server.conf]" &]
	set oput [exec [file join . rivet.test]]
	puts $oput
	exec kill $apachepid
    }
}
