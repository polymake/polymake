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

-->

<xsl:transform version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                             xmlns:libxslt="http://xmlsoft.org/XSLT/namespace"
                             xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />




<!-- Transform a vector of coefficients to a UniPolynomial -->
<xsl:template name="vector2UniPolynomial">
   <xsl:param name="vectorString"/>
   <xsl:param name="i" select="0"/>
   <xsl:if test="string-length($vectorString)">
      <xsl:variable name="leadingEntry" select="substring-before(concat($vectorString,' '), ' ')"/>
      <!-- Only add term for non-zero coefficient. -->
      <xsl:if test="$leadingEntry!='0'">
         <xsl:element name="t" namespace="{namespace-uri()}">
            <xsl:value-of select="concat($i,concat(' ',$leadingEntry))"/>
         </xsl:element>
      </xsl:if>
      <!-- Recursive call to self -->
      <xsl:call-template name="vector2UniPolynomial">
         <xsl:with-param name="vectorString" select="substring-after($vectorString, ' ')"/>
         <xsl:with-param name="i" select="$i + 1"/>
      </xsl:call-template>
   </xsl:if>
</xsl:template>


<!-- rename EHRHART_POLYNOMIAL_COEFF to EHRHART_POLYNOMIAL in Polytope -->
<xsl:template match="p:object/p:property[@name='EHRHART_POLYNOMIAL_COEFF']">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">EHRHART_POLYNOMIAL</xsl:attribute>
      <xsl:element name="t" namespace="{namespace-uri()}">
         <xsl:element name="m" namespace="{namespace-uri()}">
            <xsl:call-template name="vector2UniPolynomial">
               <xsl:with-param name="vectorString" select="*"/>
            </xsl:call-template>
         </xsl:element>
      </xsl:element>
   </xsl:element>
</xsl:template>


<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
