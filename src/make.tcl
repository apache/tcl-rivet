#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

# $Id$
#
# This file is responsible for the top-level "make" style processing.

foreach script {
    helpers.tcl
    graph.tcl
    aardvark.tcl
    parsetclConfig.tcl
    findapxs.tcl
} {
    source [file join [file dirname [info script]] buildscripts $script]
}

# Do we have a threaded Tcl?

if { [info exists tcl_platform(threaded)] } {
    set TCL_THREADED "-DTCL_THREADED=1"
} else {
    set TCL_THREADED "-DTCL_THREADED=0"
}

namespace import ::aardvark::*

## Add variables

##
## Set this variable to the location of your apxs script if it cannot be
## found by make.tcl
##
set APXS "apxs"

## Try to find the Apache apxs script.
set APXS [FindAPXS $APXS]

if { ![string length $APXS] } {
    puts stderr "Could not find Apache apxs script."
    append err "You need to edit 'make.tcl' to supply the location of "
    append err "Apache's apxs tool."
    puts stderr $err
    exit 1
}

set INCLUDEDIR [exec $APXS -q INCLUDEDIR]
set LIBEXECDIR [exec $APXS -q LIBEXECDIR]
set PREFIX [lindex $auto_path end]

set INC "-I$INCLUDEDIR -I$TCL_PREFIX/include"

set COMPILE "$TCL_CC $TCL_CFLAGS_DEBUG $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING $TCL_SHLIB_CFLAGS $INC  $TCL_EXTRA_CFLAGS $TCL_THREADED -c"

set MOD_STLIB mod_rivet.a
set MOD_SHLIB mod_rivet[info sharedlibextension]
set MOD_OBJECTS "apache_multipart_buffer.o apache_request.o rivetChannel.o rivetParser.o rivetCore.o mod_rivet.o TclWebapache.o"

set LIB_STLIB librivet.a
set LIB_SHLIB librivet[info sharedlibextension]
set LIB_OBJECTS "rivetList.o rivetCrypt.o rivetWWW.o rivetPkgInit.o"

set TCL_LIBS "$TCL_LIBS -lcrypt"

set XML_DOCS [glob [file join .. doc packages * *].xml]
set HTML_DOCS [string map {.xml .html} $XML_DOCS]
set HTML "[file join .. doc html]/"
set XSLNOCHUNK [file join .. doc rivet-nochunk.xsl]
set XSLCHUNK [file join .. doc rivet-chunk.xsl]
set XSL [file join .. doc rivet.xsl]
set XML [file join .. doc rivet.xml]

# ------------

# "AddNode" adds a compile target

# "depends" lists the nodes on which it depends

# "sh" is a shell command to execute

# "tcl" executes some Tcl code.

AddNode apache_multipart_buffer.o {
    depends apache_multipart_buffer.c apache_multipart_buffer.h
    set COMP [lremove $COMPILE -Wconversion]
    sh {$COMP apache_multipart_buffer.c}
}

AddNode apache_request.o {
    depends apache_request.c apache_request.h
    set COMP [lremove $COMPILE -Wconversion]
    sh {$COMP apache_request.c}
}

AddNode rivetChannel.o {
    depends rivetChannel.c rivetChannel.h mod_rivet.h
    sh {$COMPILE rivetChannel.c}
}

AddNode rivetParser.o {
    depends rivetParser.c rivetParser.h mod_rivet.h
    sh {$COMPILE rivetParser.c}
}

AddNode rivetCore.o {
    depends rivetCore.c rivet.h mod_rivet.h
    sh {$COMPILE rivetCore.c}
}

AddNode rivetCrypt.o {
    depends rivetCrypt.c
    sh {$COMPILE rivetCrypt.c}
}

AddNode rivetList.o {
    depends rivetList.c
    sh {$COMPILE rivetList.c}
}

AddNode rivetWWW.o {
    depends rivetWWW.c
    sh {$COMPILE rivetWWW.c}
}

AddNode rivetPkgInit.o {
    depends rivetPkgInit.c
    sh {$COMPILE rivetPkgInit.c}
}

AddNode mod_rivet.o {
    depends mod_rivet.c mod_rivet.h apache_request.h parser.h
    sh {$COMPILE mod_rivet.c}
}

