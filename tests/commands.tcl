# -- commands.tcl
#
# testing the output of various commands that 
# provide a swiss knife for formatting, generating, etc.etc.
#


if {[::rivet::var exists cmd]} {

    set cmd [::rivet::var get cmd]
    switch $cmd {
        xml {
            puts [::rivet::xml "" a [list b a1 v1 a2 v2] [list c a1 v1 a2 v2]]
            puts [::rivet::xml "" [list b a1 v1 a2 v2] [list c a1 v1 a2 v2] a]
            puts -nonewline [::rivet::xml "element text" a [list b a1 v1 a2 v2] [list c a1 v1 a2 v2]]
        }
        default {
            puts "invalid argument '$cmd'"
        }
    }

} else {

    puts "no cmd argument provided" 

}
