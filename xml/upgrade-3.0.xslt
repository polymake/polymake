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

  Rename LatticePolytope into Polytope<Rational> and AffineNormalToricVariety into NormalToricVariety
  because the former have become specializations.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="description" indent="yes" />

<xsl:template match="p:object[@type='LatticePolytope']">
  <xsl:call-template name="replace-object-type">
    <xsl:with-param name="new_type">Polytope&lt;Rational&gt;</xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="p:object[@type='polytope::LatticePolytope']">
  <xsl:call-template name="replace-object-type">
    <xsl:with-param name="new_type">polytope::Polytope&lt;Rational&gt;</xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="p:object[@type='AffineNormalToricVariety']">
  <xsl:call-template name="replace-object-type">
    <xsl:with-param name="new_type">NormalToricVariety</xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="p:object[@type='fulton::AffineNormalToricVariety']">
  <xsl:call-template name="replace-object-type">
    <xsl:with-param name="new_type">fulton::NormalToricVariety</xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template name="replace-object-type">
  <xsl:param name="new_type"/>
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='type']" />
    <xsl:attribute name="type"><xsl:value-of select="$new_type" /></xsl:attribute>
    <xsl:apply-templates select="./node()" />
  </xsl:element>
</xsl:template>

<!-- detect PointConfiguration objects -->
<xsl:template match="/p:object[@type[contains(string(),'PointConfiguration') or contains(string(),'VectorConfiguration')]]">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
    <xsl:apply-templates select="./node()" mode="pointconfig"/>
  </xsl:element>
</xsl:template>

<!-- drop DIM as it was inconsistent between PointConfiguration and VectorConfiguration -->
<xsl:template match="p:property[@name='DIM']" mode="pointconfig"/>

<!-- drop AMBIENT_DIM as it was inconsistent between PointConfiguration and VectorConfiguration -->
<xsl:template match="p:property[@name='AMBIENT_DIM']" mode="pointconfig"/>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="pointconfig">
  <xsl:apply-templates select="." />
</xsl:template>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
