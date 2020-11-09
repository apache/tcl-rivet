# apachetest.tcl -- Tcl-based Apache test suite

# Copyright 2001-2020 The Apache Tcl Team / The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This test suite provides a means to create configuration files, and
# start apache with user-specified options.  All it needs to run is
# the name of the Apache executable, which must, however, be compiled
# with the right options.

set auto_path [linsert $auto_path 0 [file dirname [info script]]]
package provide apachetest 0.1

# kill --
#	kill a process and wait until it's really gone
#	(mimic TclX's kill and wait procs)
#
# Arguments:
#	signal - signal to send (e.g. TERM, HUP, KILL...)
#	pid - process id
#
# Side Effects:
#	kills a running process/processes (if permitted)
#
# Results:
#	None.

proc kill {signal pid} {
    catch {exec kill -s $signal $pid}
    after 100
    set i 100
    while {1} {
        catch {exec ps -p $pid} out
        if {[regexp $pid $out]} {
            incr i 250
            after 250
        } else {
            break
        }
    }
    if {$i > 100} {
        puts "Waiting [expr {$i/1000.0}] seconds until process was killed"
    }
}

#package require http 2.4.5
source [file join [file dirname [info script]] http.tcl]

namespace eval apachetest {

    set debug 1

    # name of the apache binary, such as /usr/sbin/httpd
    variable binname ""
	if ![info exists ::httpd_version ] {
    	puts stderr "Please create httpd_version variable in global namespace"
	    exit 1
	} 
	variable httpd_version  $::httpd_version
    # this file should be in the same directory this script is.
    variable templatefile [file join [file dirname [info script]] template.conf.2.tcl]
}

# apachetest::need_modules --
#
#	Tell the test suite which modules we *need* to have.  The test
#	suite will then check to see if these are either 1) compiled
#	into the server or 2) loaded in its configuration file.
#
# Arguments:
#	args
#
# Side Effects:
#	Sets up the rest of the apachetest tools to find the needed
#	modules.
#
# Results:
#	None.

proc apachetest::need_modules { modlist } {
    variable module_assoc
    foreach module_pair $modlist {
        set module_assoc([lindex $module_pair 0]) [lindex $module_pair 1]
    }
}

# apachetest::connect --
#
#	Attempt to open a socket to the web server we are using in our
#	tests.  Try for 10 seconds before giving up.
#
# Arguments:
#	None.

proc apachetest::connect { } {
    set starttime [clock seconds]
    set diff 0
    # We try for 10 seconds.
    while { $diff < 10 } {
        if { ! [catch {
            set sk [socket localhost 8081]
        } err]} {
            close $sk
            return
        }
        set diff [expr {[clock seconds] - $starttime}]
    }
}

# apachetest::start --
#
#	Start the web server in the background.  After running the
#	script specified, stop the server.
#
# Arguments:
#	options - command line options to pass to the web server.
#	conftext - text to insert into test.conf.
#	code - code to run.
#
# Side Effects:
#	Runs 'code' in the global namespace.
#
# Results:
#	None.

proc apachetest::start { options conftext code } {
    variable serverpid 0
    variable binname
    variable debug
    variable httpd_version

    set fn [file join [pwd] test.conf]
    catch {file delete -force $fn}
    set fl [open $fn w]
    puts $fl [uplevel [list subst $conftext]]
    close $fl

    #OpenBSD related workaround, their stock apache tries to chroot by default
    #have to  add -u to the arguments to prevent this
    catch {exec uname} uname_str
    if {[string equal $uname_str OpenBSD]} {
        set server_args "-u -X -f"
    } else {
        set server_args [list -X -f]
    }
    # There has got to be a better way to do this, aside from waiting.
    set serverpid [eval exec $binname $server_args \
	       [file join [pwd] server.conf] $options >& apachelog.txt &] 
    apachetest::connect
    if { $debug > 0 } {
	    puts "Apache started as PID $serverpid"
    }

    if { [catch { uplevel $code } err] } { puts stderr $::errorInfo }

    # apache2 binary started with -X reacts to SIGQUIT and ignores TERM, but KILL is most reliable
    kill KILL $serverpid
}

