proc rivet_command_document {list} {
    array set info $list

    puts "<HEAD>"
    puts "<TITLE>$info(name) Documentation</TITLE>"
    puts "</HEAD>"
    puts "<BODY BGCOLOR=WHITE VLINK=blue>"

    puts "<CENTER>"
    puts {<FONT SIZE="+3">}
    puts "$info(name) - $info(package)"
    puts "</FONT>"
    puts "<BR>"
    puts "<B>"
    puts {<A HREF="#synopsis">Synopsis</A>}
    puts " * "
    puts {<A HREF="#description">Description</A>}
    if {[info exists info(seealso)]} {
	puts " * "
	puts {<A HREF="#seealso">See Also</A>}
    }
    puts "</B>"
    puts "</CENTER>"

    puts {<H3><A NAME="name" HREF="#name">Name</A></H3>}
    puts "<B>$info(name) - $info(short)</B>"

    if {![info exists info(command)]} { set info(command) $info(name) }

    puts {<H3><A NAME="synopsis" HREF="#synopsis">Synopsis</A></H3>}
    puts "$info(command)"
    if {[info exists info(arguments)]} { puts "<I>$info(arguments)</I>" }

    puts {<H3><A NAME="description" HREF="#description">Description</A></H3>}
    puts $info(description)

    if {[info exists info(seealso)]} {
	puts {<H3><A NAME="seealso" HREF="#seealso">See Also</A></H3>}
	puts $info(seealso)
    }
}
