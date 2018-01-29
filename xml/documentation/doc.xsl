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


This file produces the html file containing the documentation of a given
polymake application.
-->

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:html="http://www.w3.org/1999/xhtml"
	xmlns:pm="http://www.polymake.org/ns/docs#3"
	xmlns="http://www.w3.org/1999/xhtml"
>

<xsl:include href="macros.xsl" />

<xsl:output method="xml" indent="yes" omit-xml-declaration="yes" />


<xsl:template match="/">
			<div id="content">
				<xsl:apply-templates select="pm:polymake_apps"/>	 
				<xsl:apply-templates select="pm:application"/>	 
			</div>
</xsl:template>

<xsl:template name="page_title">
	<xsl:choose>
	<xsl:when test="descendant-or-self::pm:application/@name='core'">core functionality</xsl:when>
	<xsl:otherwise>application: <xsl:value-of select="@name"/></xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="pm:polymake_apps">
	<h1 class="extension">extension: <xsl:value-of select="$ext_sn"/></h1>
	<p><xsl:call-template name="LFsToBRs">
		<xsl:with-param name="input"><xsl:value-of select = "//pm:extension[./pm:URI=$ext_name]/pm:text"/></xsl:with-param>
	</xsl:call-template></p>
	
	<xsl:apply-templates select="pm:application">
		<xsl:sort select="@name"/>
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="pm:application">
	<h1><xsl:call-template name="page_title"/></h1>

	<xsl:apply-templates select="pm:description" />
	<xsl:apply-templates select="@ext"/>


	<xsl:apply-templates select="pm:imports-from"/>
	<xsl:apply-templates select="pm:uses"/>
		
	<xsl:apply-templates select="pm:objects"/>
	<xsl:apply-templates select="pm:user-functions"/>
	<xsl:apply-templates select="pm:property-types"/>
	<xsl:apply-templates select="pm:common-option-lists"/>
</xsl:template>



<xsl:template match="pm:description">
	<xsl:choose>
	<xsl:when test=".='UNDOCUMENTED'">
		<div class="undoc">
			<xsl:apply-templates/>
		</div>
	</xsl:when>
	<xsl:when test="html:p='UNDOCUMENTED'">
		<div class="undoc">
			<xsl:apply-templates/>
		</div>
	</xsl:when>
	<xsl:otherwise>
		<div class="descr">
			<xsl:apply-templates/>
		</div>
	</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="html:p">
	<p><xsl:apply-templates/></p>
</xsl:template>
<xsl:template match="html:div">
	<div><xsl:apply-templates/></div>
</xsl:template>
<xsl:template match="html:dl">
	<dl><xsl:apply-templates/></dl>
</xsl:template>
<xsl:template match="html:dt">
	<dt><xsl:apply-templates/></dt>
</xsl:template>
<xsl:template match="html:dd">
	<dd><xsl:apply-templates/></dd>
</xsl:template>
<xsl:template match="html:blockquote">
	<blockquote><xsl:apply-templates/></blockquote>
</xsl:template>
<xsl:template match="html:em">
	<em>
		<xsl:if test="@class">
			<xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
		</xsl:if>
		<xsl:apply-templates/>
	</em>
</xsl:template>
<xsl:template match="html:code">
	<code><xsl:apply-templates/></code>
</xsl:template>
<xsl:template match="html:strong">
	<strong><xsl:apply-templates/></strong>
</xsl:template>
<xsl:template match="html:sub">
	<sub><xsl:apply-templates/></sub>
</xsl:template>
<xsl:template match="html:sup">
	<sup><xsl:apply-templates/></sup>
</xsl:template>
<xsl:template match="html:span">
	<span>
		<xsl:for-each select="@*">
			<xsl:attribute name="{name(.)}">
				<xsl:value-of select="."/>
			</xsl:attribute>
		</xsl:for-each>
		<xsl:apply-templates/>
	</span>
</xsl:template>

<xsl:template match="@ext">
	<xsl:if test = "not(parent::*/ancestor::*/@ext=.) and not(.=$ext_name)">
		<!-- TODO: add link to the documentation for the extension?? -->
		<div class="descr">
			Contained in extension <code>
			<xsl:choose>
				<xsl:when test = "../@ext_name!=''">
					<xsl:value-of select="../@ext_name"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="."/>
				</xsl:otherwise>
			</xsl:choose></code>.
		</div>
	</xsl:if>
