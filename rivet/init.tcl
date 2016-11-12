# Copyright 2002-2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package provide Rivet 3.0

namespace eval ::Rivet {

    ###
    ## -- tcl_commands_export_list
    #
    ## this is temporary hack to export names of Tcl commands in rivet-tcl/.
    ## This function will be removed in future versions of Rivet and it's
    ## meant to provide a basic way to guarantee compatibility with older
    ## versions of Rivet (see code in ::Rivet::init)
    ##

    proc tcl_commands_export_list {tclpath} {

        # we collect the commands in rivet-tcl by reading the tclIndex
        # file and then we extract the command list from auto_index

        namespace eval ::rivet_temp { }
        set ::rivet_temp::tclpath $tclpath

        namespace eval ::rivet_temp {
            variable auto_index
            array set auto_index {}

            # the auto_index in ${tclpath}/tclIndex is loaded
            # this array is used to fetch a list of Rivet commands
            # implemented in Rivet

            set dir $tclpath
            source [file join $tclpath tclIndex]

            # Rivet Tcl commands not meant to go onto the export list must
            # be unset from auto_index here

            unset auto_index(::rivet::catch)
            unset auto_index(::rivet::try)
        }

        set command_list [namespace eval ::rivet_temp {array names auto_index}]

        # we won't leave anything behind
        namespace delete ::rivet_temp

        return $command_list
    }

    ###
    ## This routine gets called each time a new request comes in.
    ## It sets up the request namespace and creates a global command
    ## to replace the default global.  This ensures that when a user
    ## uses global variables, they're actually contained within the
    ## namespace.  So, everything gets deleted when the request is finished.
    ###
    proc initialize_request {} {
        catch { namespace delete ::request }

        namespace eval ::request { }

        proc ::request::global {args} {
            foreach arg $args {
                uplevel "::global ::request::$arg"
            }
        }
    }

    ###
    ## The default error handler for Rivet.  Any time a page runs into an
    ## error, this routine will be called to handle the error information.
    ## If an ErrorScript has been specified, this routine will not be called.
    ###
    proc handle_error {} {
        global errorInfo
        global errorOutbuf

        puts <PRE>
        puts "<HR>$errorInfo<HR>"
        puts "<P><B>OUTPUT BUFFER:</B></P>"
        puts $errorOutbuf
        puts </PRE>
    }

    ###
    ## This routine gets called each time a request is finished.  Any kind
    ## of special cleanup can be placed here.
    ###
    proc cleanup_request {} { }


    ######## mod_rivet_ng specific ++++++++

    ### 
    # -- error_message
    #
    # this message should be transparently equivalent
    # to the Rivet_PrintErrorMessage function in mod_rivet_generator.c
    #

    proc print_error_message {error_header} {
        global errorInfo

        puts "$error_header <br/>"
        puts "<pre> $errorInfo </pre>"

    }


    ###
    ## -- error_handler
    ##
    ###

    proc error_handler {script err_code err_options} {
        global errorOutbuf

        set error_script [::rivet::inspect ErrorScript]
        if {$error_script != ""} {
            if {[catch {namespace eval :: $error_script} err_code err_info]} {
                ::rivet::apache_log_err err "error script failed ($errorInfo)"
                print_error_message "<b>Rivet ErrorScript failed</b>"
            }
        } else {
            set errorOutbuf $script
            ::Rivet::handle_error
        }
    }

    ###
    ## -- url_handler
    ##
    ### 

    proc url_handler {} {

        set script [join [list  [::rivet::inspect BeforeScript]     \
                                [::rivet::url_script]               \
                                [::rivet::inspect AfterScript]] "\n"]

        #set fp [open "/tmp/script-[pid].tcl" w+]
        #puts $fp $script
        #close $fp

        return $script

    }

    ###
    ## -- Default request processing
    ##
    ## a request will handled by this procedure

