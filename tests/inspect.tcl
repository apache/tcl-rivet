
if {[::rivet::var_qs exists p]} {
    set fpar [::rivet::var_qs get p]

    switch $fpar {
        childinit {
            puts -nonewline [::rivet::inspect ChildInitScript]
        }
        script {
            puts -nonewline [::rivet::inspect script]
        }
        default {
        }
    }
} else {
            
    puts -nonewline [::rivet::inspect]

}
