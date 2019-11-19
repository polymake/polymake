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

<!-- bundled:group related:
  3.0.10:
    rename boost_dynamic_bitset to Bitset
    remove ext attribute for bundled:group (and all corresponding number only attributes)
-->

<!-- fetch id of bundled:group  -->
<xsl:template mode="groupextid" match="*[@ext[contains(string(),'bundled:group')]]">
   <xsl:value-of select="substring(@ext,string-length(substring-before(@ext, '=bundled:group')),1)" />
</xsl:template>

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

<!-- match all attributes ext and type containing boost_dynamic_bitset -->
<xsl:template match="*[@type[contains(string(),'boost_dynamic_bitset')] or @ext]">
   <xsl:element name="{name()}" namespace="{namespace-uri()}">
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="./node()"/>
  </xsl:element>
</xsl:template>

<!-- copy all other attributes by default -->
<xsl:template match="@*">
   <xsl:copy-of select="."/>
</xsl:template>

<!-- except for tm and version -->
<xsl:template match="@tm"/>
<xsl:template match="@version"/>

<!-- now adjust type -->
<xsl:template match="@type">
   <xsl:attribute name="type">
      <xsl:call-template name="string-replace-all">
         <xsl:with-param name="text" select="." />
         <xsl:with-param name="replace" select="'boost_dynamic_bitset'" />
         <xsl:with-param name="by" select="'Bitset'" />
      </xsl:call-template> 
   </xsl:attribute>
</xsl:template>

<!-- remove <id>=bundled:group from ext attribute and ext="<id>" as well for the correct id -->
<xsl:template match="@ext">
   <xsl:variable name="extid">
      <xsl:apply-templates mode="groupextid" select="/p:*" />
   </xsl:variable>
   <xsl:if test=".!=$extid and .!=concat($extid,'=bundled:group')">
      <xsl:attribute name="ext">
         <xsl:variable name="extstring">
            <xsl:call-template name="string-replace-all">
               <xsl:with-param name="text" select="." />
               <xsl:with-param name="replace" select="concat($extid,'=bundled:group')" />
               <xsl:with-param name="by" select="''" />
            </xsl:call-template> 
         </xsl:variable>
         <xsl:value-of select="normalize-space($extstring)"/>
      </xsl:attribute>
   </xsl:if>
</xsl:template>

<!-- upgrade groups as followup to upgrade-3.0.5.xslt (old -3.0.1.xslt)
  3.0.10:
    rename OrbitPolytope, SymmetricPolytope to Polytope ; SymmetricCone to Cone
    rename GENERATING_GROUP to GROUP (merge into one GROUP with multiple objects if necessary)
    move GEN_ and various other OrbitPolytope properties into GROUP.COORDINATE_ACTION (the one from GENERATING_GROUP if there are multiple groups)
    move SYMMETRIC_{FACETS,RAYS,VERTICES} from action to group
    store a few removed properties in attachments (orbits...)
-->

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

<!-- replace OrbitPolytope / SymmetricPolytope / SymmetricCone, it might be object or data (big object array) -->
<xsl:template match="p:*[@type[contains(string(),'OrbitPolytope')]]">
   <xsl:call-template name="replace-object-type">
      <xsl:with-param name="old_type" select="'OrbitPolytope'" />
      <xsl:with-param name="new_type" select="'Polytope'" />
   </xsl:call-template>
</xsl:template>
<xsl:template match="p:*[@type[contains(string(),'SymmetricPolytope')]]">
   <xsl:call-template name="replace-object-type">
      <xsl:with-param name="old_type" select="'SymmetricPolytope'" />
      <xsl:with-param name="new_type" select="'Polytope'" />
   </xsl:call-template>
</xsl:template>
<xsl:template match="p:*[@type[contains(string(),'SymmetricCone')]]">
   <xsl:call-template name="replace-object-type">
      <xsl:with-param name="old_type" select="'SymmetricCone'" />
      <xsl:with-param name="new_type" select="'Cone'" />
   </xsl:call-template>
</xsl:template>