    proc request_handling {} {
     
        set script [::Rivet::url_handler]

        ::try {

            namespace eval :: $script

        } trap {RIVET ABORTPAGE} {::Rivet::error_code ::Rivet::error_options} {

            set abort_script [::rivet::inspect AbortScript]
            if {[catch {namespace eval :: $abort_script} ::Rivet::error_code ::Rivet::error_options]} {
                ::rivet::apache_log_err err "abort script failed ($errorInfo)"
                print_error_message "<b>Rivet AbortScript failed</b>"

                ::Rivet::error_handler $abort_script $::Rivet::error_code $::Rivet::error_options
            }


        } trap {RIVET THREAD_EXIT} {::Rivet::error_code ::Rivet::error_options} {

            set abort_script [::rivet::inspect AbortScript]
            if {[catch {namespace eval :: $abort_script} ::Rivet::error_code ::Rivet::error_options]} {
                ::rivet::apache_log_err err "abort script failed ($errorInfo)"
                print_error_message "<b>Rivet AbortScript failed</b>"

                ::Rivet::error_handler $abort_script $::Rivet::error_code $::Rivet::error_options
            }

            #<sudden-exit-script>

        } on error {::Rivet::error_code ::Rivet::error_options} {

            ::Rivet::error_handler $script $::Rivet::error_code $::Rivet::error_options

        } finally {

            set after_every_script [::rivet::inspect AfterEveryScript]
            if {[catch {namespace eval :: $after_every_script} ::Rivet::error_code ::Rivet::error_options]} {
                ::rivet::apache_log_err err "AfterEveryScript failed ($errorInfo)"
                print_error_message "<b>Rivet AfterEveryScript failed</b>"

                ::Rivet::error_handler $after_every_script $::Rivet::error_code $::Rivet::error_options
            }
            #<after-every-script>
        }
 
        namespace eval :: ::Rivet::cleanup_request  
    }

    ######## mod_rivet_ng specific ---------

    ###
    ## The main initialization procedure for Rivet.
    ###

    proc init {} {
        global auto_path
        global server

        ## Add the rivet-tcl directory to Tcl's auto search path.
        ## We insert it at the head of the list because we want any of
        ## our procs named the same as Tcl's procs to be overridden.
        ## Example: parray
        set tclpath [file join [file dirname [info script]] rivet-tcl]
        set auto_path [linsert $auto_path 0 $tclpath]

        ## As we moved the commands set to ::rivet namespace we
        ## we want to guarantee the commands are still accessible
        ## at global level by putting them on the export list.
        ## Importing the ::rivet namespace is deprecated and we should
        ## make it clear in the manual

        ## we keep in ::rivet::export_list a list of importable commands

        namespace eval ::rivet [list set cmd_export_list [tcl_commands_export_list $tclpath]]
        namespace eval ::rivet {

        ## init.tcl is run by mod_rivet (which creates the ::rivet namespace) but it gets run
        ## standalone by mkPkgindex during the installation phase. We have to make sure the
        ## procedure won't fail in this case, so we check for the existence of the variable.

            if {[info exists module_conf(export_namespace_commands)] && \
                 $module_conf(export_namespace_commands)} {

                # commands in 'command_list' are prefixed with ::rivet, so we have to
                # remove it to build an export list 
                
                set export_list {}
                foreach c $cmd_export_list {
                    lappend export_list [namespace tail $c]
                }

                #puts stderr "exporting $export_list"
                eval namespace export $export_list

            }
        }
        ## Add the packages directory to the auto_path.
        ## If we have a packages$tcl_version directory
        ## (IE: packages8.3, packages8.4) append that as well.

        ## The packages directory come right after the rivet-tcl directory.
        set pkgpath [file join [file dirname [info script]] packages]
        set auto_path [linsert $auto_path 1 $pkgpath]
        set auto_path [linsert $auto_path 2 ${pkgpath}-local]

        if { [file exists ${pkgpath}$::tcl_version] } {
            lappend auto_path ${pkgpath}$::tcl_version
        }

        ## Likewise we have also to add to auto_path the directory containing 
        ## this script since it holds the pkgIndex.tcl file for package Rivet. 

        set auto_path [linsert $auto_path 0 [file dirname [info script]]]

        ## This will allow users to create proc libraries and tclIndex files
        ## in the local directory that can be autoloaded.
        ## Perhaps this must go to the front of the list to allow the user
        ## to override even Rivet's procs.
        lappend auto_path .
    }

} ;## namespace eval ::Rivet

## eventually we have to divert Tcl ::exit to ::rivet::exit

rename ::exit ::Rivet::tclcore_exit
proc ::exit {code} {

    if {[string is integer $code]} {
        eval ::rivet::exit $code
    } else {
        eval ::rivet::exit 0
    }

}

## Rivet 2.1.x supports Tcl >= 8.5, therefore there's no more need for
## the command incr0, as the functionality of creating a not yet
## existing variable is now provided by 'incr'. Being incr0 a command
## in Rivet < 2.1.0, before the move into the ::Rivet namespace, 
## we alias this command only in the global namespace

interp alias {} ::incr0 {} incr

## Initialize Rivet.
::Rivet::init

## And now we get to the import of the whole ::rivet namespace. 

# Do we actually want to import everything? If Rivet was configured
# to import the ::rivet namespace for compatibility we do it right away.
# This option is not guaranteed to be supported in future versions.

if {[info exists module_conf(import_rivet_commands)] && $module_conf(import_rivet_commands)} {

    namespace eval :: { namespace import ::rivet::* }

}

array unset module_conf

