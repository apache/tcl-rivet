#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

# Copyright 2001-2005 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $Id$

set auto_path [linsert $auto_path 0 [file join [file dirname [info script]] apachetest]]

puts stderr "runtests.tcl is running with auto_path: $auto_path"

proc runtests_usage {} {
    puts stderr "Usage: $::argv0 /path/to/apache/httpd ?startserver?"
    exit 1
}

proc get_httpd_version {httpd {bd broken_down_version}} {
    upvar 1 $bd broken_down_version

	catch {exec $httpd -v} raw_string

	set version_l [regexp -inline {([0-9]{1,})\.([0-9]{1,})\.([0-9]{1,})} $raw_string]
	if {[llength $version_l]} {

        lassign $version_l m major minor patchlevel
        set broken_down_version [list $major $minor $patchlevel]

    } else {

        return -code error "could not identify the Apache httpd version from '$raw_string'"

    }
    return $m
}

if { [llength $argv] < 1 } {
    runtests_usage
} else {
    set httpd_version [get_httpd_version [lindex $argv 0]]
}

puts stderr "Tests will be run against apache ${httpd_version} version"

package require apachetest

if { [encoding system] eq "utf-8" } {
    puts stderr {
        System encoding is utf-8 - this is known to cause problems
        with the test environment!  Continuing with tests in 5 seconds
        using the iso8859-1 encoding.
    }
    after 5000
}

if { [catch {
    apachetest::getbinname $argv
} err ] } {
    puts stderr $err
    runtests_usage
}

apachetest::need_modules {
    {mod_mime           mime_module}
    {mod_negotiation    negotiation_module}
    {mod_dir            dir_module}
    {mod_log_config     log_config_module}
    {mod_authz_core     authz_core_module}
    {mod_authz_host     authz_host_module}
    {mod_unixd          unixd_module}
    {mod_mpm_prefork    mpm_prefork_module}
}

apachetest::makeconf server.conf {
    LoadModule rivet_module         [file join $CWD .. src/.libs mod_rivet[info sharedlibextension]]
# User and Group directives removed to ease dependency of test suite from the output of command 'id' (from which
# the values for these directives were inferred (Bug #53396)

    <IfModule mod_mime.c>
        TypesConfig $CWD/mime.types
        AddLanguage en .en
        AddLanguage it .it
        AddLanguage es .es
        AddType application/x-httpd-rivet .rvt
        AddType application/x-rivet-tcl   .tcl
    </IfModule>

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

    # We can use this to include our own stuff for each test.
    Include test.conf

    # For testing, we want core dumps.
    CoreDumpDirectory $CWD
}

# Copy the rivet init files.
file delete -force rivet
file copy   -force [file join .. rivet] .
set env(TCLLIBPATH) [file normalize [file join [file dirname [info script]] rivet]]

# If 'startserver' is specified on the command line, just start up the
# server without running tests.

puts "running test with arguments: $argv"
switch -exact -- [lindex $argv 1] {
    startserver {
	    apachetest::startserver
    }
    default {
        set argv [lrange $argv 1 end]

        source [file join . rivet.test]
    }
}