</xsl:template>



<xsl:template match="pm:imports-from">
	<b>imports from: </b>
	<xsl:call-template name="listapps"/>
</xsl:template>
<xsl:template match="pm:uses">
	<b>uses: </b> 
	<xsl:call-template name="listapps"/>
</xsl:template>
<xsl:template name="listapps">
		<xsl:for-each select="pm:application">
			<xsl:sort select="@name"/>

			<a>
				<xsl:attribute name="href"><xsl:value-of select="@name"/>.html</xsl:attribute>

				<xsl:value-of select="@name"/>
			</a>

			<xsl:if test="position()!=last()">, </xsl:if>
		</xsl:for-each>
	<br />
</xsl:template>

<xsl:template name="derived-from">
	<xsl:if test="pm:derived-from"> <b>derived from: </b>
		<xsl:for-each select="pm:derived-from">
			<xsl:call-template name="typelinks"/>
			<xsl:if test="position()!=last()">, </xsl:if>
		</xsl:for-each>
		<br />
	</xsl:if>
</xsl:template>


<xsl:template name="specializations">
	<xsl:if test="pm:specialization">
	<h4>Specializations of <xsl:value-of select="@name"/></h4>

		<ul class="unfoldable">
			<xsl:apply-templates select="pm:specialization"/>
		</ul>
	</xsl:if>
</xsl:template>

<xsl:template match="pm:specialization">
<li><div class="li">

	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">
		<a>
			<xsl:call-template name="unfoldattributes"/>
			<b><xsl:value-of select="@name"/></b>
		</a>
		
		<div>
			<xsl:call-template name="unfoldspan"/>

			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="@ext"/>
		</div>
	</div>
</div></li>
</xsl:template>

<xsl:template match="pm:only">
		<div class="descr"><b>Only defined for <a><xsl:attribute name="href"><xsl:value-of select="@href" /></xsl:attribute><xsl:value-of select="@name"/></a></b></div>
</xsl:template>



<xsl:template match="pm:property-types">
	<div class="level3">

	<h2>Property Types</h2>
	
	<ul class="unfoldable">
		<xsl:apply-templates select="pm:property-type">
			<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
		</xsl:apply-templates>
		
		<xsl:apply-templates select="pm:category">
			<xsl:sort select="@name"/>
		</xsl:apply-templates>
	</ul>
	

	</div>

</xsl:template>

<xsl:template match="pm:property-type">
<li><div class="li">

	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">
		<a>
			<xsl:call-template name="unfoldattributes"/>
			<b><xsl:value-of select="@name"/></b>
		</a>
		<xsl:if test="pm:tparam">
			&lt;<xsl:for-each select="pm:tparam">
			<xsl:value-of select="@name"/>
			<xsl:if test="position()!=last()">, </xsl:if>
			</xsl:for-each>&gt;
		</xsl:if>

		<div>
			<xsl:call-template name="unfoldspan"/>

			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="@ext"/>
			
			<xsl:call-template name="derived-from"/>
			
			<div class="level3">
			<xsl:if test="pm:tparam">
				<h5>Type Parameters</h5>
				<table class="args">
					<xsl:apply-templates select="pm:tparam"/>
				</table>
			</xsl:if>
			</div>
		
		<xsl:apply-templates select="pm:user-methods"/>
		
		</div>
	</div>
	
</div></li>
</xsl:template>





<xsl:template match="pm:objects">
	<div class="level2">

		<h2><b>Objects</b> </h2>
		
		<ul class="unfoldable">
		<xsl:apply-templates select=".//pm:object">
			<xsl:sort select="@name"/>
		</xsl:apply-templates>

		</ul>
		
	</div>
</xsl:template>

<xsl:template match="pm:object">
<li><div class="li">
	
	<xsl:call-template name="inserticon"/>		
	
	<div class="unfoldable">

		<div class="object">
		<h3>
			<a>
				<xsl:call-template name="unfoldattributes"/>
				<span class="object"><xsl:value-of select="@name"/></span>
				
			</a>
		</h3>
		</div>

	
		<div>
			<xsl:call-template name="unfoldspan"/>	
			<xsl:if test="parent::pm:category">
				<b>Category: </b><xsl:value-of select="../@name"/><br />
			</xsl:if>
	
			<xsl:choose>
			<xsl:when test="pm:description">	
				<xsl:apply-templates select="pm:description" />
				<xsl:apply-templates select="@ext"/>
			</xsl:when>
			</xsl:choose>

			<xsl:apply-templates select="pm:examples"/>

			<xsl:call-template name="derived-from"/>		

			<div class="level3">
			<xsl:if test="pm:tparam">
				<h5>Type Parameters</h5>
				<table class="args">
					<xsl:apply-templates select="pm:tparam"/>
				</table>
			</xsl:if>
			</div>

			<xsl:call-template name="specializations"/>
				
			<xsl:apply-templates select="pm:properties"/>			
			<xsl:apply-templates select="pm:user-methods"/>
			<xsl:apply-templates select="pm:permutations"/>
		
		</div>
	</div>	
