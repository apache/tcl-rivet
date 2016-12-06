#
# -- check_inspect 
#
# tests ::rivet::inspect in its different forms
#
::rivet::apache_log_error info "preparing failtest array"

proc check_inspect { cmd_form args } {

    ::rivet::apache_log_error info "checking cmd_form $cmd_form ($args)"
    set ::failtest(inspect${cmd_form}) 0
    
    set cmdeval [list ::rivet::inspect {*}$args]
    if {[::catch $cmdeval e opts]} { 
        set ::failtest(inspect${cmd_form}) 1 
    }

}

# -- check_fail
#
# general purpose test function for commands having a single form 
#

proc check_fail {cmd args} {
    set ::failtest($cmd) 0

    set cmdeval [list $cmd {*}$args]
    if {[catch {eval $cmdeval}]} { set ::failtest($cmd) 1 }
}

array set ::failtest { }
#if {[catch {::rivet::env HTTP_HOST}]} { set ::failtest(env) 1 } 
#if {[catch {::rivet::makeurl}]} { set ::failtest(makeurl) 1 } 

check_fail apache_table names headers_in
check_fail env HTTP_HOST
check_fail makeurl
check_fail parse template.rvt
check_fail include -virtual
check_fail headers redirect http://tcl.apache.org/
check_fail load_env
check_fail load_headers
check_fail raw_post
check_fail var all
check_fail no_body
check_fail virtual_filename unkn
check_inspect 0 
check_inspect 1 ChildInitScript
check_inspect 2 -all
check_inspect 3 server
check_inspect 4 script

::rivet::apache_log_error info [array get ::failtest]
