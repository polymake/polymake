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

  1. Rename GROEBNER.ORDER to GROEBNER.ORDER_NAME
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />


<!-- 1. -->


<xsl:template match="p:property[@name='GROEBNER']/p:object/p:property[@name='ORDER']">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:attribute name="name">ORDER_NAME</xsl:attribute>
      <xsl:attribute name="value">
         <xsl:value-of select="@value" />
      </xsl:attribute>
   </xsl:element>
</xsl:template>


<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