</div></li>
</xsl:template>





<xsl:template match="pm:properties">

	<xsl:if test=".//pm:property">
		<h4>Properties of <xsl:value-of select="../@name"/></h4>
			
		<ul class="unfoldable">
			<xsl:apply-templates select="pm:property">
				<xsl:sort select="@name"/>
			</xsl:apply-templates>
			
			<xsl:apply-templates select="pm:category">
				<xsl:sort select="@name"/>
			</xsl:apply-templates>
		</ul>
	</xsl:if>
	
</xsl:template>

<xsl:template match="pm:property">
<li><div class="li">

	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">
		<a>
			<xsl:call-template name="unfoldattributes"/>
			<b><xsl:value-of select="@name"/></b>:
		</a>
		
		 <xsl:call-template name="typelinks"/><br />
		 
		<div>
			<xsl:call-template name="unfoldspan"/>

			<xsl:apply-templates select="pm:only"/>
			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="pm:examples"/>
			<xsl:apply-templates select="pm:depends"/>
			<xsl:apply-templates select="@ext"/>
			<xsl:apply-templates select="pm:properties"/>
		</div>
	</div>
</div></li>
</xsl:template>





<xsl:template match="pm:permutations">
	<h4>Permutations of <xsl:value-of select="../@name"/></h4>

	<ul class="unfoldable">
		<xsl:apply-templates select="pm:permutation">
			<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
		</xsl:apply-templates>
	</ul>
</xsl:template>

<xsl:template match="pm:permutation">
	<li><div class="li">

	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">

		<a>
			<xsl:call-template name="unfoldattributes"/>
			<b><xsl:value-of select="@name"/></b>
		</a>
				 
		<div>
			<xsl:call-template name="unfoldspan"/>

			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="@ext"/>
			<xsl:apply-templates select="pm:properties"/>
		</div>
	</div>
	
	</div></li>
</xsl:template>





<xsl:template match="pm:category">
<li><div class="li">
	<xsl:call-template name="inserticon"/>	

	<div class="unfoldable">
		<div class="category"><h3>
			<a>
				<xsl:call-template name="unfoldattributes"/>	
				<span class="category"><xsl:value-of select="@name"/></span>
			</a>
		</h3></div>
	
		<div>
			<xsl:call-template name="unfoldspan"/>
			
			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="@ext"/>
			
			<xsl:if test="pm:object|pm:property|pm:property-type|pm:function|pm:common-option-list">
				<ul class="unfoldable">

					<xsl:apply-templates select="pm:object">
						<xsl:sort select="@name"/>
					</xsl:apply-templates>

					<xsl:apply-templates select="pm:property">
						<xsl:sort select="@name"/>
					</xsl:apply-templates>

					<xsl:apply-templates select="pm:property-type">
						<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
					</xsl:apply-templates>

					<xsl:apply-templates select="pm:function">
						<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
					</xsl:apply-templates>

					<xsl:apply-templates select="pm:common-option-list">
						<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
					</xsl:apply-templates>

				</ul>
			</xsl:if>
		</div>
	</div>

</div></li>
</xsl:template>





<xsl:template match="pm:user-methods">

	<h4>User Methods of <xsl:value-of select="../@name"/></h4>
	
	<ul class="unfoldable">
		<xsl:apply-templates select="pm:function">
			<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
		</xsl:apply-templates>
		<xsl:apply-templates select="pm:category">
			<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
		</xsl:apply-templates>
	</ul>
</xsl:template>

<xsl:template match="pm:user-functions">
	<div class="level2">

		<h2>User Functions</h2>
			
		<ul class="unfoldable">
			<xsl:apply-templates select="pm:function">
				<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
			</xsl:apply-templates>
			
			<xsl:apply-templates select="pm:category">
				<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
			</xsl:apply-templates>
		</ul>
	
	</div>
