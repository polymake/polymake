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
-->

<!-- A collection of operations on list values -->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<!-- add a number to all elements of a list -->
<xsl:template name="add-constant-to-list">
  <xsl:param name="values"/>
  <xsl:param name="constant"/>
  <xsl:variable name="n_values" select="normalize-space($values)"/>
  <xsl:variable name="first" select="substring-before($n_values,' ')" />
  <xsl:variable name="remaining" select="substring-after($n_values, ' ')" />
  <xsl:choose>
    <xsl:when test="$first != ''">
      <xsl:value-of select="$first + ($constant)" />
      <xsl:if test="$remaining != ''">
        <xsl:text> </xsl:text>
        <xsl:call-template name="add-constant-to-list">
          <xsl:with-param name="values" select="$remaining"/>
          <xsl:with-param name="constant" select="$constant"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$n_values != ''">
        <xsl:value-of select="$n_values + ($constant)" />
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose> 
</xsl:template>

</xsl:transform>
