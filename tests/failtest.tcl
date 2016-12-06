#
# invoked from failtest.test
#
# performs a sorted dump of the ::failtest array built
# in the ChildInitScript procedure (source checkfail.tcl)
# 
#
::rivet::apache_log_error info "running [info script]"
foreach cmd [lsort [array names ::failtest]] {
    puts "$cmd->$::failtest($cmd)"
}
::rivet::apache_log_error info "failtest terminates"