</xsl:template>


<xsl:template match="pm:function">
<li><div class="li">
	
	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">
		<a>
			<xsl:call-template name="unfoldattributes"/>		
			<b><xsl:value-of select="@name"/></b> 
		</a>
		<xsl:if test="pm:tparam">
			&lt;<xsl:for-each select="pm:tparam">
				<xsl:value-of select="@name"/>
				<xsl:if test="position()!=last()">, </xsl:if>
			</xsl:for-each>&gt;
		 </xsl:if>
			(<xsl:for-each select="pm:param">
				<xsl:value-of select="@name"/>
				<xsl:if test="position()!=last()">, </xsl:if>
			</xsl:for-each>)  

			<xsl:choose>
			<xsl:when test="pm:return">
				&#8594; <xsl:value-of select="pm:return/@type"/>
			</xsl:when>
			</xsl:choose>
		
		<div>
			<xsl:call-template name="unfoldspan"/>
				
			<br />
			<div class="descr_func">
                                <xsl:apply-templates select="pm:only"/>
				<xsl:apply-templates select="pm:description"/>
				<xsl:apply-templates select="@ext"/>
			</div>
			
			<div class="level3">
			<xsl:if test="pm:tparam">
				<h5>Type Parameters</h5>
				<table class="args">
					<xsl:apply-templates select="pm:tparam"/>
				</table>
			</xsl:if>
			<xsl:if test="pm:param">
				<h5>Parameters</h5>
				<table class="args">
					<xsl:apply-templates select="pm:param"/>
				</table>
			</xsl:if>
			<xsl:if test="pm:options|pm:option|pm:common-option-list">
				<h5>Options</h5>
				<xsl:if test="pm:option|pm:common-option-list">
					<table class="args">
						<xsl:apply-templates select="pm:option"/>
						<xsl:for-each select="pm:common-option-list">
							<tr><td>option list:</td>
							<td><xsl:call-template name="optionlinks"/></td></tr>
						</xsl:for-each>
					</table>
				</xsl:if>
				<xsl:apply-templates select="pm:options"/>
			</xsl:if>
			<xsl:if test="pm:return">
				<h5>Returns</h5>
				<table class="args">
					<xsl:apply-templates select="pm:return"/>
				</table>
			</xsl:if>
			</div>
			
			<xsl:apply-templates select="pm:examples"/>
			<xsl:apply-templates select="pm:depends"/>
		</div>	
	</div>
	
</div></li>
</xsl:template>


<xsl:template match="pm:param">
	<tr>
		<td><xsl:call-template name="typelinks"/></td>
		<td class="param"><xsl:value-of select="@name"/></td>
		<td><xsl:apply-templates select="pm:description"/></td>
	</tr>
</xsl:template>

<xsl:template match="pm:tparam">
	<tr>
		<td class="param"><xsl:value-of select="@name"/></td>
		<td><xsl:apply-templates select="pm:description"/></td>
	</tr>
</xsl:template>

<xsl:template match="pm:options">
	<xsl:apply-templates select="pm:description"/>
	<table class="args">
		<xsl:apply-templates select="pm:option"/>
	</table>
</xsl:template>

<xsl:template match="pm:option">
	<tr>
		<td><xsl:call-template name="typelinks"/></td>
		<td class="param"><xsl:value-of select="@name"/></td>
		<td><xsl:apply-templates select="pm:description"/></td>
	</tr>
</xsl:template>

<xsl:template match="pm:return">
	<tr>
		<td><xsl:call-template name="typelinks"/></td>
		<td><xsl:apply-templates select="pm:description"/></td>
	</tr>
</xsl:template>


<xsl:template match="pm:author">
	<div class="author"><b>Author(s): </b><xsl:value-of select="@name"/></div>
</xsl:template>

<xsl:template match="pm:depends">
	<div class="depends"><b>Depends on: </b><xsl:value-of select="@name"/></div>
</xsl:template>

<xsl:template match="pm:examples">
	<br/>
	<div class="examples">
		<xsl:choose>
			<xsl:when test="count(pm:example)=1">
				<b>Example:</b>
				<ul style="list-style-type:none">
					<xsl:apply-templates select="pm:example"/>
				</ul>
			</xsl:when>
			<xsl:otherwise>
				<b>Examples:</b>
				<ul>
					<xsl:apply-templates select="pm:example"/>
				</ul>
			</xsl:otherwise>
		</xsl:choose>
	</div>
