#
# top-level Makefile.am for Apache Rivet
#
# ...gets turned into a Makefile.in by automake
#
# $Id$
#
#foreign -- need to set this somehow

EXTRA_DIST=LICENSE contrib debian doc rivet win/nmakehlp.c src/testing.c src/TclWebcgi.c
SUBDIRS = src doc

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local:
	$(mkinstalldirs) $(DESTDIR)@libdir@/rivet@VERSION@
	cp -r rivet/* $(DESTDIR)@libdir@/rivet@VERSION@
# This little bit of line noise generates the package index file (pkgIndex.tcl)
	-( cd  $(DESTDIR)@libdir@/rivet@VERSION@ ; echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join *[info sharedlibextension]]]' | @TCLSH_PROG@ ; )