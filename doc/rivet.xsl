<?xml version="1.0" encoding="ISO-8859-1"?>
  <xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

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

  <!-- This is the body of the XSL stylesheet for Rivet.  It is called
  from either rivet-chunk.xsl or rivet-nochunk.xsl.  -->

  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="header.rule" select="0"/>

  <xsl:param name="navig.graphics" select="1"/>
  <xsl:param name="navig.graphics.extension" select="'.png'"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="generate.section.toc.level" select="1"/>
  <xsl:param name="refentry.separator" select="1"/>

<!--
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="refentry.generate.name" select="0"/>
-->
  <xsl:variable name="arg.choice.opt.open.str">?</xsl:variable>
  <xsl:variable name="arg.choice.opt.close.str">?</xsl:variable>
  <xsl:variable name="group.choice.opt.open.str">(</xsl:variable>
  <xsl:variable name="group.choice.opt.close.str">)</xsl:variable>

  <!--
  <xsl:variable name="arg.choice.req.open.str"></xsl:variable>
  <xsl:variable name="arg.choice.req.close.str"></xsl:variable>
  -->
  <xsl:variable name="arg.choice.def.open.str"></xsl:variable>
  <xsl:variable name="arg.choice.def.close.str"></xsl:variable>
  <xsl:variable name="group.choice.def.open.str"></xsl:variable>
  <xsl:variable name="group.choice.def.close.str"></xsl:variable>
  <xsl:variable name="group.choice.req.open.str">(</xsl:variable>
  <xsl:variable name="group.choice.req.close.str">)</xsl:variable>

  <xsl:variable name="group.rep.repeat.str">...</xsl:variable>

  <xsl:template name="inline.underlineseq">
    <xsl:param name="content">
      <xsl:call-template name="anchor"/>
      <xsl:apply-templates/>
    </xsl:param>
    <span style="text-decoration:underline">
      <xsl:copy-of select="$content"/>
    </span>
  </xsl:template>

  <xsl:template name="inline.monounderlineseq">
    <xsl:param name="content">
      <xsl:call-template name="anchor"/>
      <xsl:apply-templates/>
    </xsl:param>
    <tt><span style="text-decoration:underline"><xsl:copy-of
	  select="$content"/></span></tt>
  </xsl:template>

  <xsl:template match="/article/section/title" mode="titlepage.mode"
    priority="2">
    <hr/>
    <xsl:call-template name="section.title"/>
  </xsl:template>

  <xsl:template match="para">
    <xsl:variable name="p">
      <p style="width:90%">
	<xsl:if test="position() = 1 and parent::listitem">
	  <xsl:call-template name="anchor">
	    <xsl:with-param name="node" select="parent::listitem"/>
	  </xsl:call-template>
	</xsl:if>
	<xsl:call-template name="anchor"/>
	<xsl:apply-templates/>
      </p>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$html.cleanup != 0">
	<xsl:call-template name="unwrap.p">
	  <xsl:with-param name="p" select="$p"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
	<xsl:copy-of select="$p"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="optional">
    <xsl:value-of select="$arg.choice.opt.open.str"/>
    <xsl:call-template name="inline.charseq"/>
    <xsl:value-of select="$arg.choice.opt.close.str"/>
  </xsl:template>

  <xsl:template match="option">
    <!--    <xsl:call-template name="inline.monounderlineseq"/> -->
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="command">
    <span style="font-family:monospace">
      <xsl:call-template name="inline.boldseq"/>
    </span>
  </xsl:template>

  <xsl:template match="cmdsynopsis">
    <div class="{name(.)}" style="width:80%">
      <div style="background:#bbbbff ; margin:1ex ; padding:.4ex ;
	word-spacing:1ex ">
	<xsl:call-template name="anchor"/>
	<xsl:apply-templates/>
      </div>
    </div>
  </xsl:template>

  <xsl:template match="cmdsynopsis/command">
    <span style="font-weight:bold ; font-family:monospace">
      <xsl:apply-templates/>
    </span>
    <xsl:text> </xsl:text>
  </xsl:template>

  <xsl:template match="varlistentry">
    <dt>
      <xsl:call-template name="anchor"/>
      <xsl:apply-templates select="term"/>
    </dt>
    <dd>
      <div style="padding:4 ; margin-top:3 ;
	margin-bottom:3 ; width:75%" >
	<xsl:apply-templates select="listitem"/>
      </div>
    </dd>
  </xsl:template>

  <xsl:template match="listitem/para">
    <div style="margin-bottom:1.5ex ; padding .5ex">
      <xsl:apply-templates/>
    </div>
  </xsl:template>

  <xsl:template match="arg">
    <xsl:variable name="choice" select="@choice"/>
    <xsl:variable name="rep" select="@rep"/>
    <xsl:variable name="sepchar">
      <xsl:choose>
	<xsl:when test="ancestor-or-self::*/@sepchar">
	  <xsl:value-of select="ancestor-or-self::*/@sepchar"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:text> </xsl:text>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="position()>1"><xsl:value-of select="$sepchar"/></xsl:if>
    <xsl:choose>
      <xsl:when test="$choice='plain'">
	<xsl:value-of select="$arg.choice.plain.open.str"/>
      </xsl:when>
      <xsl:when test="$choice='req'">
	<xsl:value-of select="$arg.choice.req.open.str"/>
      </xsl:when>
      <xsl:when test="$choice='opt'">
	<xsl:value-of select="$arg.choice.opt.open.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$arg.choice.def.open.str"/>
      </xsl:otherwise>
    </xsl:choose>
    <span style="font-family:monospace; font-weight: bold;">
      <xsl:apply-templates/>
    </span>
    <xsl:choose>
      <xsl:when test="$rep='repeat'">
	<xsl:value-of select="$arg.rep.repeat.str"/>
      </xsl:when>
      <xsl:when test="$rep='norepeat'">
	<xsl:value-of select="$arg.rep.norepeat.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$arg.rep.def.str"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$choice='plain'">
	<xsl:value-of select="$arg.choice.plain.close.str"/>
      </xsl:when>
      <xsl:when test="$choice='req'">
	<xsl:value-of select="$arg.choice.req.close.str"/>
      </xsl:when>
      <xsl:when test="$choice='opt'">
	<xsl:value-of select="$arg.choice.opt.close.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$arg.choice.def.close.str"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="group">
    <xsl:variable name="choice" select="@choice"/>
    <xsl:variable name="rep" select="@rep"/>
    <xsl:variable name="sepchar">
      <xsl:choose>
	<xsl:when test="ancestor-or-self::*/@sepchar">
	  <xsl:value-of select="ancestor-or-self::*/@sepchar"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:text> </xsl:text>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="position()>1"><xsl:value-of select="$sepchar"/></xsl:if>
    <xsl:choose>
      <xsl:when test="$choice='plain'">
	<xsl:value-of select="$group.choice.plain.open.str"/>
      </xsl:when>
      <xsl:when test="$choice='req'">
	<xsl:value-of select="$group.choice.req.open.str"/>
      </xsl:when>
      <xsl:when test="$choice='opt'">
	<xsl:value-of select="$group.choice.opt.open.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$group.choice.def.open.str"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates/>
    <xsl:choose>
      <xsl:when test="$rep='repeat'">
	<xsl:value-of select="$group.rep.repeat.str"/>
      </xsl:when>
      <xsl:when test="$rep='norepeat'">
	<xsl:value-of select="$arg.rep.norepeat.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$arg.rep.def.str"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$choice='plain'">
	<xsl:value-of select="$group.choice.plain.close.str"/>
      </xsl:when>
      <xsl:when test="$choice='req'">
	<xsl:value-of select="$group.choice.req.close.str"/>
      </xsl:when>
      <xsl:when test="$choice='opt'">
	<xsl:value-of select="$group.choice.opt.close.str"/>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$group.choice.def.close.str"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="group/arg">
    <xsl:variable name="choice" select="@choice"/>
    <xsl:variable name="rep" select="@rep"/>
    <xsl:if test="position()>1"><xsl:value-of
	select="$arg.or.sep"/></xsl:if>
    <span style="font-family:monospace; font-weight: bold;">
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="programlisting|screen|synopsis">
    <xsl:param name="suppress-numbers" select="'0'"/>
    <xsl:variable name="id">
      <xsl:call-template name="object.id"/>
    </xsl:variable>

    <xsl:call-template name="anchor"/>

    <xsl:variable name="content">
      <xsl:choose>
	<xsl:when test="$suppress-numbers = '0'
	  and @linenumbering = 'numbered'
	  and $use.extensions != '0'
	  and $linenumbering.extension != '0'">
	  <xsl:variable name="rtf">
	    <xsl:apply-templates/>
	  </xsl:variable>
	  <pre class="{name(.)}">
	    <xsl:call-template name="number.rtf.lines">
	      <xsl:with-param name="rtf" select="$rtf"/>
	    </xsl:call-template>
	  </pre>
	</xsl:when>
	<xsl:otherwise>
	  <pre style="background:#bbffbb ; width:90ex ; margin: 2ex ;
	    padding: 1ex; border: solid black 1px ; white-space: pre;
	    font-family:monospace ; " class="{name(.)}">
	    <xsl:apply-templates/>
	  </pre>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$shade.verbatim != 0">
	<table xsl:use-attribute-sets="shade.verbatim.style">
	  <tr>
	    <td>
	      <xsl:copy-of select="$content"/>
	    </td>
	  </tr>
	</table>
      </xsl:when>
      <xsl:otherwise>
	<xsl:copy-of select="$content"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>