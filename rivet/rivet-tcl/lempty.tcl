proc lempty {list} {
    if {[catch {llength $list} len]} { return 0 }
    return [expr $len == 0]
}
