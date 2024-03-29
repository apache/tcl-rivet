#
# init.tcl -- 
#
#
# Copyright 2002-2017 The Apache Rivet Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package require rivetlib 3.2

# the ::rivet namespace is created in mod_rivet_commoc.c:Rivet_PerInterpInit
# namespace eval ::rivet {} ; ## create namespace
namespace eval ::Rivet {} ; ## create namespace

## ::Rivet::init
##
## Initialize the interpreter with all that Rivet goodness. This is called
## once when this file is loaded (down at the bottom) and sets up the interp
## for all things Rivet.

proc ::Rivet::init {} {
    set ::Rivet::init [info script]
    set ::Rivet::root [file dirname $::Rivet::init]
    set ::Rivet::packages [file join $::Rivet::root packages]
    set ::Rivet::rivet_tcl [file join $::Rivet::root rivet-tcl]

    ## Setup auto_path within the interp to include all the places
    ## we've stored Rivet's scripts: rivet-tcl, packages, packages-local,
    ## packages$tcl_version, init_script_dir, and .

    ## Put these at the head of the list.
    set ::auto_path [linsert $::auto_path 0 $::Rivet::root \
        $::Rivet::rivet_tcl $::Rivet::packages $::Rivet::packages-local]

    ## This will allow users to create proc libraries and tclIndex files
    ## in the local directory that can be autoloaded.
    ## Perhaps this must go to the front of the list to allow the user
    ## to override even Rivet's procs.
    lappend ::auto_path ${::Rivet::packages}${::tcl_version} .

    ## As we moved the command set to the ::rivet namespace we
    ## still want to guarantee the commands to be accessible
    ## at the global level by putting them on the export list.
    ## Importing the ::rivet namespace is deprecated and we should
    ## make it clear in the manual.

    if {[string is true -strict [::rivet::inspect ExportRivetNS]]
        || [string is true -strict [::rivet::inspect ImportRivetNS]]} {

        set ::rivet::cmd_export_list \
            [tcl_commands_export_list $::Rivet::rivet_tcl]

        ## init.tcl is run by mod_rivet (which creates the ::rivet
        ## namespace) but it gets run standalone by mkPkgindex during
        ## the installation phase. We have to make sure the procedure
        ## won't fail in this case, so we check for the existence of
        ## the variable.
        namespace eval ::rivet {
            ## Commands in cmd_export_list are prefixed with ::rivet,
            ## so we have to remove it to build an export list.
            set export_list [list]
            foreach c $cmd_export_list {
                lappend export_list [namespace tail $c]
            }

            namespace export {*}$export_list
        }
    }

    ## If we are running from within mod_rivet we have already
    ## defined ::rivet::exit (mod_rivet_common.c: Rivet_PerInterpInit)
    ## and we move Tcl's exit command out of the way and replace it with
    ## our own that handles bailing from a page request properly.

    if {[info commands ::rivet::exit] != ""} {

        rename ::exit ::Rivet::tclcore_exit
        proc ::exit {code} {
            if {![string is integer -strict $code]} { set code 0 }
            ::rivet::exit $code
        }

    }

    ## If Rivet was configured for backward compatibility, import commands
    ## from the ::rivet namespace into the global namespace.
    if {[string is true -strict [::rivet::inspect ImportRivetNS]]} {
        uplevel #0 { namespace import ::rivet::* }
    }
    #unset -nocomplain ::module_conf
}

###
## This routine gets called each time a new request comes in.
## It sets up the request namespace and creates a global command
## to replace the default global.  This ensures that when a user
## uses global variables, they're actually contained within the
## namespace.  So, everything gets deleted when the request is finished.
###
proc ::Rivet::initialize_request {} {
    catch { namespace delete ::request }

    namespace eval ::request {}

    proc ::request::global {args} {
        foreach arg $args {
            uplevel "::global ::request::$arg"
        }
    }
}

## ::Rivet::handle_error
##
## If an ErrorScript has been specified, this routine will not be called.

proc ::Rivet::handle_error {} {
    puts "<pre>$::errorInfo<hr/><p>OUTPUT BUFFER:</p>$::Rivet::script</pre>"
}

