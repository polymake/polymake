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

<!-- tropical:
  3.0.6:
    Rename tropical::Cone to tropical::Polytope
    Rename its properties CONE_COVECTOR_DECOMPOSITION, CONE_MAXIMAL_COVECTORS, CONE_MAXIMAL_COVECTOR_CELLS to POLYTOPE_...
-->

<!-- Rename properties of tropical cone -->

<xsl:template match="p:property[@name='CONE_MAXIMAL_COVECTORS']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_MAXIMAL_COVECTORS</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='CONE_MAXIMAL_COVECTOR_CELLS']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_MAXIMAL_COVECTOR_CELLS</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='CONE_COVECTOR_DECOMPOSITION']" mode="tropical">
  <xsl:element name="property" namespace="{namespace-uri()}">
    <xsl:attribute name="name">POLYTOPE_COVECTOR_DECOMPOSITION</xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<!-- Rename cone itself, but keep template parameter! -->

<xsl:template match="p:object[@type[contains(string(),'tropical::Cone')]]">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='version']" />
    <xsl:attribute name="type">
      <xsl:value-of select="concat('tropical::Polytope&lt;',substring-after(@type,'&lt;'))"/>
    </xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tropical" />
  </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="tropical">
  <xsl:apply-templates select="." />
</xsl:template>


<!-- matroids again:
  3.0.7:
    This corrects the faulty DIMS array in LATTICE_OF_FLATS: The last entry and any leading 0 are removed.
-->

<!-- Strips the last entry of the DIMS vector (but keeps the space before it) -->
<xsl:template name="stripLast">
   <xsl:param name="pText"/>
   <xsl:if test="contains($pText, ' ')">
      <xsl:value-of select="concat(substring-before($pText, ' '),' ')"/>
      <xsl:call-template name="stripLast">
         <xsl:with-param name="pText" select=
            "substring-after($pText, ' ')"/>
      </xsl:call-template>
   </xsl:if>
</xsl:template>

<xsl:template match="p:property[@name='LATTICE_OF_FLATS']/p:object/p:property[@name='DIMS']/p:v/text()">
   <!-- First remove last entry using recursive template above -->
   <xsl:variable name="strip_back">
      <xsl:call-template name="stripLast">
         <xsl:with-param name="pText" select="current()"/>
      </xsl:call-template>
   </xsl:variable>
   <!-- Remove the trailing whitespace -->
   <xsl:variable name="strip_and_trim">
      <xsl:value-of select="substring($strip_back,0,string-length($strip_back))"/>
   </xsl:variable>
   <!-- Remove a leading 0 if it exists -->
   <xsl:choose>
      <xsl:when test="starts-with($strip_and_trim,'0 ')">
         <xsl:value-of select="substring-after($strip_and_trim,'0 ')"/>
      </xsl:when>
      <xsl:otherwise>
         <xsl:value-of select="$strip_and_trim"/>
      </xsl:otherwise>
   </xsl:choose>
</xsl:template>

<!--
  3.0.7:
    Rename FaceLattice to Lattice<BasicDecoration>
    (with a slightly modified replace-object-type template
-->
<xsl:template match="p:*[@type[contains(string(),'FaceLattice')]]">
   <xsl:call-template name="replace-object-type">
    <xsl:with-param name="old_type">FaceLattice</xsl:with-param>
    <xsl:with-param name="new_type">Lattice&lt;BasicDecoration&gt;</xsl:with-param>
  </xsl:call-template>
</xsl:template>


<!-- Rename TightSpan to Polytope
  3.0.8:
    Rename TightSpan itself, but keep template parameter!
    Move all additional properties into attachments. Need separate templates to specify type
-->

<!-- Rename TightSpan itself, but keep template parameter! -->

<xsl:template match="p:object[@type[contains(string(),'TightSpan')]]">
  <xsl:element name="object" namespace="{namespace-uri()}">
    <xsl:copy-of select="@*[name()!='type' and name()!='version']" />
    <xsl:attribute name="type">
      <xsl:value-of select="concat('polytope::Polytope&lt;',substring-after(@type,'&lt;'))"/>
    </xsl:attribute>
    <xsl:apply-templates select="./node()" mode="tightspan" />
  </xsl:element>
</xsl:template>

<!-- Move all additional properties into attachments. Need separate templates to specify type -->

<xsl:template match="p:property[@name='METRIC']" mode="tightspan">
        <xsl:element name="attachment" namespace="{namespace-uri()}">
                <xsl:attribute name="name">
                        <xsl:value-of select="@name"/>
                </xsl:attribute>
                <xsl:attribute name="type">
                        <xsl:value-of select="concat('Matrix&lt;',substring-after(../@type,'&lt;'))"/>
                </xsl:attribute>
                <xsl:copy-of select="./node()" />
        </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='COHERENT_SPLITS']" mode="tightspan">
        <xsl:element name="attachment" namespace="{namespace-uri()}">
                <xsl:attribute name="name">
                        <xsl:value-of select="@name"/>
                </xsl:attribute>
                <xsl:attribute name="type">Array&lt;Pair&lt;Set,Set&gt;&gt;</xsl:attribute>
                <xsl:copy-of select="./node()" />
        </xsl:element>
