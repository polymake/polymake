<?xml version="1.0"?>

<!--
  Copyright (c) 1997-2016
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

   Rename tropical::Cone to tropical::Polytope
   Rename its properties CONE_COVECTOR_DECOMPOSITION, CONE_MAXIMAL_COVECTORS, CONE_MAXIMAL_COVECTOR_CELLS to
   POLYTOPE_...
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<!-- Rename properties of tropical cone -->

<xsl:template match="p:property[@name='CONE_MAXIMAL_COVECTORS']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_MAXIMAL_COVECTORS</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='CONE_MAXIMAL_COVECTOR_CELLS']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_MAXIMAL_COVECTOR_CELLS</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='CONE_COVECTOR_DECOMPOSITION']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_COVECTOR_DECOMPOSITION</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<!-- Rename cone itself, but keep template parameter! -->

<xsl:template match="p:object[@type[contains(string(),'tropical::Cone')]]">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='version']" />
    <xsl:attribute name="type">
      <xsl:value-of select="concat('tropical::Polytope&lt;',substring-after(@type,'&lt;'))"/>
    </xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="tropical">
  <xsl:apply-templates select="." />
</xsl:template>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
