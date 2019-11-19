<?xml version="1.0"?>

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

content of the upgrade-2.13.*.xslt files
  1. Rename GROEBNER.ORDER to GROEBNER.ORDER_NAME
  2. Delete N_INEQUALITIES, N_EQUATIONS and SUPPORT_SIZE
  3. Matroid:
       Rename POINTS to VECTORS.
       Rename BINARY_POINTS to BINARY_VECTORS.
       Rename TERNARY_POINTS to TERNARY_VECTORS.
  4. No actual transformation, just enforcing upgrade of data files.
     Introduced together with support for object instance IDs, currently used for polynomial rings.
-->

<!-- header -->
<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />
<!-- end header -->

<!-- 1. -->
<xsl:template match="p:property[@name='GROEBNER']/p:object/p:property[@name='ORDER']">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:attribute name="name">ORDER_NAME</xsl:attribute>
      <xsl:attribute name="value">
         <xsl:value-of select="@value" />
      </xsl:attribute>
   </xsl:element>
</xsl:template>

<!-- 2. -->
<xsl:template match="p:property[@name='N_INEQUALITIES']"/>
<xsl:template match="p:property[@name='N_EQUATIONS']"/>
<xsl:template match="p:property[@name='SUPPORT_SIZE']"/>

<!-- 3. -->
<!-- rename Matroid::POINTS to VECTORS -->
<xsl:template match="/p:object[@type[contains(string(),'Matroid')]]/p:property[@name='POINTS']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*" />
    <xsl:attribute name="name">VECTORS</xsl:attribute>
      <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>

<!-- rename Matroid::BINARY_POINTS to BINARY_VECTORS -->
<xsl:template match="/p:object[@type[contains(string(),'Matroid')]]/p:property[@name='BINARY_POINTS']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*" />
    <xsl:attribute name="name">BINARY_VECTORS</xsl:attribute>
    <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>

<!-- rename Matroid::TERNARY_POINTS to TERNARY_VECTORS -->
<xsl:template match="p:object[@type[contains(string(),'Matroid')]]/p:property[@name='TERNARY_POINTS']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*" />
    <xsl:attribute name="name">TERNARY_VECTORS</xsl:attribute>
      <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>


<!-- footer -->
<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
