foreach cmd {env makeurl} {
    puts "$cmd->$::failtest($cmd)"
}
