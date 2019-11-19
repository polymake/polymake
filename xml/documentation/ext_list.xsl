<?xml version="1.0" encoding="utf-8"?>

<!--
  Copyright (c) 1997-2019
  Ewgenij Gawrilow, Michael Joswig, and the polymake team
  Technische Universität Berlin, Germany
  https://polymake.org

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version: http://www.gnu.org/licenses/gpl.txt.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
===============================================================================

  This file produces the extensions.html file of the polymake documentation.
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns="http://www.w3.org/1999/xhtml"
  xmlns:pm="http://www.polymake.org/ns/docs#3"
>

<xsl:include href="macros.xsl" />

<xsl:output method="xml" indent="yes" omit-xml-declaration="yes"/>

<xsl:template match="/">
      <div id="content">
        <h1>polymake:extensions</h1>
  
        This is the list of all extensions installed in your version of polymake.

	<xsl:copy-of select="$ext_list"/>
</div>	

</xsl:template>


<xsl:variable name="ext_list">
  <xsl:apply-templates mode="ext" select="document('version.xml')/*"/>
</xsl:variable>

<xsl:template mode="ext" match="pm:extensions">
  <ul class="ext">
    <xsl:for-each select="pm:extension|pm:bundled_extension">
      <li> <a>
      	<xsl:if test="pm:file">
				<xsl:attribute name = "href">
					<xsl:value-of select="pm:file"/>.html
				</xsl:attribute>
      	</xsl:if>
        <xsl:value-of select="pm:URI" /></a> </li>
    </xsl:for-each>
  </ul>
</xsl:template>

</xsl:stylesheet>
