#
# top-level Makefile.am for Apache Rivet: gets turned into a Makefile.in by automake
#
# $Id$
#
# 2007/12/25: Added target uninistall-local that removes the tcl stuff (mxmanghi)
# 2010/06/22: target instal-data-local searches for pkgIndex.tcl files and deletes them
#             before invoking pkg_mkIndex.
#

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = LICENSE contrib debian doc rivet win/nmakehlp.c src/testing.c src/TclWebcgi.c
SUBDIRS = src doc
RIVETLIB_DESTDIR = @RIVET_TCL_TARGET@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(RIVETLIB_DESTDIR)
	cp -r rivet/* $(DESTDIR)$(RIVETLIB_DESTDIR)
	-(  cd  $(DESTDIR)$(RIVETLIB_DESTDIR); \
	    echo 'eval file delete [glob [file join packages * pkgIndex.tcl]] pkgIndex.tcl' | @TCLSH_PROG@ ; \
	    echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join *[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

uninstall-local:
	rm -fr $(DESTDIR)$(RIVETLIB_DESTDIR)

# 