AddNode TclWebapache.o {
    depends TclWebapache.c mod_rivet.h apache_request.h TclWeb.h
    sh {$COMPILE TclWebapache.c}
}

AddNode librivet.a {
    depends $LIB_OBJECTS
    sh {$TCL_STLIB_LD $LIB_STLIB $LIB_OBJECTS}
}

AddNode librivet.so {
    depends $LIB_OBJECTS
    sh {$TCL_SHLIB_LD -o $LIB_SHLIB $LIB_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode mod_rivet.a {
    depends $MOD_OBJECTS
    sh {$TCL_STLIB_LD $MOD_STLIB $MOD_OBJECTS}
}

AddNode mod_rivet.so {
    depends $MOD_OBJECTS
    sh {$TCL_SHLIB_LD -o $MOD_SHLIB $MOD_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode all {
    depends module
}

AddNode module {
    depends shared
}

# Make a shared build.

AddNode shared {
    depends $MOD_SHLIB $LIB_SHLIB
}

# Make a static build - incomplete at the moment.

AddNode static {
    depends $MOD_STLIB $LIB_STLIB
}

# Clean up source directory.

AddNode clean {
    sh {rm -f [glob -nocomplain *.o]}
    sh {rm -f [glob -nocomplain *.so]}
    sh {rm -f [glob -nocomplain *.a]}
}

#AddNode testing.o {
#    sh {$COMPILE testing.c}
#}

#AddNode libtesting.so {
#    depends {parser.o testing.o}
#    sh {$TCL_SHLIB_LD -o libtesting.so parser.o testing.o}
#}

# Install everything.

AddNode install {
    depends $MOD_SHLIB $LIB_SHLIB
    tcl file delete -force [file join $LIBEXECDIR rivet]
    tcl file delete -force [file join $PREFIX rivet]
    tcl file copy -force $MOD_SHLIB $LIBEXECDIR
    tcl file copy -force [file join .. rivet] $PREFIX
    tcl file copy -force $LIB_SHLIB [file join $PREFIX rivet packages rivet]
}

# Install everything when creating a deb.  We need to find a better
# way of doing this.  It would involve passing arguments on the
# command line.

set DEBPREFIX [file join [pwd] .. debian tmp]
AddNode debinstall {
    depends $MOD_SHLIB $LIB_SHLIB
    tcl {file delete -force [file join $DEBPREFIX/$LIBEXECDIR rivet]}
    tcl {file copy -force $MOD_SHLIB "$DEBPREFIX/$LIBEXECDIR"}
    tcl {file copy -force [file join .. rivet] "$DEBPREFIX/$PREFIX/lib"}
    tcl {file copy -force $LIB_SHLIB "$DEBPREFIX/[file join $PREFIX/lib rivet packages rivet]"}
}

foreach doc $HTML_DOCS {
    set xml [string map {.html .xml} $doc]
    AddNode $doc {
	depends $XSLNOCHUNK $xml
	sh xsltproc --nonet -o $doc $XSLNOCHUNK $xml
    }
}

AddNode VERSION {
    tcl {
	cd ..
	pwd
    }
    sh { ./cvsversion.tcl }
    tcl { cd src/ }
}

# Clean up everything for distribution.

AddNode distclean {
    depends clean
    tcl cd ..
    sh { find . -name "*~" | xargs rm -f}
    sh { find . -name ".#*" | xargs rm -f}
    sh { find . -name "\#*" | xargs rm -f}
    tcl cd src
}

# Create the HTML documentation from the XML document.

AddNode distdoc {
    depends $XML $XSL $HTML_DOCS
    sh xsltproc --nonet -o $HTML $XSLCHUNK $XML
}

# Create the distribution.  This is a bit unix-specific for the
# moment, as it uses the bourne shell and unix commands.

AddNode dist {
    depends {distclean distdoc VERSION}
    tcl {
	set fl [open [file join .. VERSION]]
	set VERSION [string trim [read $fl]]
	close $fl
	cd [file join .. ..]
	exec tar czvf tcl-rivet-${VERSION}.tgz tcl-rivet/
    }
}

AddNode help {
    tcl {
	puts "Usage: $::argv0 target"
	puts "Targets are the following:"
    }
    tcl {
	foreach nd [lsort [Nodes]] {
	    puts "\t$nd"
	}
    }
}

Run
