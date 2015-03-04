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
-->

<!-- This rule copies the input node without changes and recursively descends into its
     children.  Only the top-level object attributes "version" and "tm" are not propagated. -->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">
    
<xsl:template match="*">
   <xsl:element name="{name()}" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*[name()!='tm' and name()!='version']" />
      <xsl:apply-templates select="./node()" />
   </xsl:element>
</xsl:template>

</xsl:transform>