# startserver - start the server with 'options'.

proc apachetest::startserver { args } {
    variable binname
    variable debug

    if { [catch {
        if { $debug } {
            puts "$binname -X -f [file join [pwd] server.conf] [concat $args]"
        }
        set serverpid [eval exec $binname -X -f "[file join [pwd] server.conf]" [concat $args]]
    } err] } {
        puts "$err"
    }
}

# getbinname - get the name of the apache binary, and check to make
# sure it's ok.  The user should supply this parameter.

proc apachetest::getbinname { bname } {
    variable binname

    set binname $bname
    if { $binname == "" || ! [file executable $binname] } {
	    error "Invalid executable '$binname', please supply the full name and path of the Apache executable on the command line."
    }
    return $binname
}

# get the modules that are compiled into Apache directly, and return
# the XXX_module name.  Check also for the existence of mod_so, which
# we need to load the shared object in the directory above...

proc apachetest::getcompiledin { binname } {
    variable module_assoc

    if {[catch {
        set bin [open [list | "$binname" -l] r]
        set compiledin [read $bin]
        close $bin} e einfo]} {
            puts "error: $e"
            puts "error info: $einfo"

            error "caught error in apachetest::getcompledin"
    }
    set modlist [split $compiledin]
    set compiledin [list]
    set mod_so_present 0
    foreach entry $modlist {
        if { [regexp {(.*)\.c$} $entry match modname] } {
            if { $modname == "mod_so" } {
                set mod_so_present 1
            }
            if { [info exists module_assoc($modname)] } {
                lappend compiledin $module_assoc($modname)
            }
        }
    }
    if { $mod_so_present == 0 } {
        error "We need mod_so in Apache to run these tests"
    }
    return $compiledin
}

# find the httpd.conf file

proc apachetest::gethttpdconf { binname } {
    set bin [open [list | $binname -V] r]
    set options [read $bin]
    catch {close $bin}
    regexp {SERVER_CONFIG_FILE="(.*?)"} "$options" match filename

    if {![file exists $filename]} {

        # see if we can find something by combining
        # HTTP_ROOT + SERVER_CONFIG_FILE

        regexp {HTTPD_ROOT="(.*?)"} "$options" match httpdroot
        set completename [file join $httpdroot $filename]
        if {![file exists $completename]} {
            error "neither '$filename' or '$completename' exists"
        }
        return $completename
    }

    return $filename
}

# apachetest::getallincludes --
#
#	Reads the conf file, and returns its text, plus the text in
#	all the files that it Includes itself.
#
# Arguments:
#	conffile - file to read.
#
# Side Effects:
#	None.
#
# Results:
#	Text of configuration files.

