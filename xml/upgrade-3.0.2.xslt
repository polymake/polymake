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

   Move necessary properties to new DUAL subobject of matroid
   Also remove undef TERNARY properties, since this can now be computed deterministically.
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

   <xsl:output method="xml" encoding="utf-8" cdata-section-elements="description" indent="yes" />

   <xsl:template match="p:object[@type='matroid::Matroid' or @type='Matroid' or @type='ValuatedMatroid' or @type='matroid::ValuatedMatroid']">
      <xsl:variable name="has_COCIRCUITS" select="./p:property[@name='COCIRCUITS']" />
      <xsl:variable name="has_COLOOPS" select="./p:property[@name='COLOOPS']" />
      <xsl:variable name="has_N_COCIRCUITS" select="./p:property[@name='N_COCIRCUITS']" />
      <xsl:variable name="has_N_COLOOPS" select="./p:property[@name='N_COLOOPS']" />

      <xsl:element name="object" namespace="{namespace-uri()}">
         <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
         <xsl:choose>

            <xsl:when test="(count($has_COCIRCUITS)>=1) or (count($has_COLOOPS)>=1) or (count($has_N_COCIRCUITS)>=1) or (count($has_N_COLOOPS)>=1)">
               <!-- we have dual properties -->

               <!-- most properties stay in the object -->

               <xsl:for-each select="*">
                  <xsl:variable name="nondual-props">
                     <xsl:apply-templates mode="nondual-props" select="." />
                  </xsl:variable>
                  <xsl:if test="($nondual-props='')">
                     <xsl:apply-templates select="." />
                  </xsl:if>   
               </xsl:for-each>

               <!-- copy some properties into the bounded complex object -->

               <xsl:element name="property" namespace="{namespace-uri()}">
                  <xsl:attribute name="name">DUAL</xsl:attribute>
                  <xsl:element name="object" namespace="{namespace-uri()}">
                     <xsl:for-each select="*">
                        <xsl:variable name="nondual-props">
                           <xsl:apply-templates mode="nondual-props" select="." />
                        </xsl:variable>
                        <xsl:if test="($nondual-props!='')">
                           <xsl:element name="property" namespace="{namespace-uri()}">
                              <xsl:attribute name="name">
                                 <xsl:value-of select="$nondual-props" />
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

<!-- These properties will be moved into DUAL  -->

<xsl:template mode="nondual-props" match="p:property[@name='COCIRCUITS']">
   <xsl:text>CIRCUITS</xsl:text>
</xsl:template>

<xsl:template mode="nondual-props" match="p:property[@name='COLOOPS']">
   <xsl:text>LOOPS</xsl:text>
</xsl:template>

<xsl:template mode="nondual-props" match="p:property[@name='N_COCIRCUITS']">
   <xsl:text>N_CIRCUITS</xsl:text>
</xsl:template>

<xsl:template mode="nondual-props" match="p:property[@name='N_COLOOPS']">
   <xsl:text>N_LOOPS</xsl:text>
</xsl:template>
<xsl:template mode="nondual-props" match="*" />

<!-- Removing undef ternary properties -->

<xsl:template match="p:object[@type='matroid::Matroid' or @type='Matroid']//p:property[@name='TERNARY' and @undef='true']"/>

<!-- Only remove undef ternary vectors, if TERNARY was also undef -->

<xsl:template match="p:object[@type='matroid::Matroid' or @type='Matroid' or @type='ValuatedMatroid' or @type='matroid::ValuatedMatroid']//p:property[@name='TERNARY_VECTORS' and @undef='true']">
   <xsl:if test="not(/p:object[@type='matroid::Matroid' or @type='Matroid' or @type='ValuatedMatroid' or @type='matroid::ValuatedMatroid']//p:property[@name='TERNARY' and @undef='true'])">
      <xsl:copy-of select="."/>
   </xsl:if>
</xsl:template>



<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
