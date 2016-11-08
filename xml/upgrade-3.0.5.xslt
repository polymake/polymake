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

  Renaming of SPLITTING_FLACETS and SPLIT_COMPATIBLE to SPLIT_FLACETS and SPLIT
  Removes property SPLIT_COMPATIBLE in some cases. This is due a change in the definition.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />

<xsl:template match="p:object[@type='matroid::Matroid' or @type='Matroid']//p:property[@name='SPLITTING_FLACETS']">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">SPLIT_FLACETS</xsl:attribute>
      <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:object[@type='matroid::Matroid' or @type='Matroid']//p:property[@name='SPLIT_COMPATIBLE']">
   <xsl:if test="/p:object[@type='matroid::Matroid' or @type='Matroid']//p:property[@name='CONNECTED' and @value='true'] or /p:object[@type='matroid::Matroid' or @type='Matroid']//p:property[@name='SPLIT_COMPATIBLE' and @value='false']">
      <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">SPLIT</xsl:attribute>
      <xsl:copy-of select="*" />
      </xsl:element>
   </xsl:if>
</xsl:template>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
