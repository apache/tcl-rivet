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
EXTRA_DIST = LICENSE contrib doc rivet win/nmakehlp.c src/testing.c src/TclWebcgi.c
SUBDIRS = src doc
RIVETLIB_DESTDIR = $(DESTDIR)@RIVET_TCL_TARGET@
RIVET_PACKAGES = $(RIVETLIB_DESTDIR)/packages
AM_DISTCHECK_CONFIGURE_FLAGS = --with-tcl=${TCL_BIN_DIR} --with-rivet-target-dir=${RIVETLIB_DESTDIR} --with-apxs=${APXS_PREFIX}/bin/apxs

# I expect this symbol sooner or later to be handled by configure for arch dependent builds
# Current value is the rivetlib directory
RIVET_LIBS_ARCH = 

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-data-local: install-binaries install-packages

install-binaries:
	echo 'running target install-binaries ($(RIVETLIB_DESTDIR))'
	$(mkinstalldirs) $(RIVETLIB_DESTDIR)
	$(mkinstalldirs) $(RIVETLIB_DESTDIR)/rivet-tcl
	$(install_sh) -C $(builddir)/rivet/init.tcl $(RIVETLIB_DESTDIR)
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/rivet-tcl/*) $(RIVETLIB_DESTDIR)/rivet-tcl

#cp -rv $(srcdir)/rivet/packages $(RIVETLIB_DESTDIR)/
#-(  cd $(RIVETLIB_DESTDIR); rm -f *.la; \
#    echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join $(RIVET_LIBS_ARCH) *[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

uninstall-local:
	echo 'running target uninstall-local ($(RIVETLIB_DESTDIR))'
	rm -f  $(RIVETLIB_DESTDIR)/init.tcl
	rm -fr $(RIVETLIB_DESTDIR)/rivet-tcl
	rm -f  $(RIVETLIB_DESTDIR)/librivetlib.so
	rm -f  $(RIVETLIB_DESTDIR)/librivetparser.so
	rm -fr $(RIVETLIB_DESTDIR)/packages
	rm -f  $(RIVETLIB_DESTDIR)/pkgIndex.tcl
	@if [ -z '$(wildcard $(RIVETLIB_DESTDIR)/mpm/*)' ]; then \
		rmdir $(RIVETLIB_DESTDIR)/mpm; \
	fi
	@if [ -z '$(wildcard $(RIVETLIB_DESTDIR)/*)' ]; then \
		rmdir $(RIVETLIB_DESTDIR); \
	fi

install-packages:
	echo 'running target install-packages ($(RIVETLIB_DESTDIR))'
	$(mkinstalldirs) $(RIVET_PACKAGES)/asciiglyphs
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/asciiglyphs/*) $(RIVET_PACKAGES)/asciiglyphs/
	$(mkinstalldirs) $(RIVET_PACKAGES)/calendar
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/calendar/*) $(RIVET_PACKAGES)/calendar/
	$(mkinstalldirs) $(RIVET_PACKAGES)/dio
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/dio/*) $(RIVET_PACKAGES)/dio/
	$(mkinstalldirs) $(RIVET_PACKAGES)/entities
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/entities/*) $(RIVET_PACKAGES)/entities/
	$(mkinstalldirs) $(RIVET_PACKAGES)/form
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/form/*) $(RIVET_PACKAGES)/form/
	$(mkinstalldirs) $(RIVET_PACKAGES)/formbroker
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/formbroker/*) $(RIVET_PACKAGES)/formbroker/
	$(mkinstalldirs) $(RIVET_PACKAGES)/session
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/session/*) $(RIVET_PACKAGES)/session/
	$(mkinstalldirs) $(RIVET_PACKAGES)/tclrivet
	$(install_sh) -C -m 644 $(shell /bin/ls $(srcdir)/rivet/packages/tclrivet/*) $(RIVET_PACKAGES)/tclrivet/
	-(  cd  $(RIVETLIB_DESTDIR); rm -f *.la; \
	    echo 'eval file delete [glob [file join packages * pkgIndex.tcl]] pkgIndex.tcl' | @TCLSH_PROG@ ; \
	    echo 'eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl] [file join $(RIVET_LIBS_ARCH) *[info sharedlibextension]]]' | @TCLSH_PROG@ ; )

uninstall-packages:
	echo 'running target uninstall-packages ($(RIVETLIB_DESTDIR))'
	rm -f	$(RIVETLIB_DESTDIR)/pkgIndex.tcl
	rm -fr  $(RIVETLIB_DESTDIR)/packages
	rm -fr  $(RIVETLIB_DESTDIR)/rivet-tcl
	rm -f	$(RIVETLIB_DESTDIR)/*so
	@if [ -z '$(wildcard $(RIVETLIB_DESTDIR)/*)' ]; then \
		echo 'removing directory $(RIVETLIB_DESTDIR)'
		@rmdir $(RIVETLIB_DESTDIR); \
	fi

# 
distclean-local:
	rm -fr rivet_config.h
