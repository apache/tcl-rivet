# This just ties together all the other build scripts.

set auto_path "[file dirname [info script]] $auto_path"
package require aardvark

foreach script {
    helpers.tcl
    parsetclConfig.tcl
    findapxs.tcl
} {
    source [file join [file dirname [info script]] $script]
}
