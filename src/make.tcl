#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

# $Id$

# this file actually runs things, making use of the aardvark build
# system.

set scripts {
    helpers.tcl
    graph.tcl
    aardvark.tcl
    parsetclConfig.tcl
    findapxs.tcl
}

foreach script $scripts { source [file join . buildscripts $script] }

namespace import ::aardvark::*

## Add variables

##
## Set this variable to the location of your apxs script if it cannot be
## found by make.tcl
##
set APXS "apxs"

## Try to find the Apache apxs script.
set APXS [FindAPXS $APXS]

if {![string length $APXS]} {
    puts stderr "Could not find Apache apxs script."
    append err "You need to edit 'make.tcl' to supply the location of "
    append err "Apache's apxs tool."
    puts stderr $err
    exit 1
}

set INCLUDEDIR [exec $APXS -q INCLUDEDIR]
set LIBEXECDIR [exec $APXS -q LIBEXECDIR]
set PREFIX [exec $APXS -q PREFIX]

set INC "-I$INCLUDEDIR -I$TCL_PREFIX/include"

set COMPILE "$TCL_CC $TCL_CFLAGS_DEBUG $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING $TCL_SHLIB_CFLAGS $INC  $TCL_EXTRA_CFLAGS -c"

set MOD_STLIB mod_rivet.a
set MOD_SHLIB mod_rivet[info sharedlibextension]
set MOD_OBJECTS "apache_multipart_buffer.o apache_request.o rivetChannel.o rivetParser.o rivetCore.o mod_rivet.o TclWebapache.o"

set LIB_STLIB librivet.a
set LIB_SHLIB librivet[info sharedlibextension]
set LIB_OBJECTS "rivetList.o rivetCrypt.o rivetWWW.o rivetPkgInit.o"

set TCL_LIBS "$TCL_LIBS -lcrypt"

# ------------

# Verbose

# AddNode adds a compile target
# depends lists the nodes on which it depends
# command is the command to compile

AddNode apache_multipart_buffer.o {
    depends "apache_multipart_buffer.c apache_multipart_buffer.h"
    set COMP [lremove $COMPILE -Wconversion]
    command {$COMP apache_multipart_buffer.c}
}

AddNode apache_request.o {
    depends "apache_request.c apache_request.h"
    set COMP [lremove $COMPILE -Wconversion]
    command {$COMP apache_request.c}
}

AddNode rivetChannel.o {
    depends "rivetChannel.c rivetChannel.h mod_rivet.h"
    command {$COMPILE rivetChannel.c}
}

AddNode rivetParser.o {
    depends "rivetParser.c rivetParser.h mod_rivet.h"
    command {$COMPILE rivetParser.c}
}

AddNode rivetCore.o {
    depends "rivetCore.c rivet.h mod_rivet.h"
    command {$COMPILE rivetCore.c}
}

AddNode rivetCrypt.o {
    depends "rivetCrypt.c"
    command {$COMPILE rivetCrypt.c}
}

AddNode rivetList.o {
    depends "rivetList.c"
    command {$COMPILE rivetList.c}
}

AddNode rivetWWW.o {
    depends "rivetWWW.c"
    command {$COMPILE rivetWWW.c}
}

AddNode rivetPkgInit.o {
    depends "rivetPkgInit.c"
    command {$COMPILE rivetPkgInit.c}
}

AddNode mod_rivet.o {
    depends "mod_rivet.c mod_rivet.h apache_request.h parser.h"
    command {$COMPILE mod_rivet.c}
}

AddNode TclWebapache.o {
    depends "TclWebapache.c mod_rivet.h apache_request.h"
    command {$COMPILE TclWebapache.c}
}

AddNode librivet.a {
    depends $LIB_OBJECTS
    command {$TCL_STLIB_LD $LIB_STLIB $LIB_OBJECTS}
}

AddNode librivet.so {
    depends $LIB_OBJECTS
    command {$TCL_SHLIB_LD -o $LIB_SHLIB $LIB_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode mod_rivet.a {
    depends $MOD_OBJECTS
    command {$TCL_STLIB_LD $MOD_STLIB $MOD_OBJECTS}
}

AddNode mod_rivet.so {
    depends $MOD_OBJECTS
    command {$TCL_SHLIB_LD -o $MOD_SHLIB $MOD_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode all {
    depends module
}

AddNode module {
    depends "shared"
}

AddNode shared {
    depends "$MOD_SHLIB $LIB_SHLIB"
}

AddNode static {
    depends "$MOD_STLIB $LIB_STLIB"
}

AddNode clean {
    command {rm -f [glob -nocomplain *.o]}
    command {rm -f [glob -nocomplain *.so]}
    command {rm -f [glob -nocomplain *.a]}
}

AddNode testing.o {
    command {$COMPILE testing.c}
}

AddNode libtesting.so {
    depends {parser.o testing.o}
    command {$TCL_SHLIB_LD -o libtesting.so parser.o testing.o}
}

AddNode install {
    depends "$MOD_SHLIB $LIB_SHLIB"
    tclcommand "file copy -force $MOD_SHLIB $LIBEXECDIR"
    tclcommand "file copy -force [file join .. rivet] $PREFIX"
    tclcommand "file copy -force $LIB_SHLIB [file join $PREFIX rivet packages rivet]"
}

Run
