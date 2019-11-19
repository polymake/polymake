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

  former property REVERSE_TRANSFORMATION is stored as attachment now
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="description" indent="yes" />

<!-- data format change -->
<xsl:template match="p:property[@name='TRANSVERSALS']" />
<xsl:template match="p:property[@name='TRANSVERSAL_SIZES']" />

<!-- remove this flag since there was an inconsistency, see #406 -->
<xsl:template match="p:property[@name='BOUNDED']" />

<xsl:include href="trivial-copy.xslt" />

</xsl:transform>
