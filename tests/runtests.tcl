#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

source [file join apachetest apachetest.tcl]

apachetest::getbinname $argv

apachetest::need_modules {
    {mod_log_config	  config_log_module}
    {mod_mime		mime_module}
    {mod_negotiation	 negotiation_module}
    {mod_dir		         dir_module}
    {mod_access	      access_module}
    {mod_auth		auth_module}
}

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

# Copy the rivet init files.
file delete -force rivet
file copy -force [file join .. rivet] .

# If 'startserver' is specified on the command line, just start up the
# server without running tests.

switch -exact -- [lindex $argv 1] {
    startserver {
	apachetest::startserver
    }
    default {
	set argv [lrange $argv 1 end]
	source [file join . rivet.test]
    }
}
