# -- commands.tcl
#
# testing the output of various commands that 
# provide a swiss knife for formatting, generating, etc.etc.
#


if {[::rivet::var exists cmd]} {

    set cmd [::rivet::var get cmd]
    switch $cmd {
        xml {

            # generic ::rivet::xml usage

            puts [::rivet::xml "a text string" a [list b a1 v1 a2 v2] [list c a1 v1 a2 v2]]
            puts [::rivet::xml "a text string" [list b a1 v1 a2 v2] [list c a1 v1 a2 v2] a]

            puts [::rivet::xml "" [list b a1 v1 a2 v2] [list c a1 v1 a2 v2] a]

            # self closing single element

            puts [::rivet::xml [list a a1 v1 a2 v2]]
            puts -nonewline [::rivet::xml p]
        }
        default {
            puts "invalid argument '$cmd'"
        }
    }

} else {

    puts "no cmd argument provided" 

}
