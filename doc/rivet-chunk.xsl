<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
  exclude-result-prefixes="doc"
  version='1.0'>

<!--
   Copyright 2002-2004 The Apache Software Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   	http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-->

  <!--
  <xsl:output method="html"
  encoding="ISO-8859-1"
  indent="no"/>
  -->

  <!-- ********************************************************************
  $Id$
  ********************************************************************

  This file is part of the XSL DocBook Stylesheet distribution.
  See ../README or http://nwalsh.com/docbook/xsl/ for copyright
  and other information.

  ******************************************************************** -->

  <!-- ==================================================================== -->

  <xsl:param name="refentry.separator" select="1"/>

  <xsl:template match="section">
    <xsl:variable name="depth" select="count(ancestor::section)+1"/>

    <div class="{name(.)}">
      <xsl:call-template name="language.attribute"/>
      <xsl:call-template name="section.titlepage"/>

      <xsl:variable name="toc.params">
	<xsl:call-template name="find.path.params">
	  <xsl:with-param name="table" select="normalize-space($generate.toc)"/>
	</xsl:call-template>
      </xsl:variable>

      <xsl:if test="@role = 'reference' or (contains($toc.params, 'toc')
	and $depth &lt;= $generate.section.toc.level)">
	<xsl:call-template name="section.toc">
	  <xsl:with-param name="toc.title.p" select="contains($toc.params, 'title')"/>
	</xsl:call-template>
	<xsl:call-template name="section.toc.separator"/>
      </xsl:if>
      <xsl:apply-templates/>
      <xsl:call-template name="process.chunk.footnotes"/>
    </div>
  </xsl:template>

<!--  <xsl:include -->
<!--  href="/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/html/manifest.xsl"/> -->

  <xsl:include href="/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/html/chunk.xsl"/>

  <xsl:include href="norefchunk.xsl"/>

  <xsl:include href="rivet.xsl"/>
  <xsl:include href="refentry.xsl"/>

</xsl:stylesheet>
