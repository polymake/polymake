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

  PointConfiguration:
    Rename POLYTOPE_AMBIENT_DIM to AMBIENT_DIM.
    Rename POLYTOPE_DIM to DIM.

  PolyhedralFan:
    Rename DIM to FAN_DIM.
    Rename AMBIENT_DIM to FAN_AMBIENT_DIM.
    Decrease COMBINATORIAL_DIM by 1.
    Replace MAXIMAL_CONES_DIMS with MAXIMAL_CONES_COMBINATORIAL_DIMS and amend if necessary.

  SimplicialComplex:
    Rename BALANCED to FOLDABLE.

  Polytope etc.:
    Move TRIANGULATION.FACET_TRIANGULATIONS to TRIANGULATION.BOUNDARY.FACET_TRIANGULATIONS.

  Polytope, PointConfiguration:
    Create a subobject for POLYTOPAL_SUBDIVISION.
    Move WEIGHTS into TRIANGULATION if WEIGHTS_GENERIC.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                             xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description p:credit" indent="yes" />

<!-- detect top-level objects -->
<xsl:template match="/p:object">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
    <xsl:apply-templates select="./node()"/>
  </xsl:element>
</xsl:template>

<!-- rename PointConfiguration::POLYTOPE_DIM to DIM and POLYTOPE_AMBIENT_DIM to AMBIENT_DIM -->
<xsl:template match="/p:object[@type[contains(string(),'PointConfiguration')]]/p:property[@name='POLYTOPE_DIM' or @name='POLYTOPE_AMBIENT_DIM']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">
      <xsl:value-of select="substring-after(@name, 'POLYTOPE_')" />
    </xsl:attribute>
    <xsl:attribute name="value">
      <xsl:value-of select="@value" />
    </xsl:attribute>
  </xsl:element>
</xsl:template>

<!-- decrease PolyhedralFan::COMBINATORIAL_DIM -->
<xsl:template match="/p:object[@type[contains(string(),'PolyhedralFan')]]/p:property[@name='COMBINATORIAL_DIM']">
  <xsl:variable name="com_dim" select="@value" /> 
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">COMBINATORIAL_DIM</xsl:attribute>
    <xsl:attribute name="value">
      <xsl:value-of select="$com_dim - 1" />
    </xsl:attribute>
  </xsl:element>
</xsl:template>

<!-- transform PolyhedralFan::MAXIMAL_CONES_DIMS -->
<xsl:template match="/p:object[@type[contains(string(),'PolyhedralFan')]]/p:property[@name='MAXIMAL_CONES_DIMS']">
  <xsl:variable name="lin_dim" select="../p:property[@name='LINEALITY_DIM']/@value" />
  <xsl:variable name="subtract_dim">
    <xsl:choose>
      <xsl:when test="$lin_dim != ''"> <xsl:value-of select="$lin_dim + 1"/> </xsl:when>
      <xsl:otherwise> 1 </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">MAXIMAL_CONES_COMBINATORIAL_DIMS</xsl:attribute>
    <xsl:element name="v" namespace="{namespace-uri()}">
      <xsl:call-template name="add-constant-to-list">
	<xsl:with-param name="values" select="./p:v/text()"/>
        <xsl:with-param name="constant" select="-$subtract_dim"/>
      </xsl:call-template>
    </xsl:element>
  </xsl:element>
</xsl:template>

<!-- rename PolyhedralFan::DIM to FAN_DIM and AMBIENT_DIM to FAN_AMBIENT_DIM -->
<xsl:template match="/p:object[@type[contains(string(),'PolyhedralFan')]]/p:property[@name='DIM' or @name='AMBIENT_DIM']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">
      <xsl:value-of select="concat('FAN_', @name)" />
    </xsl:attribute>
    <xsl:attribute name="value">
      <xsl:value-of select="@value" />
    </xsl:attribute>
  </xsl:element>
</xsl:template>


