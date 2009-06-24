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


# English, multiple files.
html/index.html: rivet.xml rivet-chunk.xsl xml/*.xml
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".html" --nonet -o html/ rivet-chunk.xsl rivet.xml

# English, one big file.
html/rivet.html: rivet.xml rivet-nochunk.xsl xml/*.xml
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".html" --nonet -o html/rivet.html rivet-nochunk.xsl \
	rivet.xml

# FIXME - this means these are not build automatically, but at least
# people aren't forced to build them either.

docs: html/index.html html/rivet.html

clean:
	-rm html/*.html
