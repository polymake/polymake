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

  This file produces the index of the polymake documentation.
-->

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:html="http://www.w3.org/1999/xhtml"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
	xmlns="http://www.w3.org/1999/xhtml"
>

<xsl:include href="macros.xsl" />

<xsl:output method="xml" indent="yes" omit-xml-declaration="yes" />


<xsl:template match="/">
	<div id="content">
	<h1>polymake: index</h1>
	<p>This table contains all entities available in polymake in alphabetical order.</p>
	<p>For everything listed, the <b>name</b>, the <b>type</b> (i.e., one of object, property, function, property-type, application, option, common-option-list or permutation), the corresponding <b>application</b> and, for properties and user-methods the <b>object</b> or <b>property-type</b> which they belong to are given.</p> 

	<br/>
	<table border="1">
		<tr>
			<th align="left">Name</th>
			<th align="left">Type</th>
			<th align="left">Object or Property-Type</th>
			<th align="left">Application</th>
		</tr>
	
		<xsl:apply-templates select="//*[@name!='']">
			<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')" lang="en"/>
		</xsl:apply-templates>	 
					
	</table>

				
	</div>
</xsl:template>



<xsl:template match="//*[@name!='']">

<xsl:if test="name()='object' or name()='application' or name()='property-type' or name()='function' or name()='property' or name()='permutation' or name()='option' or name()='common-option-list'">
<xsl:if test="name()!='option' or ancestor::pm:common-option-list">
<xsl:if test="not(name()='application' and @name!=ancestor-or-self::pm:application/@name)">
<xsl:if test="name()!='common-option-list' or not(@href)">
	<tr>
		<td><b><a>
			<xsl:choose>
			<xsl:when test="name()='option'">
				<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="ancestor::pm:common-option-list/@id"/></xsl:attribute>
			</xsl:when>
			<xsl:otherwise>
				<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="@id"/></xsl:attribute>
			</xsl:otherwise>
			</xsl:choose>
			<xsl:value-of select="@name"/>
		</a></b></td>
		
		<td><xsl:value-of select="name()"/></td>
		
		<td><a>
			<xsl:choose>
			<xsl:when test="ancestor::pm:object">
				<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="ancestor::pm:object/@id"/></xsl:attribute>
				<xsl:value-of select="ancestor::pm:object/@name"/>
			</xsl:when>
			<xsl:when test="ancestor::pm:property-type">
				<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="ancestor::pm:property-type/@id"/></xsl:attribute>
				<xsl:value-of select="ancestor::pm:property-type/@name"/>
			</xsl:when>
			<xsl:when test="ancestor::pm:common-option-list">
				<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="ancestor::pm:common-option-list/@id"/></xsl:attribute>
				<xsl:value-of select="ancestor::pm:common-option-list/@name"/>
			</xsl:when>
			</xsl:choose>
		</a></td>

		<td><a>
			<xsl:attribute name="href"><xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html</xsl:attribute>
			<xsl:value-of select="ancestor-or-self::pm:application/@name"/>		
		</a></td>
	</tr>
	
</xsl:if></xsl:if></xsl:if></xsl:if>

</xsl:template>



</xsl:stylesheet>