proc apachetest::getallincludes { conffile } {
    if [file exists $conffile] {
	    set fl [open $conffile r]
	    set data [read $fl]
	    close $fl

	    set newdata {}
	    foreach line [split $data \n] {
		# Look for Include lines.
            if { [regexp -line {^[^\#]*Include +(.*)} $line match file] } {
                puts "including files from $file"
                set file [string trim $file]

                # Include directives accept as argument a file, a directory
                # or a glob-style file matching pattern. Patterns usually match
                # many files, but are not directories, so we have to handle 
                # all these 3 cases

                # we use the glob command to tell whether we are dealing with
                # a pure file expression or a matching pattern

                set matched_files [glob -nocomplain $file]
                set matched_files_n [llength $matched_files]
                if {$matched_files_n > 1} {
                    foreach fl $matched_files {
                        puts "including $fl"
                        if [file  exists $fl] {
                            append newdata [getallincludes $fl]
                        }
                    }
                } elseif {$matched_files_n == 1} {
                    set file $matched_files
                    if { [file isdirectory $file] } {
                        foreach fl [glob -nocomplain [file join $file *]] {
                            puts "including $fl"
                            if [file  exists $fl] {
                                append newdata [getallincludes $fl]
                            }
                        }
                    } else {
                        append newdata [getallincludes $file]
                    }
                }
            }
	    }
	    append data $newdata
	    return $data
   } else {
	return
   }
}

# apachetest::getloadmodules --
#
#	Get the LoadModule lines for modules that we want to load.
#
# Arguments:
#	conffile - the name of the conf file to read.
#	needtoget - list of modules that we want to load.
#
# Side Effects:
#	None.
#
# Results:
#	Returns a string suitable for inclusion in a conf file.


proc apachetest::getloadmodules { conffile needtoget } {
    puts "checking $conffile "
    set confdata [getallincludes $conffile]
    set loadline [list]
    regexp -line {^[^#]*(ServerRoot[\s]?[\"]?)([^\"]+)()([\"]?)} $confdata \
    match sub1 server_root_path sub2 
    foreach mod $needtoget {

    	# Look for LoadModule lines.
        puts -nonewline "check conf line for $mod module..."
        flush stdout
        if { ! [regexp -line "^\[^\#\]*(LoadModule\\s+$mod\\s+.+)\$"\
                        $confdata match line] } {
            error "No LoadModule line for $mod\!"
        } else {
            puts ok

            set raw_path [join [lrange [split $line { }] 2 end]]
            #trimming leading whitespaces
            set path [string trimleft $raw_path]
            if ![string equal [file pathtype $line] "absolute"] {
                set absolute_path [file join $server_root_path $path]
                lappend loadline "[join [lrange [split $line " "]  0 1]] $absolute_path"
            } else {
                lappend loadline $line
            }
        }
    }
    return [join $loadline "\n"]
}

# Compare what's compiled in with what we need.

proc apachetest::determinemodules { binname } {
    variable module_assoc
    set compiledin [lsort [getcompiledin $binname]]
    set conffile [gethttpdconf $binname]

    foreach {n k} [array get module_assoc] {
        lappend needed $k
    }
    set needed [lsort $needed]

    puts "needed: $needed"

    set needtoget [list]
    foreach mod $needed {
        if { [lsearch $compiledin $mod] == -1 } {
            lappend needtoget $mod
        }
    }
    if {$needtoget == ""} {
        return ""
    } else {
        return [getloadmodules $conffile $needtoget]
    }
}

# apachetest::makeconf --
#
#	Creates a config file and writes it to disk.
#
# Arguments:
#	outfile - the file to create/write to.
#	extra - extra config options to add.
#
# Side Effects:
#	Creates a new config file.
#
# Results:
#	None.

proc apachetest::makeconf { outfile bridge {extra ""} } {
    variable binname
    variable templatefile
    set CWD [pwd]

    #getting uid and gid of user
    catch {exec id} raw_string		

# username and group no longer needed. User and Group directives removed from
# main conffile template (#Bug 53396)

#   set username  [lindex [regexp -inline {(uid=)([\d]+)(\()([^\)]+)(\))} $raw_string]  4]
#   set group  [lindex [regexp -inline {(gid=)([\d]+)(\()([^\)]+)(\))} $raw_string]  4]

    # replace with determinemodules
    set LOADMODULES [determinemodules $binname]

    puts "reading template from $templatefile"
    set fl [open [file join . $templatefile] r]
    set template [read $fl]
    close $fl

# configure a specific bridge if -bridge passed as argument

    if {$bridge == "default"} {
        set bridge_conf "# Default bridge"
    } else {
        set bridge_conf "RivetServerConf MpmBridge $bridge"
    }
    set extra [string map [list BRIDGE $bridge_conf] $extra]

    append template $extra

    puts $template

    set out [subst $template]

    set of [open $outfile w]
    puts $of "$out"
    close $of
}
