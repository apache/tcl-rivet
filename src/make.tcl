#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

# $Id$

# this file actually runs things, making use of the aardvark build
# system.

# get aardvark build system
source [ file join . buildscripts aardvark.tcl ]
namespace import ::aardvark::*

# add in variables from tclConfig.sh
source [ file join . buildscripts parsetclConfig.tcl ]

# add variables

set INC "-I /usr/include/apache-1.3/"
set STATICLIB mod_rivet.a
set SHLIB "mod_rivet[ info sharedlibextension ]"
set COMPILE "$TCL_CC $TCL_CFLAGS_DEBUG $TCL_CFLAGS_OPTIMIZE $TCL_CFLAGS_WARNING $TCL_SHLIB_CFLAGS $INC  $TCL_EXTRA_CFLAGS -c"
set OBJECTS "apache_cookie.o apache_request.o mod_rivet.o tcl_commands.o apache_multipart_buffer.o channel.o parser.o"

# ------------

# Verbose

# AddNode adds a compile target
# depends lists the nodes on which it depends
# command is the command to compile

AddNode apache_cookie.o {
    depends "apache_cookie.c apache_cookie.h"
    command {$COMPILE apache_cookie.c}
}

AddNode apache_multipart_buffer.o {
    depends "apache_multipart_buffer.c apache_multipart_buffer.h"
    command {$COMPILE apache_multipart_buffer.c}
}

AddNode apache_request.o {
    depends "apache_request.c apache_request.h"
    command {$COMPILE apache_request.c}
}

AddNode mod_rivet.o {
    depends "mod_rivet.c mod_rivet.h tcl_commands.h apache_request.h parser.h parser.h"
    command {$COMPILE mod_rivet.c}
}

AddNode tcl_commands.o {
    depends "tcl_commands.c tcl_commands.h mod_rivet.h"
    command {$COMPILE tcl_commands.c}
}

AddNode parser.o {
    depends "parser.c mod_rivet.h parser.h"
    command {$COMPILE parser.c}
}

AddNode channel.o {
    depends "channel.c mod_rivet.h channel.h"
    command {$COMPILE channel.c}
}

AddNode all {
    depends shared
}

AddNode shared {
    depends $OBJECTS
    command {$TCL_SHLIB_LD -o $SHLIB $OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode static {
    depends $OBJECTS
    command {$TCL_STLIB_LD $STATICLIB $OBJECTS}
}

AddNode clean {
    command {rm -f [glob -nocomplain *.o]}
    command {rm -f [glob -nocomplain *.so]}
    command {rm -f mod_rivet.a}
}

AddNode install {
    depends static
    command {./cvsversion.tcl}
}

Run
