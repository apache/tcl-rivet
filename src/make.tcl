#!/bin/sh
# the next line restarts using tclsh \
	exec tclsh "$0" "$@"

# This file is responsible for the top-level "make" style processing.
# It uses the 'aardvark' make-like system, located in the buildscripts
# directory.

# Copyright 2002-2004 The Apache Software Foundation

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

# Source the other scripts we need.
source [file join [file dirname [info script]] buildscripts buildscripts.tcl]
namespace import ::aardvark::*

# Get the configuration options generated by ./configure.tcl
getconfigs configs.tcl

# Configurable options.
#set START_TAG {"<?tcl"}
set START_TAG {"<?"}
set END_TAG   {"?>"}

# These are build targets.
set MOD_STLIB mod_rivet.a
set MOD_SHLIB mod_rivet[info sharedlibextension]
set MOD_OBJECTS {apache_multipart_buffer.o apache_request.o rivetChannel.o rivetParser.o rivetCore.o mod_rivet.o TclWebapache.o}

set RIVETLIB_STLIB librivet.a
set RIVETLIB_SHLIB librivet[info sharedlibextension]
set RIVETLIB_OBJECTS {rivetList.o rivetCrypt.o rivetWWW.o rivetPkgInit.o}

set PARSER_SHLIB librivetparser[info sharedlibextension]
set PARSER_OBJECTS {rivetParser.o parserPkgInit.o}

#set XML_DOCS [glob [file join .. doc packages * *].xml]
#set HTML_DOCS [string map {.xml .html} $XML_DOCS]
set HTML "[file join .. doc html]/"
set XSLNOCHUNK [file join .. doc rivet-nochunk.xsl]
set XSLCHUNK [file join .. doc rivet-chunk.xsl]
set XSL [file join .. doc rivet.xsl]
set XML [file join .. doc rivet.xml]
# Existing translations.
set TRANSLATIONS {ru it}
set PKGINDEX [file join .. rivet pkgIndex.tcl]

# ------------

# "AddNode" adds a compile target

# "depends" lists the nodes on which it depends

# "sh" is a shell command to execute

# "tcl" executes some Tcl code.

AddNode apache_multipart_buffer.o {
    depends apache_multipart_buffer.c apache_multipart_buffer.h
    set COMP [lremove $COMPILE -Wconversion]
    sh {$COMP apache_multipart_buffer.c}
}

AddNode apache_request.o {
    depends apache_request.c apache_request.h
    set COMP [lremove $COMPILE -Wconversion]
    sh {$COMP apache_request.c}
}

AddNode rivetChannel.o {
    depends rivetChannel.c rivetChannel.h mod_rivet.h
    sh {$COMPILE rivetChannel.c}
}

AddNode rivetParser.o {
    depends rivetParser.c rivetParser.h mod_rivet.h
    sh {$COMPILE -DSTART_TAG=$START_TAG -DEND_TAG=$END_TAG rivetParser.c}
}

AddNode rivetCore.o {
    depends rivetCore.c rivet.h mod_rivet.h
    sh {$COMPILE rivetCore.c}
}

AddNode rivetCrypt.o {
    depends rivetCrypt.c
    sh {$COMPILE rivetCrypt.c}
}

AddNode rivetList.o {
    depends rivetList.c
    sh {$COMPILE rivetList.c}
}

AddNode rivetWWW.o {
    depends rivetWWW.c
    sh {$COMPILE rivetWWW.c}
}

AddNode rivetPkgInit.o {
    depends rivetPkgInit.c
    sh {$COMPILE rivetPkgInit.c}
}

AddNode mod_rivet.o {
    depends mod_rivet.c mod_rivet.h apache_request.h parser.h
    sh {$COMPILE -DNAMEOFEXECUTABLE="[info nameofexecutable]" mod_rivet.c}
}

AddNode TclWebapache.o {
    depends TclWebapache.c mod_rivet.h apache_request.h TclWeb.h
    sh {$COMPILE TclWebapache.c}
}

AddNode parserPkgInit.o {
    depends parserPkgInit.c rivetParser.h
    sh {$COMPILE parserPkgInit.c}
}

