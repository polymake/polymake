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


  This file embeds the output of the documentation generation into valid html files equipped with the polymake theme.
-->


<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:html="http://www.w3.org/1999/xhtml"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
>

<xsl:include href="macros.xsl" />

<xsl:output method="xml" indent="yes" 
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" />



<xsl:template match="/">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
	<title>polymake documentation</title>

	<link rel="stylesheet" href="docstyle.css" />
	<script type="text/javascript" src="functions.js"></script>
	<link rel="stylesheet" href="style.css" type="text/css" media="all" />
	<link rel="shortcut icon" href= "favicon.png" />

	<meta http-equiv="content-script-type" content="text/javascript" />
	<meta http-equiv="content-type" content="text/html; charset=utf-8" />
	<script type="text/javascript"   src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>	
</head>


<body onload="start()">


<div id="header_top">
<div class="biglogo"><img src="images/pmlogobig.png" alt="polymake" /></div>
<a href="index.html"><img src="images/doclogo.png" alt="polymake wiki" class="toplogo" /></a>
</div>

<div id="container">
<div id="page">

<xsl:call-template name="nav-box"/>

	<xsl:apply-templates/>
	
</div>
</div>

<div id="footer"></div>
</body>
</html>
</xsl:template>


<xsl:template match="@*|node()">
   <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
   </xsl:copy>
</xsl:template>

</xsl:stylesheet>
