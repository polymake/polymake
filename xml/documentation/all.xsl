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
  
  
  This file copies the content of every application's xml file into a single file, which is then used for the creation of the index of the documentation.
-->


<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
>

<xsl:output method="xml"/>

<xsl:template match="/">
<pm:polymake_apps>

	<xsl:for-each select="/pm:applications/pm:file">
		<xsl:apply-templates select="document(@name)/pm:application" />
	</xsl:for-each>

</pm:polymake_apps>
</xsl:template>


<xsl:template match="@*|node()">
   <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
   </xsl:copy>
</xsl:template>

<xsl:template match="@xml:id">
	<xsl:attribute name="id"><xsl:value-of select="."/></xsl:attribute>
</xsl:template>


</xsl:stylesheet>
