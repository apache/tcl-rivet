#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

source [file join apachetest apachetest.tcl]

apachetest::getbinname $argv

apachetest::makeconf server.conf {
    LoadModule rivet_module [file join $CWD .. src mod_rivet[info sharedlibextension]]

    <IfModule mod_mime.c>
    TypesConfig $CWD/mime.types
    AddLanguage en .en
    AddLanguage it .it
    AddLanguage es .es
    AddType application/x-httpd-rivet .rvt
    AddType application/x-rivet-tcl .tcl
    </IfModule>

    RivetServerConf UploadFilesToVar on

    <IfDefine SERVERCONFTEST>
    RivetServerConf BeforeScript 'puts "Page Header"'
    RivetServerConf AfterScript 'puts "Page Footer"'
    </IfDefine>

    <IfDefine DIRTEST>
    <Directory />
    RivetDirConf BeforeScript 'puts "Page Header"'
    RivetDirConf AfterScript 'puts "Page Footer"'
    </Directory>
    </IfDefine>
}

# copy the rivet init files.

if { ! [file exists [file join rivet init.tcl]] } {
    file copy -force [file join .. rivet] .
}

# we do this to keep tcltest happy - it reads argv...
set commandline [lindex $argv 1]
set argv {}

switch -exact $commandline {
    startserver {
	apachetest::startserver
    }
    default {
	source [file join . rivet.test]
    }
}
