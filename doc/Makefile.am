# Makefile for Rivet documentation

# Copyright 2004-2005 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $Id$

.PHONY:	clean 

MANDIRS = $(srcdir)/html/ $(srcdir)/html/images

PICTURES:=$(wildcard images/*.png)
EXAMPLES_SGML:=$(wildcard examples-sgml/*.*)
EXAMPLES:=$(wildcard examples/*.*)

# English, multiple files.
$(buildir)/html/index.html: $(srcdir)/rivet.xml $(srcdir)/rivet-chunk.xsl $(srcdir)/rivet.xsl $(srcdir)/xml-2/*.xml
	xsltproc --stringparam html.stylesheet rivet.css \
	--stringparam html.ext ".html" \
	--stringparam chunker.output.encoding UTF-8  \
	--nonet -o $(builddir)/html/ $(srcdir)/rivet-chunk.xsl $(srcdir)/rivet.xml

# English, one big file.
$(buildir)/html/rivet.html: $(srcdir)/rivet.xml $(srcdir)/rivet-nochunk.xsl $(srcdir)/rivet.xsl $(srcdir)/xml-2/*.xml
	xsltproc --stringparam html.stylesheet rivet.css \
	--stringparam html.ext ".html" \
	--stringparam chunker.output.encoding UTF-8  \
	--nonet -o $(builddir)/html/rivet.html $(srcdir)/rivet-nochunk.xsl \
	$(srcdir)/rivet.xml

# This means these are not build automatically, but at least
# people aren't forced to build them either.

docs: $(buildir)/html/rivet.css $(buildir)/html/index.html picts

docs-nochunk: html/rivet.css html/rivet.html picts 

$(buildir)/html/rivet.css: rivet.css
	$(mkinstalldirs) $(builddir)/html
	$(install_sh) -C -m 644 $(srcdir)/rivet.css $(builddir)/html/

# target picts checks the directory tree exists, then lets target 'graphics' copy new
# or updated files to the target tree.

picts: graphics

graphics: $(PICTURES)
	$(mkinstalldirs) $(builddir)/html/images
	@for gfile in $?; do \
		$(install_sh) -C -m 644 $(srcdir)/$$gfile $(builddir)/html/images/ ; \
	done  
	touch graphics

examples: 
	( echo 'set source_examples { $(EXAMPLES) }; source ./convert_examples.tcl' | @TCLSH_PROG@ ; )

#  target that builds the essential directory tree that will get the html and 
#  graphic files of the manual

mandirs: $(MANDIRS)

$(MANDIRS):
	mkdir $@  

install-data-local: docs

# this target cleans everything up in the html directory

distclean:
	-rm -fr $(builddir)/html/
	-rm -f  $(builddir)/graphics
	-rm Makefile

uninstall-local:
	rm -fr $(builddir)/convert_examples.tcl

.PHONY: examples
