<?xml version="1.0"?>

<!--
  Copyright (c) 1997-2015
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

  BOUNDED_COMPLEX as a subobject.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />


<!-- 1 -->

<!-- We search for objects with properties of the bounded complex -->

<xsl:template match="p:object">
   <xsl:variable name="has_BOUNDED_COMPLEX" select="./p:property[@name='BOUNDED_COMPLEX']" />
   <xsl:variable name="has_BOUNDED_GRAPH" select="./p:property[@name='BOUNDED_GRAPH']" />
   <xsl:variable name="has_BOUNDED_DUAL_GRAPH" select="./p:property[@name='BOUNDED_DUAL_GRAPH']" />
   <xsl:variable name="has_BOUNDED_HASSE_DIAGRAM" select="./p:property[@name='BOUNDED_HASSE_DIAGRAM']" />
   <xsl:variable name="has_BOUNDED_F_VECTOR" select="./p:property[@name='BOUNDED_HASSE_DIAGRAM']" />
   <xsl:variable name="has_BOUNDED_DIM" select="./p:property[@name='BOUNDED_DIM']" />

   
    <xsl:element name="object" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
      <xsl:choose>

        <xsl:when test="(count($has_BOUNDED_COMPLEX)>=1) or (count($has_BOUNDED_HASSE_DIAGRAM)>=1) or (count($has_BOUNDED_GRAPH)>=1) or (count($has_BOUNDED_DUAL_GRAPH)>=1)or (count($has_BOUNDED_F_VECTOR)>=1) or (count($has_BOUNDED_DIM)>=1)">
            <!-- we have a BOUNDED_COMPLEX-->

	    <!-- most properties stay in the object -->
	
	    <xsl:for-each select="*">
              <xsl:variable name="PD-props">
                <xsl:apply-templates mode="PD-props" select="." />
              </xsl:variable>
              <xsl:if test="($PD-props='')">
                <xsl:apply-templates select="." />
              </xsl:if>   
	    </xsl:for-each>
	    
	    <!-- copy some properties into the bounded complex object -->
	    	    
	    <xsl:element name="property" namespace="{namespace-uri()}">
              <xsl:attribute name="name">fan::BOUNDED_COMPLEX</xsl:attribute>
              <xsl:element name="object" namespace="{namespace-uri()}">
                <xsl:for-each select="*">
                  <xsl:variable name="PD-props">
                    <xsl:apply-templates mode="PD-props" select="." />
                  </xsl:variable>
                  <xsl:if test="($PD-props!='')">
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

        </xsl:when>

        <xsl:otherwise>
          <xsl:apply-templates  />
        </xsl:otherwise>


     </xsl:choose>
  </xsl:element>
</xsl:template>



<!-- These properties will be moved into BOUNDED_COMPLEX  -->

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_COMPLEX']">
   <xsl:text>MAXIMAL_POLYTOPES</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_GRAPH']">
   <xsl:text>GRAPH</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_DUAL_GRAPH']">
   <xsl:text>DUAL_GRAPH</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_F_VECTOR']">
   <xsl:text>F_VECTOR</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_DIM']">
   <xsl:text>COMBINATORIAL_DIM</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="p:property[@name='BOUNDED_HASSE_DIAGRAM']">
   <xsl:text>HASSE_DIAGRAM</xsl:text>
</xsl:template>

<xsl:template mode="PD-props" match="*" />



<xsl:include href="trivial-copy.xslt" />


</xsl:transform>