AddNode $PARSER_SHLIB {
    depends $PARSER_OBJECTS
    sh {$TCL_SHLIB_LD -o $PARSER_SHLIB $PARSER_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode $RIVETLIB_STLIB {
    depends $RIVETLIB_OBJECTS
    sh {$TCL_STLIB_LD $RIVETLIB_STLIB $RIVETLIB_OBJECTS}
}

AddNode $RIVETLIB_SHLIB {
    depends $RIVETLIB_OBJECTS
    sh {$TCL_SHLIB_LD -o $RIVETLIB_SHLIB $RIVETLIB_OBJECTS $TCL_LIB_SPEC $TCL_LIBS $CRYPT_LIB}
}

AddNode $MOD_STLIB {
    depends $MOD_OBJECTS
    sh {$TCL_STLIB_LD $MOD_STLIB $MOD_OBJECTS}
}

AddNode $MOD_SHLIB {
    depends $MOD_OBJECTS
    sh {$TCL_SHLIB_LD -o $MOD_SHLIB $MOD_OBJECTS $TCL_LIB_SPEC $TCL_LIBS}
}

AddNode all {
    depends module
}

AddNode module {
    depends shared
    tcl {puts "mod_rivet[info sharedlibextension] built - now run $argv0 install to complete installation."}
}

# Make a shared build.

AddNode shared {
    depends $MOD_SHLIB $RIVETLIB_SHLIB $PARSER_SHLIB
}

# Make a static build - incomplete at the moment.

AddNode static {
    depends $MOD_STLIB $RIVETLIB_STLIB
}

# Clean up source directory.

AddNode clean {
    tcl {
	foreach fl [glob -nocomplain *.o *.so *.a] {
	    file delete $fl
	}
    }
}

# FIXME - we need to do this at install time, because the file join
# here makes the package index use "/".
AddNode $PKGINDEX {
    tcl {
	set curdir [pwd]
	cd [file dirname $PKGINDEX]
	eval pkg_mkIndex -verbose [pwd] init.tcl [glob [file join packages * *.tcl]]
	cd $curdir
    }
}

#AddNode testing.o {
#    sh {$COMPILE testing.c}
#}

#AddNode libtesting.so {
#    depends {parser.o testing.o}
#    sh {$TCL_SHLIB_LD -o libtesting.so parser.o testing.o}
#}

# Install everything.

AddNode install {
    depends $MOD_SHLIB $RIVETLIB_SHLIB $PARSER_SHLIB
    tcl fileutil::install -m o+r $MOD_SHLIB $LIBEXECDIR
    tcl fileutil::install -m o+r [file join .. rivet] $PREFIX
    tcl file mkdir [file join $PREFIX rivet packages rivet]
    tcl fileutil::install -m o+r $RIVETLIB_SHLIB [file join $PREFIX rivet packages rivet]
    tcl fileutil::install -m o+r $PARSER_SHLIB [file join $PREFIX rivet packages rivet]
}

#foreach doc $HTML_DOCS {
#    set xml [string map {.html .xml} $doc]
#    AddNode $doc {
#	depends $XSLNOCHUNK $xml
#	sh xsltproc --stringparam html.stylesheet rivet.css --nonet -o $doc $XSLNOCHUNK $xml
#    }
#}

# Clean up everything for distribution.

AddNode distclean {
    depends clean
    tcl cd ..
    sh { find . -name "*~" | xargs rm -f }
    sh { find . -name ".#*" | xargs rm -f }
    sh { find . -name "\#*" | xargs rm -f }
    tcl cd src
    tcl file delete -force configs.tcl
}

# Create the HTML documentation from the XML document.

# Chunked english version of docs.
AddNode [file join $HTML index.en.html] {
    depends $XML $XSL $XSLCHUNK
    sh xsltproc --stringparam html.stylesheet rivet.css --stringparam html.ext ".en.html" --nonet -o $HTML $XSLCHUNK $XML
}

# No chunk english version.
AddNode [file join $HTML rivet.en.html] {
    depends $XML $XSL $XSLNOCHUNK
    sh xsltproc --stringparam html.stylesheet rivet.css  --nonet -o [file join $HTML rivet.en.html] $XSLNOCHUNK $XML
}

# Create targets for all translations.
foreach tr $TRANSLATIONS {
    # No chunk docs.
    AddNode [file join $HTML rivet.${tr}.html] {
	depends [string map [list .xml ".${tr}.xml"] $XML] $XSLNOCHUNK
	sh xsltproc --stringparam html.stylesheet rivet.css  --nonet -o [file join $HTML rivet.${tr}.html] $XSLNOCHUNK [string map [list .xml ".${tr}.xml"] $XML]
    }

    # Chunked docs.
    AddNode [file join $HTML index.${tr}.html] {
	depends [string map [list .xml ".${tr}.xml"] $XML] $XSLCHUNK
	sh xsltproc --stringparam html.stylesheet rivet.css  --stringparam html.ext ".${tr}.html" --nonet -o $HTML $XSLCHUNK [string map [list .xml ".${tr}.xml"] $XML]
    }
}

# Use this to create all the docs.
AddNode distdoc {
    depends [file join $HTML index.en.html] [file join $HTML rivet.en.html]
}

# Create the distribution.  This is a bit unix-specific for the
# moment, as it uses the bourne shell and unix commands.

AddNode dist {
    depends distclean distdoc $PKGINDEX
    tcl {
	set fl [open [file join .. VERSION]]
	set VERSION [string trim [read $fl]]
	close $fl
	cd [file join .. ..]
	exec tar czvf tcl-rivet-${VERSION}.tgz tcl-rivet/
    }
}

AddNode help {
    tcl {
	puts "Usage: $::argv0 target"
	puts "Targets are the following:"
    }
    tcl {
	foreach nd [lsort [Nodes]] {
	    puts "\t$nd"
	}
    }
}

Run
