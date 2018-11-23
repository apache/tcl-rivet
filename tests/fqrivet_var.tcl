switch [::rivet::var_qs get t1] {

    1 {
        set qsvariables   [dict create {*}[::rivet::var_qs all]]
        set postvariables [dict create {*}[::rivet::var_post all]]

        set qsvar   {qsarg1 qsarg2}
        set postvar {postarg1 postarg2}

        set qs ""
        set post ""
        foreach v $qsvar {lappend qs $v [dict get $qsvariables $v]}
        foreach v $postvar {lappend post $v [dict get $postvariables $v]}
        puts -nonewline "var_qs = $qs\nvar_post = $post"
    }
    2 {
        #::rivet::parray server
        # GET request: no var_post variables are supposed to be returned 

        set qsvariables   [dict create {*}[::rivet::var_qs all]]
        set postvariables [dict create {*}[::rivet::var_post all]]

        if {[dict exists $postvariables qsarg1] || [dict exists $postvariables qsarg2]} { 
            puts "KO: [::rivet::var_post all]" 
        } else {
            puts -nonewline "OK"
        }

    }
    3 {
        set qsvariables   [dict create {*}[::rivet::var_qs all]]
        set postvariables [dict create {*}[::rivet::var_post all]]

        if {[dict exists $qsvariables postarg1] || [dict exists $qsvariables postarg2]} { 
            puts "KO: $qsvariables" 
        } else {
            puts -nonewline "OK"
        }
    }
}
