#!/usr/bin/tclsh


# the modules we need and their '.c' names
array set module_assoc {
    mod_log_config	config_log_module 
    mod_mime		mime_module 
    mod_negotiation	negotiation_module 
    mod_dir		dir_module 
    mod_access		access_module 
    mod_auth		auth_module
}    
    
# get the modules that adre compiled into Apache directly, and return
# the _module name.  Check also for the existence of mod_so, which we
# need to load mod_rivet.so in the directory above...

proc getcompiledin { binname } {
    global module_assoc
    set bin [ open [list | "$binname" -l ] r ]
    set compiledin [ read $bin ]
    close $bin
    set modlist [ split $compiledin ]
    set compiledin [list]
    set mod_so_present 0
    foreach entry $modlist {
	if { [regexp {(.*)\.c$} $entry match modname] } {
	    if { $modname == "mod_so" } { set mod_so_present 1 }
	    if { [ info exists module_assoc($modname) ] } {
		lappend compiledin $module_assoc($modname)
	    }
	}
    }
    if { $mod_so_present == 0 } {
	puts stderr "We need mod_so in Apache to run these tests"
	exit 1
    }
    return $compiledin
}

# find the httpd.conf file

proc gethttpdconf { binname } {
    set bin [ open [list | "$binname" -V ] r ]
    set options [ read $bin ]
    close $bin
    regexp {SERVER_CONFIG_FILE="(.*?)"} "$options" match filename
    if { ! [ file exists $filename ] } {
	# see if we can find something by combining HTTP_ROOT + SERVER_CONFIG_FILE
	regexp {HTTPD_ROOT="(.*?)"} "$options" match httpdroot
	set completename "$httpdroot/$filename"
	if { ! [ file exists $completename ] } {
	    puts stderr "neither '$filename' or '$completename' exists"
	    exit 1
	} 
	return $completename
    }
    return $filename
}

# if we need to load some modules, find out how to do it from the
# 'real' conf file, with this proc

proc getloadmodules { conffile needtoget } {
    set fl [ open $conffile r ]
    set confdata [ read $fl ]
    close $fl
    set loadline [list]
    foreach mod $needtoget {
	if { ! [ regexp -line "^.*?(LoadModule\\s+$mod\\s+.+)\$" $confdata match line ] } {
	    puts stderr "No LoadModule line for $mod!"
	    exit 1
	} else {
	    lappend loadline $line
	}
    }
    return [join $loadline "\n"]
}

# compare what's compiled in with what we need

proc determinemodules { binname } {
    global module_assoc
    set compiledin [lsort [ getcompiledin $binname ]]
    set conffile [ gethttpdconf $binname ]

    foreach {n k} [ array get module_assoc ] {
	lappend needed $k
    }
    set needed [lsort $needed]

    set needtoget [list]
    foreach mod $needed {
	if { [ lsearch $compiledin $mod ] == -1 } {
	    lappend needtoget $mod
	}
    }
    if { $needtoget == "" } {
	return ""
    } else {
	return [ getloadmodules $conffile $needtoget ]
    }
}

# dump out a config

proc makeconf { binname outfile } {
    set CWD [ pwd ]

    # replace with determinemodules
    set LOADMODULES [ determinemodules $binname ]
    
    set fl [ open "template.conf.tcl" r ]
    set template [ read $fl ]
    close $fl

    set out [ subst $template ]
    
    set of [ open $outfile w ]
    puts $of "$out"
    close $of
}
#makeconf [ getbinname ]
#puts [ determinemodules $binname ]
#gethttpdconf $binname
#getcompiledin $binname