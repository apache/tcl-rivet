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
html/index.en.html: rivet.xml rivet-chunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".en.html" --nonet -o html/ rivet-chunk.xsl rivet.xml

# English, one big file.
html/rivet.en.html: rivet.xml rivet-nochunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".en.html" --nonet -o html/rivet.en.html rivet-nochunk.xsl \
	rivet.xml

# Russian, multiple files.
html/index.ru.html: rivet.ru.xml rivet-chunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".ru.html" --nonet -o html/ rivet-chunk.xsl rivet.ru.xml

# Russian, one big file.
html/rivet.ru.html: rivet.ru.xml rivet-nochunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".ru.html" --nonet -o html/rivet.ru.html rivet-nochunk.xsl \
	rivet.ru.xml

# Italian, multiple files.
html/index.it.html: rivet.it.xml rivet-chunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".it.html" --nonet -o html/ rivet-chunk.xsl rivet.it.xml

# Italian, one big file.
html/rivet.it.html: rivet.it.xml rivet-nochunk.xsl
	xsltproc --stringparam html.stylesheet rivet.css --stringparam \
	html.ext ".it.html" --nonet -o html/rivet.it.html rivet-nochunk.xsl \
	rivet.it.xml

# Is this the right way to do this, having these be dependencies of
# the all target?
all: html/index.en.html html/rivet.en.html \
	html/index.ru.html html/rivet.ru.html \
	html/index.it.html html/rivet.it.html
