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

  Insert a "cols" attribute in empty coordinate matrices like AFFINE_HULL or LINEALITY_SPACE
  wherever possible; remove ones when no dimension property is at hand
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />

<xsl:template match="p:property[@name='AFFINE_HULL' or @name='LINEAR_SPAN' or @name='LINEAR_SPAN_NORMALS' or @name='LINEALITY_SPACE' or @name='EQUATIONS' or @name='INPUT_LINEALITY' or @name='INPUT_RAYS' or @name='LINEALITY_VALUES']">
  <xsl:variable name="needs_cols"><xsl:apply-templates mode="needs_cols" select="." /></xsl:variable>
  <xsl:choose>
    <xsl:when test="$needs_cols = 'true'">
      <xsl:variable name="dim"><xsl:apply-templates mode="fetch_dim" select="../*[@value]"><xsl:with-param name="for_prop" select="@name" /></xsl:apply-templates></xsl:variable>
      <xsl:if test="$dim!=''">
        <!-- empty matrix must be provided with "cols" -->
        <xsl:element name="{name()}" namespace="{namespace-uri()}">
          <xsl:copy-of select="@*" />
          <xsl:element name="m" namespace="{namespace-uri()}">
            <xsl:attribute name="cols"><xsl:value-of select="$dim" /></xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <!-- non-empty matrix or known number of columns: no changes -->
      <xsl:element name="{name()}" namespace="{namespace-uri()}">
        <xsl:copy-of select="@*" />
        <xsl:apply-templates select="./node()" />
      </xsl:element>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- undefined value must not have columns -->
<xsl:template mode="needs_cols" match="p:property[@undef]" />

<!-- count the rows unless the cols attribute already exists -->
<xsl:template mode="needs_cols" match="p:property">
  <xsl:value-of select="count(./p:m[@cols])=0 and count(./p:m/*)=0 and count(./p:v)=0" />
</xsl:template>

<xsl:template mode="fetch_dim" match="p:property[@name='CONE_AMBIENT_DIM' or @name='FAN_AMBIENT_DIM' or @name='VECTOR_AMBIENT_DIM']">
  <xsl:param name="for_prop" />
  <xsl:if test="$for_prop!='LINEALITY_VALUES'">
    <xsl:value-of select="@value" />
  </xsl:if>
</xsl:template>

<xsl:template mode="fetch_dim" match="*" />

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