</xsl:template>

<xsl:template match="pm:example">
	<li>
		<xsl:apply-templates select="pm:description"/>
	</li>
</xsl:template>

<xsl:template match="html:a">
	<a>
		<xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
		<xsl:attribute name="onclick">unfold(&#39;span:<xsl:value-of select="substring-after(@href,'#')"/>&#39;);</xsl:attribute>		

		<xsl:if test="@class">
			<xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
		</xsl:if>

		<xsl:value-of select="."/>
	
	</a>
</xsl:template>



<xsl:template name="inserticon">
	<div class="icon">
		<xsl:attribute name="id">icon:<xsl:value-of select="@xml:id"/></xsl:attribute>
		<xsl:attribute name="onclick">swap_content(&#39;span:<xsl:value-of select="@xml:id"/>&#39;); return false;</xsl:attribute>
		&#160;	
	</div>
</xsl:template>

<xsl:template name="unfoldattributes">
	<xsl:attribute name="href">#</xsl:attribute>
	<xsl:attribute name="class">javalink</xsl:attribute>
	<xsl:attribute name="id"><xsl:value-of select="@xml:id"/></xsl:attribute>
	<xsl:attribute name="onclick">swap_content(&#39;span:<xsl:value-of select="@xml:id"/>&#39;); return false;</xsl:attribute>
</xsl:template>

<xsl:template name="unfoldspan">
	<xsl:attribute name="style">display: inline</xsl:attribute>
	<xsl:attribute name="id">span:<xsl:value-of select="@xml:id"/></xsl:attribute>
	<xsl:attribute name="class">foldit</xsl:attribute>
</xsl:template>




<xsl:template name="typelinks">
	<xsl:choose>
	<xsl:when test="@class='invalid'">
		<xsl:value-of select="@type|@object"/>	
	</xsl:when>
	<xsl:otherwise>
		<a>
			<xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
			<xsl:attribute name="onclick">unfold(&#39;span:<xsl:value-of select="substring-after(@href,'#')"/>&#39;);</xsl:attribute>		
			
			<xsl:value-of select="@type|@object"/>
		</a>
	</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="optionlinks">
	<xsl:choose>
	<xsl:when test="@class='invalid'">
		<xsl:value-of select="@name"/>	
	</xsl:when>
	<xsl:otherwise>
		<a>
			<xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
			<xsl:attribute name="onclick">unfold(&#39;span:<xsl:value-of select="substring-after(@href,'#')"/>&#39;);</xsl:attribute>		
			
			<xsl:value-of select="@name"/>
		</a>
	</xsl:otherwise>
	</xsl:choose>
</xsl:template>


<xsl:template match="pm:common-option-lists">
	<div class="level3">

		<h2>Common Option Lists</h2>
		
		<ul class="unfoldable">
			<xsl:apply-templates select="pm:common-option-list">
				<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>			
			</xsl:apply-templates>
			
			<xsl:apply-templates select="pm:category">
				<xsl:sort select="translate(@name,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>			
			</xsl:apply-templates>
		</ul>
	</div>
</xsl:template>



<xsl:template match="pm:common-option-list">
	<li><div class="li">

	<xsl:call-template name="inserticon"/>	
	
	<div class="unfoldable">

		<a>
			<xsl:call-template name="unfoldattributes"/>
			<b><xsl:value-of select="@name"/></b>
		</a>
				 
		<div>
			<xsl:call-template name="unfoldspan"/>

			<xsl:apply-templates select="pm:description"/>
			<xsl:apply-templates select="@ext"/>
	
			<xsl:if test="pm:imports-from"> <b>imports from: </b>
				<xsl:for-each select="pm:imports-from/pm:common-option-list">
						<xsl:call-template name="optionlinks"/>
					<xsl:if test="position()!=last()">, </xsl:if>
				</xsl:for-each>
				<br /><br />
			</xsl:if>

			<xsl:if test="pm:options|pm:option">
				<h5>Options</h5>
				<table class="args">
					<xsl:apply-templates select="pm:option"/>
				</table>
				<xsl:apply-templates select="pm:options"/>
			</xsl:if>

		</div>
	</div>
	
	</div></li>
</xsl:template>

</xsl:stylesheet>
