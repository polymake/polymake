<?xml version="1.0"?>

<!--
  Copyright (c) 1997-2014
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

  Make any SimplicialComplex containing one of GEOMETRIC_REALIZATION, G_DIM, VOLUME, SIGNATURE into a GeometricSimplicialComplex

  Change the property GEOMETRIC_REALIZATION to COORDINATES

  In all objects of type Polytope:
    Replace MINKOWSKI_SUMMAND_CONE with MINKOWSKI_CONE.

  Upgrade LATTICE_POINTS to LATTICE_POINTS_GENERATORS.

  Change the object TropicalHypersurface<Rational> to Hypersurface<Min>

  Changed encoding for Rings of polynomials.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />

<!-- 1 -->

<!-- We search for objects with properties of a GeometricSimplicialComplex -->

<xsl:template match="p:object">
  <xsl:variable name="attr_type" select="@type" />
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
    <xsl:choose>
      <xsl:when test="$attr_type='topaz::SimplicialComplex'">
	<!-- we have a SimplicialComplex -->
	<xsl:variable name="has_GEOMETRIC_REALIZATION" select="./p:property[@name='GEOMETRIC_REALIZATION']" />
	<xsl:variable name="has_G_DIM" select="./p:property[@name='G_DIM']" />
	<xsl:variable name="has_VOLUME" select="./p:property[@name='VOLUME']" />
	<xsl:variable name="has_SIGNATURE" select="./p:property[@name='SIGNATURE']" />
	
	<xsl:if test="(count($has_GEOMETRIC_REALIZATION)>=1) or (count($has_G_DIM)>=1) or (count($has_VOLUME)>=1) or (count($has_SIGNATURE)>=1)">
	  <!-- we need to change the type to GeometricSimplicialComplex -->
	  <xsl:attribute name="type">topaz::GeometricSimplicialComplex&lt;Rational&gt;</xsl:attribute>
	</xsl:if>
	<xsl:for-each select="*">
	  <xsl:apply-templates select="." />
	</xsl:for-each>
      </xsl:when>
      
      <xsl:otherwise>
	<xsl:apply-templates  />
      </xsl:otherwise>

    </xsl:choose>
  </xsl:element>

</xsl:template>

<!-- the name GEOMETRIC_REALIZATION gets changed to COORDINATES -->

<xsl:template match="p:property[@name='GEOMETRIC_REALIZATION']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">COORDINATES</xsl:attribute>
    <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>

<!-- detect the LatticePolytope objects -->
<xsl:template match="/p:object[@type[contains(string(),'Polytope')]]">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
    <xsl:apply-templates select="./node()" mode="polytope" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='MINKOWSKI_SUMMAND_CONE']" mode="polytope">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">MINKOWSKI_CONE</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="polytope" />
  </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="polytope">
  <xsl:apply-templates select="." />
</xsl:template>

<xsl:template match="p:property[@name='LATTICE_POINTS']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">LATTICE_POINTS_GENERATORS</xsl:attribute>
      <xsl:element name="m" namespace="{namespace-uri()}">
        <xsl:copy-of select="*" />
          <xsl:element name="m" namespace="{namespace-uri()}" />
          <xsl:element name="m" namespace="{namespace-uri()}" />
      </xsl:element>
  </xsl:element>
</xsl:template>

<!-- Rename object TropicalHypersurface -->
<xsl:template match="p:object[@type='tropical::TropicalHypersurface&lt;Rational&gt;']">

  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='version']" />
    <!-- Rename object TropicalHypersurface&lt;Rational&gt; to Hypersurface&lt;Min&gt; -->
    <xsl:attribute name="type">tropical::Hypersurface&lt;Min&gt;</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="trophypsurface" />
  </xsl:element>

</xsl:template>

<!-- Substitute N_VARS = Hypersurface::AMBIENT_DIM + 1 -->
<xsl:template match="p:property[@name='AMBIENT_DIM']" mode="trophypsurface">
  <xsl:variable name="amb_dim" select="@value" /> 
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">N_VARS</xsl:attribute>
    <xsl:attribute name="value">
      <xsl:value-of select="$amb_dim + 1" />
    </xsl:attribute>
  </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="trophypsurface">
  <xsl:apply-templates select="." />
</xsl:template>

<!-- enclose the list of variable names in the Ring in an extra `t' element -->

<xsl:template match="p:property[@name='GENERATORS' or @name='BASIS']/p:v/p:t/p:v">
  <xsl:element name="t" namespace="{namespace-uri()}">
    <xsl:copy-of select="." />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='HILBERT_POLYNOMIAL' or @name='CHARACTERISTIC_POLYNOMIAL']/p:t/p:v">
  <xsl:element name="t" namespace="{namespace-uri()}">
    <xsl:copy-of select="." />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='RING']/p:v">
  <xsl:element name="t" namespace="{namespace-uri()}">
    <xsl:copy-of select="." />
  </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim -->

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