## ::Rivet::request_handling
##
## Process the actual request. This is the main handler for each request.
## This collects all of the necessary BeforeScripts, AfterScripts, and
## other bits and calls them in order.

proc ::Rivet::request_handling {} {
    ::try {
        uplevel #0 ::Rivet::initialize_request
    } on error {err} {
        ::rivet::apache_log_error crit \
            "Rivet request initialization failed: $::errorInfo"
    }

    ::try {
        set script [::rivet::inspect BeforeScript]
        if {$script ne ""} {
            set ::Rivet::script $script
            uplevel #0 $script
        }

        set script [::rivet::url_script]
        if {$script ne ""} {
            set ::Rivet::script $script
            namespace eval ::request $script
        }

        set script [::rivet::inspect AfterScript]
        if {$script ne ""} {
            set ::Rivet::script $script
            uplevel #0 $script
        }
    } trap {RIVET ABORTPAGE} {err opts} {
        ::Rivet::finish_request $script $err $opts AbortScript
    } trap {RIVET THREAD_EXIT} {err opts} {
        ::Rivet::finish_request $script $err $opts AbortScript
    } on error {err opts} {
        ::Rivet::finish_request $script $err $opts
    } finally {
        ::Rivet::finish_request $script "" "" AfterEveryScript
    }

}

## ::Rivet::finish_request
##
## Finish processing the request by checking our error state and executing
## whichever script we need to close things up. If this script results in
## an error, we'll try to call ErrorScript before bailing.

proc ::Rivet::finish_request {script errorCode errorOpts {scriptName ""}} {
    set ::Rivet::errorCode $errorCode
    set ::Rivet::errorOpts $errorOpts

    if {$scriptName ne ""} {
        set scriptBody [::rivet::inspect $scriptName]
        ::try {
            uplevel #0 $scriptBody
        } on ok {} {
            return
        } on error {} {
            ::rivet::apache_log_error err \
                "Rivet $scriptName failed: $::errorInfo"
            print_error_message "Rivet $scriptName failed"
        }
    }

    set error_script [::rivet::inspect ErrorScript]
    if {$error_script eq ""} {
        set ::errorOutbuf $script ; ## legacy variable
        set error_script ::Rivet::handle_error
    }

    ::try {
        set ::Rivet::script $script
        uplevel #0 $error_script
    } on error {err} {
        ::rivet::apache_log_error err "Rivet ErrorScript failed: $::errorInfo"
        print_error_message "Rivet ErrorScript failed"
    }
}

## ::Rivet::print_error_message
##
## This message should be transparently equivalent to the
## Rivet_PrintErrorMessage function in mod_rivet_generator.c

proc ::Rivet::print_error_message {error_header} {
    puts "<strong>$error_header</strong><br/><pre>$::errorInfo</pre>"
}

## ::Rivet::tcl_commands_export_list
##
## this is temporary hack to export names of Tcl commands in rivet-tcl/.
## This function will be removed in future versions of Rivet and it's
## meant to provide a basic way to guarantee compatibility with older
## versions of Rivet (see code in ::Rivet::init)

proc ::Rivet::tcl_commands_export_list {tclpath} {
    # we collect the commands in rivet-tcl by reading the tclIndex
    # file and then we extract the command list from auto_index
    namespace eval ::Rivet::temp {}
    set ::Rivet::temp::tclpath $tclpath

    namespace eval ::Rivet::temp {
        variable auto_index
        array set auto_index {}

        # The auto_index in ${tclpath}/tclIndex is loaded.
        # This array is used to fetch a list of Rivet commands
        # implemented in Rivet

        set dir $tclpath
        source [file join $tclpath tclIndex]

        # Rivet Tcl commands not meant to go onto the export list must
        # be unset from auto_index here

        unset auto_index(::rivet::catch)
        unset auto_index(::rivet::try)
    }

    set commands [namespace eval ::Rivet::temp {array names auto_index}]

    # we won't leave anything behind
    namespace delete ::Rivet::temp

    return $commands
}

::Rivet::init

package provide Rivet 3.2
