<?xml version="1.0" encoding="utf-8"?>

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

  This file produces the index.html file of the polymake documentation.
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns="http://www.w3.org/1999/xhtml"
  xmlns:pm="http://www.polymake.org/ns/docs#3"
>

<xsl:include href="macros.xsl" />

<xsl:output method="xml" indent="yes" omit-xml-declaration="yes"/>

<xsl:template match="/">
      <div id="content">
        <h1>Welcome to the documentation of <span class="pm">polymake</span></h1>
  
        This documentation constitutes a reference of all functions available in polymake <xsl:copy-of select="$version_text"/>

        <div class="level2">
          <h2><a name="apps">applications</a></h2>
          <ul>
            <xsl:apply-templates select="/pm:applications/pm:file[@name!='core.xml']">
              <xsl:sort select="@name"/>
            </xsl:apply-templates>
          </ul>

          <h2><a href="core.html">core functionality</a></h2>
          <a href="core.html">Core functionality</a> of polymake available in all applications.

          <h2><a href="doc_index.html">index</a></h2>
          The <a href="doc_index.html">index</a> contains an alphabetical list of all entities 
          (i.e., objects, properties, functions, types and applications) available in your version of polymake.

          <!-- FIXME: remove the condition as soon as the doxygen documentation ripens enough to be published without disgrace -->
          <xsl:choose>
            <xsl:when test="contains($version_text,'commit')">
          <h2><a href="PTL">PTL</a></h2>
          The automatically built <a href="PTL">documentation of the PTL</a> (polymake C++ template library) can be found <a href="PTL">here</a>.
            </xsl:when>
          </xsl:choose>

          <xsl:choose>
            <xsl:when test="contains($version_text,'commit')">
		<h2><a href="/release_docs/">release documentations</a></h2>
		The documentation of the current release can be found <a href="/release_docs/">here</a>.
            </xsl:when>
          </xsl:choose>

	</div>
</div>	

</xsl:template>


<xsl:template match="pm:file">
  <li>
    <a>
      <xsl:attribute name="href"><xsl:value-of select="substring-before(@name,'.')"/>.html</xsl:attribute>
      <xsl:value-of select="substring-before(@name,'.')"/>
    </a>
  </li>
</xsl:template>

</xsl:stylesheet>

<!--
  Local Variables:
  indent-tabs-mode:nil
  End:
-->
