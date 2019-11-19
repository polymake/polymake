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


  This style sheet filters the documentation for one particular extension.
-->


<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
	xmlns="http://www.polymake.org/ns/docs#3"
>

<xsl:output method="xml"/>


<xsl:template match = "/">
	<polymake_apps>
		<xsl:copy-of select = "document('version.xml')//pm:extension"/>

		<xsl:apply-templates select=".//pm:application">	 
			<xsl:sort select="@name"/>
		</xsl:apply-templates>
	</polymake_apps>
</xsl:template>


<xsl:template match = "pm:application">
	<xsl:if test = "descendant-or-self::*[@ext=$ext_name]">
		<application>
			<xsl:apply-templates select="@*|node()"/>
		</application>
	</xsl:if>
</xsl:template>


<xsl:template match="pm:properties|pm:user-methods|pm:permutations|pm:user-functions|pm:common-option-lists|pm:objects|pm:options|pm:property-types">
	<xsl:if test="descendant::*[@ext=$ext_name]">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()" />
		</xsl:copy>
	</xsl:if>	
</xsl:template>



<xsl:template match="pm:object|pm:property-type">
	<xsl:if test="descendant-or-self::*[@ext=$ext_name]">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()" />
		</xsl:copy>
	</xsl:if>	
</xsl:template>








<xsl:template match="pm:category">
	<xsl:choose>
	<xsl:when test = "@ext=$ext_name">
		<category>
			<xsl:apply-templates select="@*|node()" />
		</category>
	</xsl:when>
	<xsl:when test = "descendant::*[@ext=$ext_name]">
		<xsl:apply-templates select="node()"/>	
	</xsl:when>
	</xsl:choose>	
</xsl:template>



<xsl:template match="pm:imports-from|pm:uses">
	<xsl:if test = "parent::*[@ext=$ext_name]">
		<xsl:copy>
			<xsl:for-each select="pm:application">
				<xsl:sort select = "@name"/>
				<xsl:copy-of select = "."/>
			</xsl:for-each>
		</xsl:copy>
	</xsl:if>
</xsl:template>


<xsl:template match="pm:param|pm:tparam|pm:option|pm:return|pm:derived-from">
	<xsl:if test = "parent::*[@ext=$ext_name]">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()" />
		</xsl:copy>
	</xsl:if>
</xsl:template>

<xsl:template match="pm:description">
	<xsl:if test = "parent::*[@ext=$ext_name] or parent::*/parent::*[@ext=$ext_name]">
	<!-- TODO: problem if there are dependent extensions -->
		<xsl:copy>
			<xsl:apply-templates select="@*|node()" />
		</xsl:copy>
	</xsl:if>
</xsl:template>


<xsl:template match="pm:function|pm:property|pm:permutation|pm:common-option-list">
	<xsl:if test = "@ext=$ext_name">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:if>
</xsl:template>


<xsl:template match="pm:specialization">
	<xsl:if test="parent::*//pm:only[@name=current()/@name][ancestor::*/@ext=$ext_name]">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:if>
</xsl:template>



<xsl:template match="@id">
	<xsl:attribute name="xml:id"><xsl:value-of select = "ancestor::pm:application/@name"/>__<xsl:value-of select="."/></xsl:attribute>
</xsl:template>


<xsl:template match="@href">
	<xsl:attribute name="href">
		<xsl:choose>	
		<xsl:when test="//*[@id=substring-after(current(),'#')][@ext=$ext_name]">
			#<xsl:value-of select="ancestor-or-self::pm:application/@name"/>__<xsl:value-of select="substring-after(current(),'#')"/>
		</xsl:when>
		<xsl:when test="substring-before(.,'#')!=''">
			<xsl:value-of select="."/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="ancestor-or-self::pm:application/@name"/>.html#<xsl:value-of select="substring-after(.,'#')"/>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:attribute>
</xsl:template>


<!-- identity template -->
<xsl:template match="@*|node()">
	<xsl:copy>
		<xsl:apply-templates select="@*|node()" />
	</xsl:copy>
</xsl:template>


</xsl:stylesheet>
