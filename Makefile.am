#
# top-level Makefile.am for Apache Rivet
#
# ...gets turned into a Makefile.in by automake
#
# $Id$
#
#foreign -- need to set this somehow

EXTRA_DIST=LICENSE
SUBDIRS = src doc

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local:
	cp -r rivet $(DESTDIR)/@TCL_PACKAGE_PATH@
# This little bit of line noise generates the package index file (pkgIndex.tcl)
	( cd $(DESTDIR)/@TCL_PACKAGE_PATH@/rivet ; echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join *[info sharedlibextension]]]' | @TCLSH_PROG@ ; )