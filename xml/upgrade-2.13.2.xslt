<?xml version="1.0"?>

<!--
  Copyright (c) 1997-2014
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

  1. Delete N_INEQUALITIES, N_EQUATIONS and SUPPORT_SIZE
-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
			     xmlns:p="http://www.math.tu-berlin.de/polymake/#3">

<xsl:output method="xml" encoding="utf-8" cdata-section-elements="p:description" indent="yes" />


<xsl:include href="trivial-copy.xslt" />

<!-- 1. -->

<xsl:template match="p:property[@name='N_INEQUALITIES']"/>
<xsl:template match="p:property[@name='N_EQUATIONS']"/>
<xsl:template match="p:property[@name='SUPPORT_SIZE']"/>

</xsl:transform>