<!-- Move TRIANGULATION.FACET_TRIANGULATIONS to TRIANGULATION.BOUNDARY.FACET_TRIANGULATIONS -->
<xsl:template match="p:property[@name='TRIANGULATION']/p:object">
  <xsl:variable name="has_FACET_TR" select="./p:property[@name='FACET_TRIANGULATIONS']" />
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*" />
    <xsl:choose>
      <xsl:when test="count($has_FACET_TR)=1">
        <xsl:element name="property" namespace="{namespace-uri()}">
          <xsl:attribute name="name">BOUNDARY</xsl:attribute>
          <xsl:element name="object" namespace="{namespace-uri()}">
            <xsl:copy-of select="$has_FACET_TR" />
            <xsl:for-each select="./p:property[@name='BOUNDARY']/p:object/*">
              <xsl:apply-templates select="." />
            </xsl:for-each>
          </xsl:element>
        </xsl:element>
        <xsl:for-each select="./*[name()!='property' or (@name!='FACET_TRIANGULATIONS' and @name!='BOUNDARY')]">
          <xsl:apply-templates select="." />
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="./*" />
      </xsl:otherwise>
    </xsl:choose>

    <!-- Move WEIGHTS into TRIANGULATION if WEIGHTS_GENERIC is true -->
    <xsl:variable name="weights" select="../../p:property[@name='WEIGHTS']" />
    <xsl:if test="count($weights)=1">
      <xsl:variable name="wg" select="normalize-space(../../p:property[@name='WEIGHTS_GENERIC']/@value)" />
      <xsl:if test="$wg='true' and count(../../p:property[@name='POLYTOPAL_SUBDIVISION'])=0">
        <xsl:copy-of select="$weights" />
      </xsl:if>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='WEIGHTS']">
  <!-- only process WEIGHTS here if no other property takes care of it -->
  <xsl:if test="count(../p:property[@name='POLYTOPAL_SUBDIVISION'])=0">
    <xsl:variable name="wg" select="normalize-space(../p:property[@name='WEIGHTS_GENERIC']/@value)" />
    <xsl:choose>
      <xsl:when test="$wg='true'">
        <xsl:if test="count(../p:property[@name='TRIANGULATION'])=0">
          <xsl:element name="property" namespace="{namespace-uri()}">
            <xsl:attribute name="name">TRIANGULATION</xsl:attribute>
            <xsl:element name="object" namespace="{namespace-uri()}">
              <xsl:copy-of select="." />
            </xsl:element>
          </xsl:element>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="PD-create" select=".." />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>

<xsl:template match="p:property[@name='POLYTOPAL_SUBDIVISION']">
  <xsl:apply-templates mode="PD-create" select=".." />
</xsl:template>

<!-- These properties are copied in other templates -->
<xsl:template match="p:property[@name='WEIGHTS_GENERIC' or @name='SPLITS_IN_SUBDIVISION']" />

<xsl:template mode="PD-create" match="*">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">fan::POLYTOPAL_SUBDIVISION</xsl:attribute>
    <xsl:element name="object" namespace="{namespace-uri()}">
      <xsl:for-each select="*">
        <xsl:variable name="PD-props">
          <xsl:apply-templates mode="PD-props" select="." />
        </xsl:variable>
        <xsl:if test="$PD-props!=''">
          <xsl:element name="property" namespace="{namespace-uri()}">
            <xsl:attribute name="name">
              <xsl:value-of select="$PD-props" />
            </xsl:attribute>
            <xsl:copy-of select="@*[name()!='name']" />
            <xsl:apply-templates select="./node()" />
          </xsl:element>
        </xsl:if>
      </xsl:for-each>
    </xsl:element>
  </xsl:element>
</xsl:template>


<!-- These properties will be moved into POLYTOPAL_SUBDIVISION  -->

<xsl:template mode="PD-props" match="p:property[@name='SPLITS_IN_SUBDIVISION']">
  <xsl:text>fan::REFINED_SPLITS</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='WEIGHTS']">
  <xsl:text>fan::WEIGHTS</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='POLYTOPAL_SUBDIVISION']">
  <xsl:text>MAXIMAL_CELLS</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="*" />


<!-- rename SimplicialComplex::BALANCED to FOLDABLE -->
<xsl:template match="/p:object[@type[contains(string(),'SimplicialComplex')]]/p:property[@name='BALANCED']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">FOLDABLE</xsl:attribute>
    <xsl:attribute name="value">
      <xsl:value-of select="@value" />
    </xsl:attribute>
  </xsl:element>
</xsl:template>


<xsl:include href="trivial-copy.xslt" />
<xsl:include href="list-ops.xslt" />

</xsl:transform>
