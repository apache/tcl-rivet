#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

source makeconf.tcl

set binname [ getbinname ]

makeconf $binname server.conf

# We could include some sort of loop here, running the server + tests
# with different config options.  Something along these lines,
# although I think it could be done better - davidw.

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
    set apachepid [ exec $binname -X -f "[file join [pwd] server.conf]" -c "$option $val" & ]
    set oput [ exec [file join . rivet.test] ]
    puts $oput
    exec kill $apachepid
}