</xsl:template>

<xsl:template match="p:property[@name='TAXA']" mode="tightspan">
        <xsl:element name="attachment" namespace="{namespace-uri()}">
                <xsl:attribute name="name">
                        <xsl:value-of select="@name"/>
                </xsl:attribute>
                <xsl:attribute name="type">Array&lt;String&gt;</xsl:attribute>
                <xsl:copy-of select="./node()" />
        </xsl:element>
</xsl:template>


<xsl:template match="p:property[@name='VERTICES_IN_METRIC']" mode="tightspan">
        <xsl:element name="attachment" namespace="{namespace-uri()}">
                <xsl:attribute name="name">
                        <xsl:value-of select="@name"/>
                </xsl:attribute>
                <xsl:attribute name="type">Array&lt;Int&gt;</xsl:attribute>
                <xsl:copy-of select="./node()" />
        </xsl:element>
</xsl:template>

<!-- everything else is copied verbatim by switching back to the default mode -->
<xsl:template match="*" mode="tightspan">
  <xsl:apply-templates select="." />
</xsl:template>


<!-- rings / polynomials:
  3.0.9:
    Takes care of migration of old polynomials to new ones:
    - Replace the tag describing the ring by a simple <e> tag containing the number of variables
    - Remove property RING from Ideal
-->

<!-- Builds a node set of id-annotated <e> tags for each ring with an id -->
<xsl:variable name="ring_sizes">
  <xsl:call-template name="calc_ring_sizes">
    <xsl:with-param name="elems" select="//p:t[@id != '']" />
  </xsl:call-template>
</xsl:variable>

<!-- For each ring definition, this creates an id-annotated <e> tag with the number of variables -->
<xsl:template name="calc_ring_sizes">
  <xsl:param name="elems"/>
  <xsl:for-each select="$elems">
    <xsl:element name="e" namespace="{namespace-uri()}">
      <xsl:attribute name="id"><xsl:value-of select="@id" /></xsl:attribute>
      <xsl:call-template name="count_words">
        <xsl:with-param name="str" select="normalize-space(./p:v)" />
      </xsl:call-template>
    </xsl:element>
  </xsl:for-each>
</xsl:template>

<!-- Recursive template used to obtain the number of elements in a whitespace-separated list -->
<xsl:template name="count_words">
  <xsl:param name="str"/>
  <xsl:choose>
    <xsl:when test="contains($str, ' ')">
      <xsl:variable name="count_tail">
        <xsl:call-template name="count_words">
          <xsl:with-param name="str" select="substring-after($str, ' ')"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="1+$count_tail" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="1" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Builds the <e> node containing the number of variables -->
<xsl:template mode="fetch_ring_size" match="/">
  <xsl:param name="ring_id" />
  <xsl:for-each select="p:e[@id=$ring_id]">
    <xsl:if test="text() != 1">
      <xsl:element name="e" namespace="{namespace-uri()}">
        <xsl:value-of select="text()" />
      </xsl:element>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- Match rings with given id, either the definition (<t>) or the backreference (<r>) -->
<xsl:template match="p:t/p:t[@id != ''] | p:r[@id != '']" priority="0.5">
  <xsl:apply-templates mode="fetch_ring_size" select="libxslt:node-set($ring_sizes)">
    <xsl:with-param name="ring_id" select="@id" />
  </xsl:apply-templates>
</xsl:template>


<!-- Match ring with empty backreference (means there is only one ring in the file -->
<xsl:template match="p:r">
  <xsl:apply-templates mode="fetch_ring_size" select="libxslt:node-set($ring_sizes)">
    <xsl:with-param name="ring_id" select="1" />
  </xsl:apply-templates>
</xsl:template>

<!-- Remove property RING from Ideal -->
<xsl:template match="p:object[@type='ideal::Ideal' or @type='Ideal']//p:property[@name='RING']"/>


<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