<!-- OrbitPolytope properties are moved into the corresponding action -->
<xsl:template match="p:object/p:property[@name[contains(string(),'_ACTION')]]" mode="gengroup">
   <xsl:variable name="genpoint" namespace="{namespace-uri()}">
      <xsl:choose>
         <xsl:when test="../../../p:property[@name='GEN_POINTS']">
            <xsl:element name="property" namespace="{namespace-uri()}">
               <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
               <xsl:attribute name="name">POINTS_GENERATORS</xsl:attribute>
               <xsl:copy-of select="../../../p:property[@name='GEN_POINTS']/*"/>
            </xsl:element>
         </xsl:when>
         <xsl:otherwise>
            <xsl:if test="../../../p:property[@name='GEN_POINT']">
               <xsl:element name="property" namespace="{namespace-uri()}">
                  <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
                  <xsl:attribute name="name">POINTS_GENERATORS</xsl:attribute>
                  <xsl:element name="m" namespace="{namespace-uri()}">
                     <xsl:copy-of select="../../../p:property[@name='GEN_POINT']/*"/>
                  </xsl:element>
               </xsl:element>
            </xsl:if>
         </xsl:otherwise>
         <xsl:if test="../../../p:property[@name='GEN_EQUATIONS']">
            <xsl:element name="property" namespace="{namespace-uri()}">
               <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
               <xsl:attribute name="name">EQUATIONS_GENERATORS</xsl:attribute>
               <xsl:element name="m" namespace="{namespace-uri()}">
                  <xsl:copy-of select="../../../p:property[@name='GEN_EQUATIONS']/*"/>
               </xsl:element>
            </xsl:element>
         </xsl:if>
         <xsl:if test="../../../p:property[@name='GEN_INEQUALITIES']">
            <xsl:element name="property" namespace="{namespace-uri()}">
               <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
               <xsl:attribute name="name">INEQUALITIES_GENERATORS</xsl:attribute>
               <xsl:element name="m" namespace="{namespace-uri()}">
                  <xsl:copy-of select="../../../p:property[@name='GEN_INEQUALITIES']/*"/>
               </xsl:element>
            </xsl:element>
         </xsl:if>
         <xsl:if test="../../../p:property[@name='GEN_INPUT_RAYS']">
            <xsl:element name="property" namespace="{namespace-uri()}">
               <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
               <xsl:attribute name="name">INPUT_RAYS_GENERATORS</xsl:attribute>
               <xsl:element name="m" namespace="{namespace-uri()}">
                  <xsl:copy-of select="../../../p:property[@name='GEN_INPUT_RAYS']/*"/>
               </xsl:element>
            </xsl:element>
         </xsl:if>
         <xsl:if test="../../../p:property[@name='GEN_INPUT_LINEALITY']">
            <xsl:element name="property" namespace="{namespace-uri()}">
               <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
               <xsl:attribute name="name">INPUT_LINEALITY_GENERATORS</xsl:attribute>
               <xsl:element name="m" namespace="{namespace-uri()}">
                  <xsl:copy-of select="../../../p:property[@name='GEN_INPUT_LINEALITY']/*"/>
               </xsl:element>
            </xsl:element>
         </xsl:if>

      </xsl:choose>
      <xsl:copy-of select="../../../p:property[@name='NOP_GRAPH' or @name='REPRESENTATIVE_CERTIFIERS' or @name='N_REPRESENTATIVE_CERTIFIERS' or @name='CP_INDICES' or @name='REPRESENTATIVE_CORE_POINTS' or @name='N_REPRESENTATIVE_CORE_POINTS']"/>
   </xsl:variable>
   <xsl:element name="object" namespace="{namespace-uri()}">
      <xsl:if test="not(../@name)">
         <xsl:attribute name="name">generating_group</xsl:attribute>
      </xsl:if>
      <xsl:copy-of select="../@*[name()!='tm' and name()!='version']" />
      <xsl:element name="property" namespace="{namespace-uri()}">
         <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
         <xsl:element name="object" namespace="{namespace-uri()}">
            <xsl:copy-of select="./p:object/@*[name()!='tm' and name()!='version']" />
            <xsl:apply-templates select="./p:object/node()" />
            <xsl:if test="count($genpoint)>=1">
               <xsl:copy-of select="$genpoint"/>
            </xsl:if>
         </xsl:element>
      </xsl:element>
      <xsl:if test="./p:object/p:property[@name='SYMMETRIC_RAYS' or @name='SYMMETRIC_FACETS' or @name='SYMMETRIC_VERTICES']">
         <xsl:copy-of select="./p:object/p:property[@name='SYMMETRIC_RAYS' or @name='SYMMETRIC_FACETS' or @name='SYMMETRIC_VERTICES']"/>
      </xsl:if>
   </xsl:element>
