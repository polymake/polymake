<?xml version="1.0"?>

<!--
  Copyright (c) 1997-2018
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

<xsl:transform version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                             xmlns:libxslt="http://xmlsoft.org/XSLT/namespace"
                             xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />

<!--
	rename SymmetricFan to PolyhedralFan
	rename MATRIX_GROUP_ACTION to MATRIX_ACTION
	move REPRESENTATIVE_*, SIMPLEXITY_LOWER_BOUND from action to group
	rename EQUATIONS to PROJECTED_EQUATIONS in SymmetrizedCocircuitEquations
	remove property REPRESENTATIVE_SIMPLICES
	fix wrong handling of COORDINATE_ACTION in upgrade-3.0.5
	replace PermutationActionOnSets by group::PermutationActionOnSets
-->

<!-- replace all in string -->
<xsl:template name="string-replace-all">
  <xsl:param name="text" />
  <xsl:param name="replace" />
  <xsl:param name="by" />
  <xsl:choose>
    <xsl:when test="contains($text, $replace)">
      <xsl:value-of select="substring-before($text,$replace)" />
      <xsl:value-of select="$by" />
      <xsl:call-template name="string-replace-all">
        <xsl:with-param name="text" select="substring-after($text,$replace)" />
        <xsl:with-param name="replace" select="$replace" />
        <xsl:with-param name="by" select="$by" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$text" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="replace-object-type">
   <xsl:param name="old_type"/>
   <xsl:param name="new_type"/>
   <xsl:element name="{name()}" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='type']" />
      <xsl:attribute name="type">
         <xsl:call-template name="string-replace-all">
            <xsl:with-param name="text" select="@type" />
            <xsl:with-param name="replace" select="$old_type" />
            <xsl:with-param name="by" select="$new_type" />
         </xsl:call-template> 
      </xsl:attribute>
      <xsl:apply-templates select="./node()" />
   </xsl:element>
</xsl:template>

<xsl:template name="replace-object-name">
   <xsl:param name="old_name"/>
   <xsl:param name="new_name"/>
   <xsl:element name="{name()}" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
      <xsl:attribute name="name">
         <xsl:call-template name="string-replace-all">
            <xsl:with-param name="text" select="@name" />
            <xsl:with-param name="replace" select="$old_name" />
            <xsl:with-param name="by" select="$new_name" />
         </xsl:call-template> 
      </xsl:attribute>
      <xsl:apply-templates select="./node()" />
   </xsl:element>
</xsl:template>

<!-- replace SymmetricFan by PolyhedralFan, it might be object or data (big object array) -->
<xsl:template match="p:*[@type[contains(string(),'SymmetricFan')]]">
   <xsl:call-template name="replace-object-type">
      <xsl:with-param name="old_type" select="'SymmetricFan'" />
      <xsl:with-param name="new_type" select="'PolyhedralFan'" />
   </xsl:call-template>
</xsl:template>

<!-- replace MATRIX_GROUP_ACTION by MATRIX_ACTION, it might be object or data (big object array) -->
<xsl:template match="p:*[@name[contains(string(),'MATRIX_GROUP_ACTION')]]">
   <xsl:call-template name="replace-object-name">
      <xsl:with-param name="old_name" select="'MATRIX_GROUP_ACTION'" />
      <xsl:with-param name="new_name" select="'MATRIX_ACTION'" />
   </xsl:call-template>
</xsl:template>

<!-- replace matrix_group_action by matrix_action, it might be object or data (big object array) -->
<xsl:template match="p:*[@name[contains(string(),'matrix_group_action')]]">
   <xsl:call-template name="replace-object-name">
      <xsl:with-param name="old_name" select="'matrix_group_action'" />
      <xsl:with-param name="new_name" select="'matrix_action'" />
   </xsl:call-template>
</xsl:template>

<!-- move REPRESENTATIVE_*, SIMPLEXITY_LOWER_BOUND from action to group -->

