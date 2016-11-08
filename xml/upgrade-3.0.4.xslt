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

  Remove the template parameter HasDual/HasNoDual from Matroid objects.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />

<xsl:template match="p:object[@type='matroid::Matroid&lt;HasDual&gt;' or @type='matroid::Matroid&lt;HasNoDual&gt;']">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='tm' and name()!='version']" />
    <xsl:attribute name="type">matroid::Matroid</xsl:attribute>
    <xsl:apply-templates select="./node()" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:object[@type='Matroid&lt;HasDual&gt;' or @type='Matroid&lt;HasNoDual&gt;']">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type']" />
    <xsl:attribute name="type">Matroid</xsl:attribute>
    <xsl:apply-templates select="./node()" />
  </xsl:element>
</xsl:template>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
