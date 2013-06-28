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

MANDIRS = html/ html/images
PICTURES:=$(wildcard images/*.png)
EXAMPLES_SGML:=$(wildcard examples-sgml/*.*)
EXAMPLES:=$(wildcard examples/*.*)

# English, multiple files.
html/index.html: rivet.xml rivet-chunk.xsl rivet.xsl xml/*.xml
	xsltproc --stringparam html.stylesheet rivet.css \
	--stringparam html.ext ".html" \
	--stringparam chunker.output.encoding UTF-8  \
	--nonet -o html/ rivet-chunk.xsl rivet.xml

# English, one big file.
html/rivet.html: rivet.xml rivet-nochunk.xsl rivet.xsl xml/*.xml
	xsltproc --stringparam html.stylesheet rivet.css \
	--stringparam html.ext ".html" \
	--stringparam chunker.output.encoding UTF-8  \
	--nonet -o html/rivet.html rivet-nochunk.xsl \
	rivet.xml

# This means these are not build automatically, but at least
# people aren't forced to build them either.

docs: picts html/rivet.css html/index.html

docs-nochunk: picts html/rivet.css html/rivet.html

html/rivet.css: mandirs rivet.css
	cp -v rivet.css html/

# target picts checks the directory tree exists, then lets target 'graphics' copy new
# or updated files to the target tree.

picts:	mandirs graphics

graphics: $(PICTURES)
	for gfile in $?; do \
	    cp -v $$gfile html/images/ ; \
	done ; \
	touch graphics

examples: 
	( echo 'set source_examples { $(EXAMPLES) }; source ./convert_examples.tcl' | @TCLSH_PROG@ ; )

#  target that builds the essential directory tree that will get the html and 
#  graphic files of the manual

mandirs: $(MANDIRS)

$(MANDIRS):
	mkdir $@  

# this target cleans everything up in the html directory

distclean:
	-rm -fr html/*.html 
	-rm -f graphics
	-rm Makefile

.PHONY: examples
