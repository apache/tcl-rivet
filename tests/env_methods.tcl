
    set envvar [::rivet::var_qs get envvar]
    set v1 [::rivet::env $envvar]
    
    ::rivet::load_env loadenv
    set v2 $loadenv($envvar)
    set comp [string match $v1 $v2]
    if {$comp} {
        set msg OK
    } else {
        set msg "env: $v1, load_env: $v2"
    }

    puts -nonewline "$envvar: $msg"

