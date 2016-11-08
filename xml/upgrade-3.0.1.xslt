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

0. Switch variables in Tutte polynomial to match mainstream notation.
1. All properties of a GROUP, except for CHARACTER_TABLE and ORDER, go into a new section PERMUTATION_ACTION
2. GroupOfPolytope and GroupOfCone get transformed to Groups with RAY_ACTION or FACET_ACTION
3. GENERATING_GROUP gets transformed to RAY_ACTION or FACET_ACTION, depending on GENERATING_GROUP.DOMAIN
4. GENERATING_GROUP: N_ORBITS_OF_INPUT_RAYS, N_ORBITS_OF_RAYS, N_ORBITS_OF_INEQUALITIES, 
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="description" indent="yes" />

   <xsl:template match="p:property[@name='TUTTE_POLYNOMIAL']//p:v[@dim]">
      <xsl:copy>
         <xsl:for-each select="./@*">
            <xsl:copy-of select="."/>
         </xsl:for-each>

         <xsl:for-each select="p:e[@i=1]">
            <xsl:copy>
               <xsl:attribute name="i">
                  <xsl:value-of select="'0'"/>
               </xsl:attribute>
               <xsl:value-of select="."/>
            </xsl:copy>
         </xsl:for-each>
         <xsl:for-each select="p:e[@i=0]">
            <xsl:copy>
               <xsl:attribute name="i">
                  <xsl:value-of select="'1'"/>
               </xsl:attribute>
               <xsl:value-of select="."/>
            </xsl:copy>
         </xsl:for-each>
      </xsl:copy>
   </xsl:template>

   <xsl:template match="p:property[@name='SPLITTING_FLACETS']"/>


<xsl:template match="/p:object[@type='group::Group'] | /p:object[@type='group::GroupOfPolytope'] | /p:object[@type='group::GroupOfCone']">
         <xsl:element name="object" namespace="{namespace-uri(/p:object)}">
            <xsl:attribute name="type">group::Group</xsl:attribute>
            <xsl:copy-of select="/p:object/@*[name()!='tm' and name()!='version' and name()!='type']" />


            <!-- Copy the Group's top properties -->
            <xsl:for-each select="/p:object/*">
               <xsl:variable name="new_name">
                  <xsl:apply-templates mode="Group-props" select="." />
        	  <xsl:apply-templates mode="Group-non_change-props" select="." />
               </xsl:variable>
               <xsl:variable name="keep_in_Group">
                  <xsl:apply-templates mode="keep-in-Group" select="." />
               </xsl:variable>
               <xsl:choose>
                  <xsl:when test="$new_name!=''">
			<xsl:copy-of select="." />
<!--
                        <xsl:apply-templates select="./node()" />
                        <xsl:copy-of select="@*[name()!='name']" />
-->
                  </xsl:when>
                  <xsl:when test="$keep_in_Group!=''">
                     <xsl:copy-of select="." />
                  </xsl:when>
               </xsl:choose>
            </xsl:for-each>

            <xsl:element name="property" namespace="{namespace-uri(/p:object)}">
               <xsl:attribute name="name">PERMUTATION_ACTION</xsl:attribute>
               <xsl:element name="object" namespace="{namespace-uri(/p:object)}">
               <xsl:attribute name="type">group::PermutationAction</xsl:attribute>
                  <!-- copy all children which do not belong to Group -->
                  <xsl:for-each select="/p:object/*">
                     <xsl:variable name="keep_in_Group">
                        <xsl:apply-templates mode="keep-in-Group" select="." />
                     </xsl:variable>
                     <xsl:if test="$keep_in_Group=''">
		       <xsl:copy-of select="." />
                     </xsl:if>
                  </xsl:for-each>
               </xsl:element>
            </xsl:element>

         </xsl:element>


</xsl:template>

<!-- The following group of rules encode the mapping between the old and new property names.
     Every property named here remains in the top-level Group object,
     the rest goes into the PERMUTATION_ACTION subobject, if one the properties occurs, the object
     is translated into a Group-->



<xsl:template mode="Group-props" match="*" />


<!-- The following group of rules encode the mapping between the old and new property names.
     Every property named here remains in the top-level Group object,
     the rest goes into the PERMUTATION_ACTION subobject. -->
     
<xsl:template mode="Group-non_change-props" match="p:property[@name='CHARACTER_TABLE']">
  <xsl:text>CHARACTER_TABLE</xsl:text>
</xsl:template>

<xsl:template mode="Group-non_change-props" match="*" />

<!-- The following group of rules lists the children elements which should stay at the top level.
     Unlike in the first group, they are copied verbatim. -->

<xsl:template mode="keep-in-Group" match="p:property[@name='CHARACTER_TABLE']">Yes</xsl:template>

<xsl:template mode="keep-in-Group" match="p:property[@name='ORDER']">Yes</xsl:template>

<xsl:template mode="keep-in-Group" match="p:property[@name='AMBIENT_DIM']">Yes</xsl:template>

<xsl:template mode="PA-props" match="p:property[@name='GENERATORS']">
   <xsl:text>GENERATORS</xsl:text>
</xsl:template>


<xsl:template mode="keep-in-Group" match="p:description">Yes</xsl:template>

<xsl:template mode="keep-in-Group" match="*">
   <xsl:apply-templates mode="Group-props" select="." />
   <xsl:apply-templates mode="Group-non_change-props" select="." />
</xsl:template>

<!-- 1. -->

<!-- 2. -->

<xsl:template match="//p:object[@type='group::GroupOfPolytope'] | //p:object[@type='group::GroupOfCone']">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:attribute name="type">group::Group</xsl:attribute>
    <xsl:variable name="action" select="./p:property[@name='DOMAIN']/@value"/>
    <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:choose>
	<xsl:when test="$action=1">
	  <xsl:attribute name="name">RAY_ACTION</xsl:attribute>
	</xsl:when>
	<xsl:when test="$action=2">
	  <xsl:attribute name="name">FACET_ACTION</xsl:attribute>
	</xsl:when>
	<xsl:when test="$action=3">
	  <xsl:attribute name="name">COORDINATE_ACTION</xsl:attribute>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:attribute name="name">PERMUTATION_ACTION</xsl:attribute>
	</xsl:otherwise>
      </xsl:choose>
      <xsl:element name="object" namespace="{namespace-uri()}">
	<xsl:attribute name="type">group::PermutationAction</xsl:attribute>

	<!-- copy all children which do not belong to Group -->
	<xsl:for-each select="p:property[@name!='DOMAIN']">
	  <xsl:variable name="keep_in_Group">
	    <xsl:apply-templates mode="keep-in-Group" select="." />
	  </xsl:variable>
	  <xsl:if test="$keep_in_Group=''">
	    <xsl:copy-of select="." />
	  </xsl:if>
	</xsl:for-each>
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template mode="keep-in-Group" match="p:property[@name='CHARACTER_TABLE']">Yes</xsl:template>

<!-- 2. -->

<xsl:include href="trivial-copy.xslt" />


</xsl:transform>
