<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exsl="http://exslt.org/common"
  version="1.0"
  exclude-result-prefixes="exsl">

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

  <!-- Why is chunk-code now xsl:included?

  Suppose you want to customize *both* the chunking algorithm used *and* the
  presentation of some elements that may be chunks. In order to do that, you
  must get the order of imports "just right". The answer is to make your own
  copy of this file, where you replace the initial import of "docbook.xsl"
  with an import of your own base.xsl (that does its own import of docbook.xsl).

  Put the templates for changing the presentation of elements in your base.xsl.

  Put the templates that control chunking after the include of chunk-code.xsl.

  Voila! (Man I hope we can do this better in XSLT 2.0)

  -->

  <xsl:template name="chunk">
    <xsl:param name="node" select="."/>
    <!-- returns 1 if $node is a chunk -->

    <!-- ==================================================================== -->
    <!-- What's a chunk?

    The root element
    appendix
    article
    bibliography  in article or book
    book
    chapter
    colophon
    glossary      in article or book
    index         in article or book
    part
    preface
    reference
    sect{1,2,3,4,5}  if position()>1 && depth < chunk.section.depth
    section          if position()>1 && depth < chunk.section.depth
    set
    setindex
    -->
    <!-- ==================================================================== -->

    <!--
    <xsl:message>
    <xsl:text>chunk: </xsl:text>
    <xsl:value-of select="name($node)"/>
    <xsl:text>(</xsl:text>
    <xsl:value-of select="$node/@id"/>
    <xsl:text>)</xsl:text>
    <xsl:text> csd: </xsl:text>
    <xsl:value-of select="$chunk.section.depth"/>
    <xsl:text> cfs: </xsl:text>
    <xsl:value-of select="$chunk.first.sections"/>
    <xsl:text> ps: </xsl:text>
    <xsl:value-of select="count($node/parent::section)"/>
    <xsl:text> prs: </xsl:text>
    <xsl:value-of select="count($node/preceding-sibling::section)"/>
  </xsl:message>
    -->

    <xsl:choose>
      <xsl:when test="not($node/parent::*)">1</xsl:when>

      <xsl:when test="local-name($node) = 'sect1'
	and $chunk.section.depth &gt;= 1
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::sect1) &gt; 0)">
	<xsl:text>1</xsl:text>
      </xsl:when>
      <xsl:when test="local-name($node) = 'sect2'
	and $chunk.section.depth &gt;= 2
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::sect2) &gt; 0)">
	<xsl:call-template name="chunk">
	  <xsl:with-param name="node" select="$node/parent::*"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:when test="local-name($node) = 'sect3'
	and $chunk.section.depth &gt;= 3
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::sect3) &gt; 0)">
	<xsl:call-template name="chunk">
	  <xsl:with-param name="node" select="$node/parent::*"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:when test="local-name($node) = 'sect4'
	and $chunk.section.depth &gt;= 4
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::sect4) &gt; 0)">
	<xsl:call-template name="chunk">
	  <xsl:with-param name="node" select="$node/parent::*"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:when test="local-name($node) = 'sect5'
	and $chunk.section.depth &gt;= 5
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::sect5) &gt; 0)">
	<xsl:call-template name="chunk">
	  <xsl:with-param name="node" select="$node/parent::*"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:when test="local-name($node) = 'section'
	and $chunk.section.depth &gt;= count($node/ancestor::section)+1
	and ($chunk.first.sections != 0
	or count($node/preceding-sibling::section) &gt; 0)">
	<xsl:call-template name="chunk">
	  <xsl:with-param name="node" select="$node/parent::*"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:when test="name($node)='preface'">1</xsl:when>
      <xsl:when test="name($node)='chapter'">1</xsl:when>
      <xsl:when test="name($node)='appendix'">1</xsl:when>
      <xsl:when test="name($node)='article'">1</xsl:when>
      <xsl:when test="name($node)='part'">1</xsl:when>
      <xsl:when test="name($node)='reference'">1</xsl:when>
      <xsl:when test="name($node)='refentry'">0</xsl:when>
      <xsl:when test="name($node)='index'
	and (name($node/parent::*) = 'article'
	or name($node/parent::*) = 'book')">1</xsl:when>
      <xsl:when test="name($node)='bibliography'
	and (name($node/parent::*) = 'article'
	or name($node/parent::*) = 'book')">1</xsl:when>
      <xsl:when test="name($node)='glossary'
	and (name($node/parent::*) = 'article'
	or name($node/parent::*) = 'book')">1</xsl:when>
      <xsl:when test="name($node)='colophon'">1</xsl:when>
      <xsl:when test="name($node)='book'">1</xsl:when>
      <xsl:when test="name($node)='set'">1</xsl:when>
      <xsl:when test="name($node)='setindex'">1</xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="refentry">
    <xsl:apply-imports/>
  </xsl:template>

  <xsl:template match="set|book|part|preface|chapter|appendix
    |article
    |reference
    |book/glossary|article/glossary|part/glossary
    |book/bibliography|article/bibliography
    |colophon">
    <xsl:choose>
      <xsl:when test="$onechunk != 0 and parent::*">
	<xsl:apply-imports/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:call-template name="process-chunk-element"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>

