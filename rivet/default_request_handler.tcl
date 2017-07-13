# -- request_handler.tcl
#
# code for the default handler of HTTP requests
#
#


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


