# This just ties together all the other build scripts.

set auto_path "[file dirname [info script]] $auto_path"
package require aardvark

foreach script {
    cmdline.tcl
    fileutil.tcl
    helpers.tcl
} {
    source [file join [file dirname [info script]] $script]
}
