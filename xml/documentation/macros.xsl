<?xml version="1.0" encoding="utf-8"?>

<!--
  Copyright (c) 1997-2015
  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
  http://www.polymake.org

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version: http://www.gnu.org/licenses/gpl.txt.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
===============================================================================

  Common pieces of documentation processing stylesheets
-->

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
        xmlns="http://www.w3.org/1999/xhtml"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
>

<xsl:variable name="version_text">
  <xsl:apply-templates mode="version" select="document('version.xml')/*"/>
</xsl:variable>

<xsl:template name="nav-box">
	<div class="sidebar">
		<div class="sidebox">
			<h1>Navigation</h1>
			<ul>
				<li> <a href="index.html#apps">applications</a> </li>
				<li> <a href="core.html">core functionality</a></li>
				<li> <a href="doc_index.html">index</a> </li>
				<xsl:if test="document('version.xml')//pm:file"><li> <a href="extensions.html">extensions</a> </li></xsl:if>
          <!-- FIXME: remove the condition as soon as the doxygen documentation ripens enough to be published without disgrace -->
           		 <xsl:if test="contains($version_text,'snapshot')">
					<li> <a href="PTL/index.html">PTL</a></li>
           		 </xsl:if>
            	<xsl:if test="contains($version_text,'snapshot')">
					<li> <a href="/release_docs/">release documentation</a></li>
            	</xsl:if>
			</ul>  
		</div>  
	</div>
</xsl:template>

<xsl:template name="wiki-ref">
  <xsl:param name="reftext" />
  <xsl:element name="a">
    <xsl:attribute name="href">
      <xsl:value-of select="$Wiki"/>
    </xsl:attribute>
    <xsl:value-of select="$reftext" />
  </xsl:element>
</xsl:template>

<xsl:template mode="version" match="/pm:version">
  <xsl:apply-templates mode="version" select="*" /><xsl:if test="not(pm:non_bundled)">.</xsl:if>
</xsl:template>

<xsl:template mode="version" match="pm:commit">
  <xsl:text> extracted from git commit </xsl:text> 
  <a>
    <xsl:attribute name = "href">http://trac.polymake.org/browser/?rev=<xsl:value-of select="normalize-space(current())"/>
    </xsl:attribute>
    <xsl:value-of select="normalize-space(current())"/> </a>
</xsl:template>

<xsl:template mode="version" match="pm:release">
  <xsl:text> as of release </xsl:text> <xsl:value-of select="normalize-space(current())"/>
</xsl:template>

<xsl:template mode="version" match="pm:extensions">
	<xsl:if test="descendant::pm:non_bundled">
  <xsl:text> with the following individual extensions:</xsl:text>
  <ul class="ext">
    <xsl:for-each select="pm:extension[./pm:non_bundled]">
      <li> <a>
      	<xsl:if test="pm:file">
				<xsl:attribute name = "href">
					<xsl:value-of select="pm:file"/>.html
				</xsl:attribute>
      	</xsl:if>
        <xsl:value-of select="pm:URI" /></a> </li>
    </xsl:for-each>
  </ul>
  </xsl:if>
</xsl:template>
  
  
<xsl:variable name = "ext_sn">
	<xsl:choose>
	<xsl:when test = "//pm:extension[./pm:URI=$ext_name]/pm:name!=''">
		<xsl:value-of select = "//pm:extension[./pm:URI=$ext_name]/pm:name"/>
	</xsl:when>
	<xsl:otherwise>
		<xsl:value-of select = "//pm:extension/pm:URI[.=$ext_name]"/>	
	</xsl:otherwise>
	</xsl:choose>
</xsl:variable>

<xsl:variable name = "ext_file">
	<xsl:value-of select = "//pm:extension[./pm:URI=$ext_name]/pm:file"/>
</xsl:variable>


<xsl:template name="LFsToBRs">
	<xsl:param name="input" />
	<xsl:choose>
		<xsl:when test="contains($input, '&#10;')">
			<xsl:value-of select="substring-before($input, '&#10;')" /><br />
			<xsl:call-template name="LFsToBRs">
				<xsl:with-param name="input" select="substring-after($input, '&#10;')" />
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$input" />
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
