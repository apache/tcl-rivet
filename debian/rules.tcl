#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh8.4 "$0" "$@"

# Build .deb with aardvark build system.

# This is shelved for now.  The officially sanctioned way of building
# a deb is by calling a bunch of crufty, hard-to-read perl programs.

# $Id$

source [file join [file dirname [info script]] debian.tcl]

AddNode clean {
    tcl cd src
    tcl {puts [pwd]}
    sh ./make.tcl clean
    tcl cd ..
    tcl file delete debian/substvars
}

AddNode build {
    tcl set env(C_INCLUDE_PATH) /usr/include/tcl$tcl_version
    tcl cd src
    sh ./make.tcl shared
    tcl cd ..
}

AddNode binary-arch {
    depends build binary-indep
    tcl file delete -force debian/tmp

    tcl file mkdir debian/tmp/usr/lib/apache/1.3/ debian/tmp/usr/lib/rivet/
    tcl file copy debian/500mod_rivet.info debian/tmp/usr/lib/apache/1.3/

    tcl file mkdir debian/tmp/usr/share/doc/rivet/
    tcl file copy debian/copyright debian/tmp/usr/share/doc/rivet/
    tcl file copy doc/html debian/tmp/usr/share/doc/rivet/

    tcl file copy ChangeLog debian/tmp/usr/share/doc/rivet/
    tcl file copy debian/changelog debian/tmp/usr/share/doc/rivet/changelog.Debian
    sh strip src/librivet.so src/mod_rivet.so
    tcl DocClean rivet
    tcl DocCompress rivet
    tcl FixDocPerms
    tcl InstallDeb
    tcl ShlibDeps src/mod_rivet.so
    tcl GetVersion
}

AddNode binary-indep {
    tcl cd src
    #sh ./make.tcl distdoc
    tcl cd ..
}

AddNode binary {
    depends binary-arch
}

AddNode all {
    depends binary
}

Run