#
# invoked from failtest.test
#
# performs a sorted dump of the ::failtest array built
# in the ChildInitScript procedure (source checkfail.tcl)
# 
#
foreach cmd [lsort [array names ::failtest]] {
    puts "$cmd->$::failtest($cmd)"
}
