foreach cmd [lsort [array names ::failtest]] {
    puts "$cmd->$::failtest($cmd)"
}
