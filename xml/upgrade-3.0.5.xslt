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

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="description" indent="yes" />

<!-- matroid first:
  3.0.1:
    Switch variables in Tutte polynomial to match mainstream notation.
  3.0.2:
    Move necessary properties to new DUAL subobject of matroid
    Also remove undef TERNARY properties, since this can now be computed deterministically.
    Remove SPLITTING_FLACETS
  3.0.4:
    we keep the section removing HasDual / HasNoDual even though
    it should not occur as it was introduced after 3.0:
      Remove the template parameter HasDual/HasNoDual from Matroid objects.
  3.0.5:
    Renaming of SPLITTING_FLACETS and SPLIT_COMPATIBLE to SPLIT_FLACETS and SPLIT
    (the first rename is removed as this property is removed in 3.0.2)
    Removes property SPLIT_COMPATIBLE in some cases. This is due a change in the definition.

-->

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
                     <xsl:apply-templates select="." mode="matroid" />
                  </xsl:if>   
               </xsl:for-each>

               <!-- copy some properties into the dual object -->

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
               <xsl:apply-templates select="./node()" mode="matroid"/>
            </xsl:otherwise>


         </xsl:choose>
      </xsl:element>

   </xsl:template>

   <!-- this is not in matroid mode as it is a subnode of the property -->
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

   <xsl:template match="p:property[@name='SPLITTING_FLACETS']" mode="matroid"/>

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

<xsl:template match="p:property[@name='TERNARY' and @undef='true']" mode="matroid"/>

<!-- Only remove undef ternary vectors, if TERNARY was also undef -->

<xsl:template match="p:property[@name='TERNARY_VECTORS' and @undef='true']" mode="matroid">
   <xsl:if test="not(../p:property[@name='TERNARY' and @undef='true'])">
      <xsl:copy-of select="."/>
   </xsl:if>
</xsl:template>

<!--
  Remove the template parameter HasDual/HasNoDual from Matroid objects.
  (this should not be necessary but is kept as it does not hurt)
-->

<xsl:template match="p:object[@type='matroid::Matroid&lt;HasDual&gt;' or @type='matroid::Matroid&lt;HasNoDual&gt;']">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='tm' and name()!='version']" />
    <xsl:attribute name="type">matroid::Matroid</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="matroid" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:object[@type='Matroid&lt;HasDual&gt;' or @type='Matroid&lt;HasNoDual&gt;']">
  <xsl:element name="{name()}" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type']" />
    <xsl:attribute name="type">Matroid</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="matroid" />
  </xsl:element>
</xsl:template>

<!--
  Renaming of SPLITTING_FLACETS and SPLIT_COMPATIBLE to SPLIT_FLACETS and SPLIT
  Removes property SPLIT_COMPATIBLE in some cases. This is due a change in the definition.
-->
<!-- The rename is removed as SPLITTING_FLACETS is removed earlier (by 3.0.2)
<xsl:template match="p:property[@name='SPLITTING_FLACETS']" mode="matroid">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">SPLIT_FLACETS</xsl:attribute>
      <xsl:copy-of select="*" />
  </xsl:element>
</xsl:template>
-->
<xsl:template match="p:property[@name='SPLIT_COMPATIBLE']" mode="matroid">
   <xsl:if test="../p:property[@name='CONNECTED' and @value='true'] or ../p:property[@name='SPLIT_COMPATIBLE' and @value='false']">
      <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">SPLIT</xsl:attribute>
      <xsl:copy-of select="*" />
      </xsl:element>
   </xsl:if>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="matroid">
  <xsl:apply-templates select="." />
</xsl:template>

<!-- groups
  3.0.1:
    1. All properties of a GROUP, except for CHARACTER_TABLE and ORDER, go into a new section PERMUTATION_ACTION
    2. GroupOfPolytope and GroupOfCone get transformed to Groups with RAY_ACTION or FACET_ACTION
    3. GENERATING_GROUP gets transformed to RAY_ACTION or FACET_ACTION, depending on GENERATING_GROUP.DOMAIN
    4. GENERATING_GROUP: N_ORBITS_OF_INPUT_RAYS, N_ORBITS_OF_RAYS, N_ORBITS_OF_INEQUALITIES, 
-->

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

<xsl:template match="//p:object[@type='group::GroupOfPolytope'] | //p:object[@type='group::GroupOfCone'] | //p:object[@type[contains(string(),'Polytope') or contains(string(),'Cone') or contains(string(),'PointConfiguration')]]/p:property[@name='GROUP']/p:object | //p:object[@type[contains(string(),'QuotientSpace')]]/p:property[@name='SYMMETRY_GROUP']/p:object | //p:property[@name='QUOTIENT_SPACE']/p:object/p:property[@name='SYMMETRY_GROUP']/p:object ">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:attribute name="type">group::Group</xsl:attribute>
    <xsl:variable name="action" select="./p:property[@name='DOMAIN']/@value"/>
    <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:choose>
	<xsl:when test="$action=1">
	  <xsl:attribute name="name">RAYS_ACTION</xsl:attribute>
	</xsl:when>
	<xsl:when test="$action=2">
	  <xsl:attribute name="name">FACETS_ACTION</xsl:attribute>
	</xsl:when>
	<xsl:when test="$action=3">
	  <xsl:attribute name="name">HOMOGENEOUS_COORDINATE_ACTION</xsl:attribute>
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

<xsl:template mode="keep-in-Group" match="p:property[@name='CONJUGACY_CLASS_SIZES']">Yes</xsl:template>

<xsl:template mode="keep-in-Group" match="p:property[@name='REPRESENTATIVE_FACETS']">Yes</xsl:template>
<xsl:template mode="keep-in-Group" match="p:property[@name='REPRESENTATIVE_VERTICES']">Yes</xsl:template>
<xsl:template mode="keep-in-Group" match="p:property[@name='REPRESENTATIVE_RAYS']">Yes</xsl:template>
<xsl:template mode="keep-in-Group" match="p:property[@name='REPRESENTATIVE_AFFINE_HULL']">Yes</xsl:template>

<xsl:template match="p:property[@name='IDENTIFICATION_GROUP']">
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*" />
      <xsl:attribute name="name">IDENTIFICATION_ACTION</xsl:attribute>
      <xsl:element name="object" namespace="{namespace-uri(/p:object)}">
         <xsl:copy-of select="p:object/@*" />
         <xsl:copy-of select="*[@name!='ORDER']" />
      </xsl:element>
  </xsl:element>
</xsl:template>

<!--
  3.0.3:
    Insert a "cols" attribute in empty coordinate matrices like AFFINE_HULL or LINEALITY_SPACE
    wherever possible; remove ones when no dimension property is at hand
-->

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
