<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
  exclude-result-prefixes="doc"
  version='1.0'>

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