<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]">
  <xsl:if test="./p:object/p:property[@name='polytope::REPRESENTATIVE_MAX_INTERIOR_SIMPLICES' or @name='polytope::REPRESENTATIVE_INTERIOR_RIDGE_SIMPLICES' or @name='polytope::REPRESENTATIVE_MAX_BOUNDARY_SIMPLICES' or @name='polytope::SIMPLEXITY_LOWER_BOUND']">
    <xsl:copy-of select="./p:object/p:property[@name='polytope::REPRESENTATIVE_MAX_INTERIOR_SIMPLICES' or @name='polytope::REPRESENTATIVE_INTERIOR_RIDGE_SIMPLICES' or @name='polytope::REPRESENTATIVE_MAX_BOUNDARY_SIMPLICES' or @name='polytope::SIMPLEXITY_LOWER_BOUND']"/>
  </xsl:if>
  <xsl:element name="property" namespace="{namespace-uri()}">    
    <xsl:copy-of select="@*" />
    <xsl:apply-templates select="./node()" />
  </xsl:element>
</xsl:template>

<!-- ... and remove it from action -->

<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]/p:object/p:property[@name='polytope::REPRESENTATIVE_MAX_INTERIOR_SIMPLICES' or @name='polytope::REPRESENTATIVE_INTERIOR_RIDGE_SIMPLICES' or @name='polytope::REPRESENTATIVE_MAX_BOUNDARY_SIMPLICES' or @name='polytope::SIMPLEXITY_LOWER_BOUND']"/>

<!-- rename EQUATIONS to PROJECTED_EQUATIONS in SymmetrizedCocircuitEquations -->

<xsl:template match="p:object/p:property[@name='polytope::SYMMETRIZED_COCIRCUIT_EQUATIONS']/p:object/p:property[@name='EQUATIONS']">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*" />
    <xsl:attribute name="name">PROJECTED_EQUATIONS</xsl:attribute>
    <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>

<!-- insert default ISOTYPIC_COMPONENTS for old versions of SYMMETRIZED_COCIRCUIT_EQUATIONS -->

<xsl:template match="p:object/p:property[@name='polytope::SYMMETRIZED_COCIRCUIT_EQUATIONS']/p:m">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:attribute name="name">ISOTYPIC_COMPONENTS</xsl:attribute>
      <xsl:element name="v" namespace="{namespace-uri()}">
	<xsl:text>0</xsl:text>
      </xsl:element>
    </xsl:element>
    <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:attribute name="name">RIDGES</xsl:attribute>
      <xsl:copy-of select="//p:property[@name='polytope::REPRESENTATIVE_INTERIOR_RIDGE_SIMPLICES']/p:m" />
    </xsl:element>
    <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:attribute name="name">PROJECTED_EQUATIONS</xsl:attribute>
      <xsl:element name="m" namespace="{namespace-uri()}">
	<xsl:copy-of select="@*" />
	<xsl:copy-of select="*" />
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template>

<!-- remove property REPRESENTATIVE_SIMPLICES -->

<xsl:template match="p:property[@name='polytope::REPRESENTATIVE_SIMPLICES']" />
<xsl:template match="p:property[@name='REPRESENTATIVE_SIMPLICES']" />

<!-- remove property *ACTION.COCIRCUIT_EQUATIONS -->

<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]/p:object/p:property[@name='COCIRCUIT_EQUATIONS']" />

<!-- fix wrong handling of COORDINATE_ACTION in upgrade-3.0.5 -->

<xsl:template match="p:property[@name='GROUP']/p:object/p:property[@name='PERMUTATION_ACTION']">
  <xsl:apply-templates select="./p:object/p:property[@name[contains(string(),'_ACTION')]]" />
</xsl:template>

<!-- replace PermutationActionOnSets by group::PermutationActionOnSets -->

<xsl:template match="p:*[@type='PermutationActionOnSets']">
   <xsl:call-template name="replace-object-type">
     <xsl:with-param name="old_type" select="'PermutationActionOnSets'" />
     <xsl:with-param name="new_type" select="'group::PermutationActionOnSets'" />
   </xsl:call-template>
</xsl:template>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
