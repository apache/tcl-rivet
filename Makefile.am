#
# top-level Makefile.am for Apache Rivet: gets turned into a Makefile.in by automake
#
# $Id$
#
# 2007/12/25: Added target uninistall-local that removes the tcl stuff (mxmanghi)
# 2010/06/22: target instal-data-local searches for pkgIndex.tcl files and deletes them
#             before invoking pkg_mkIndex.
# 2013/01/20: removing libtool created .la files

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = LICENSE contrib debian doc rivet win/nmakehlp.c src/testing.c src/TclWebcgi.c
SUBDIRS = src doc
RIVETLIB_DESTDIR = $(DESTDIR)@RIVET_TCL_TARGET@
RIVET_PACKAGES = $(RIVETLIB_DESTDIR)/packages

# I expect this symbol sooner or later to be handled by configure for arch dependent builds
# Current value is the rivetlib directory
RIVET_LIBS_ARCH = 

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local: install-binaries install-packages

install-binaries:
	$(mkinstalldirs) $(RIVETLIB_DESTDIR)
	cp -v rivet/init.tcl $(RIVETLIB_DESTDIR)/
	cp -rv rivet/rivet-tcl $(RIVETLIB_DESTDIR)/
	-(  cd $(RIVETLIB_DESTDIR); rm -f *.la; \
        echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join $(RIVET_LIBS_ARCH)*[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

uninstall-local:
	rm -f  $(RIVETLIB_DESTDIR)/init.tcl
	rm -fr $(RIVETLIB_DESTDIR)/rivet-tcl
	rm -f  $(RIVETLIB_DESTDIR)/librivetlib.so
	rm -f  $(RIVETLIB_DESTDIR)/librivetparser.so
	rm -fr $(RIVETLIB_DESTDIR)/packages
	rm -f  $(RIVETLIB_DESTDIR)/pkgIndex.tcl
	@if [ -z '$(wildcard $(RIVETLIB_DESTDIR)/*)' ]; then \
		rmdir $(RIVETLIB_DESTDIR); \
	fi

install-packages:
	$(mkinstalldirs) $(RIVET_PACKAGES)
	cp -rv rivet/packages/* $(RIVET_PACKAGES)
	-(  cd  $(RIVETLIB_DESTDIR); \
	    echo 'eval file delete [glob [file join packages * pkgIndex.tcl]] pkgIndex.tcl' | @TCLSH_PROG@ ; \
	    echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join $(RIVET_LIBS_ARCH)*[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

uninstall-packages:
	rm -fr $(RIVET_PACKAGES)
	rm -f  $(RIVETLIB_DESTDIR)/pkgIndex.tcl
	@if [ -z '$(wildcard $(RIVETLIB_DESTDIR)/*)' ]; then \
		@rmdir $(RIVETLIB_DESTDIR); \
	fi
	-(  cd  $(RIVETLIB_DESTDIR); \
	    echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join $(RIVET_LIBS_ARCH)*[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

# 
distclean-local:
	rm -fr rivet_config.h
