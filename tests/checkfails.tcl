
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
check_fail inspect 