</xsl:template>

<!-- move SYMMETRIC_*  from action to group -->
<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]">
   <xsl:if test="./p:object/p:property[@name='SYMMETRIC_RAYS' or @name='SYMMETRIC_FACETS' or @name='SYMMETRIC_VERTICES']">
      <xsl:copy-of select="./p:object/p:property[@name='SYMMETRIC_RAYS' or @name='SYMMETRIC_FACETS' or @name='SYMMETRIC_VERTICES']"/>
   </xsl:if>
   <xsl:element name="property" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
      <xsl:apply-templates select="./node()" />
   </xsl:element>
</xsl:template>

<!-- merge GENERATING_GROUP and GROUP into one GROUP property with multiple objects 
     further make sure the additional OrbitPolytope stuff ends up in the one from GENERATING_GROUP
     -->
<xsl:template match="p:property[@name='GROUP' or @name='GENERATING_GROUP']">
   <xsl:variable name="gengroup">
      <xsl:if test="../p:property[@name='GENERATING_GROUP']/p:object">
         <xsl:value-of select="'yes'"/>
      </xsl:if>
   </xsl:variable>
   <xsl:variable name="group">
      <xsl:if test="../p:property[@name='GROUP']/p:object">
         <xsl:value-of select="'yes'"/>
      </xsl:if>
   </xsl:variable>
   <xsl:if test="$group!='yes' or @name='GROUP'">
      <xsl:element name="property" namespace="{namespace-uri()}">
         <xsl:copy-of select="@*[name()!='tm' and name()!='version' and name()!='name']" />
         <xsl:attribute name="name">GROUP</xsl:attribute>
         <xsl:choose>
            <xsl:when test="$gengroup and @name='GROUP'">
               <xsl:apply-templates select="../p:property[@name='GENERATING_GROUP']/p:object" mode="gengroup"/>
               <xsl:apply-templates select="../p:property[@name='GROUP']/p:object"/>
            </xsl:when>
            <xsl:otherwise>
               <xsl:apply-templates select="./p:object" mode="gengroup"/>
            </xsl:otherwise>
         </xsl:choose>
      </xsl:element>
   </xsl:if>
</xsl:template>

<!-- remove various properties that have been dealt with -->
<xsl:template match="p:object[@type[contains(string(),'Polytope') or contains(string(),'Cone')]]/p:property[@name='GEN_POINT' or @name='GEN_POINTS' or @name='GEN_INPUT_RAYS' or @name='GEN_INPUT_LINEALITY' or @name='GEN_EQUATIONS' or @name='GEN_INEQUALITIES' or @name='NOP_GRAPH' or @name='REPRESENTATIVE_CERTIFIERS' or @name='N_REPRESENTATIVE_CERTIFIERS' or @name='CP_INDICES' or @name='REPRESENTATIVE_CORE_POINTS' or @name='N_REPRESENTATIVE_CORE_POINTS']"/>

<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]/p:object/p:property[@name='SYMMETRIC_FACETS' or @name='SYMMETRIC_RAYS' or @name='SYMMETRIC_VERTICES']"/>

<xsl:template match="p:property[@name[contains(string(),'_ACTION')]]/p:object/p:property[@name[contains(string(),'N_ORBITS_OF_')]]"/>

<!-- _in_orbits is now an attachment, the new ORBITS property will be recomputed -->
<xsl:template match="p:property[@name[contains(string(),'_IN_ORBITS')]]">
	<xsl:element name="attachment" namespace="{namespace-uri()}">
		<xsl:attribute name="name">
			<xsl:value-of select="@name"/>
		</xsl:attribute>
      <xsl:attribute name="type">Array&lt;Set&lt;Int&gt;&gt;</xsl:attribute>
		<xsl:copy-of select="./node()" />
	</xsl:element>
</xsl:template>

<xsl:template match="p:object[@type[contains(string(),'SubdivisionOfPoints')]]/p:property[@name='REFINED_SPLITS']"/>

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
