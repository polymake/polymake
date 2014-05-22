/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.tools;

import java.awt.Color;
import java.lang.reflect.Method;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;

public class PointerDisplayTool extends AbstractTool {

	protected static final InputSlot AVATAR_POINTER = InputSlot.getDevice("PointerShipTransformation");
	SceneGraphComponent cmp = new SceneGraphComponent();
	private IndexedLineSetFactory ilsf;

	private Color highlightColor = Color.orange;
	private Color defaultColor = new Color(160, 160, 160);

	private transient boolean highlight = false;

	public PointerDisplayTool(double radius) {
		addCurrentSlot(AVATAR_POINTER);
		cmp.setAppearance(new Appearance());
		cmp.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);
		cmp.getAppearance().setAttribute("showPoints", false);
		cmp.getAppearance().setAttribute("showFaces", false);
		cmp.getAppearance().setAttribute("showLines", true);
		cmp.getAppearance().setAttribute("lineShader.tubeDraw", true);
		cmp.getAppearance().setAttribute("lineShader.tubeRadius", radius);
		cmp.setPickable(false);
		cmp.setTransformation(new Transformation());	
		
		setHighlight(false);
		
		ilsf = new IndexedLineSetFactory();
		ilsf.setVertexCount(2);
		ilsf.setEdgeCount(1);
		ilsf.setEdgeIndices(new int[]{0, 1});
		cmp.setGeometry(ilsf.getGeometry());
		setLength(1);
	}
	public PointerDisplayTool() {
		this(0.003);
	}

	boolean isAssigned;
	private Matrix m = new Matrix();
	private Method getPortalScale;
	{ 
		try {
			getPortalScale = Class.forName("de.jreality.portal.PortalCoordinateSystem").getMethod("getPortalScale");
		} catch (Exception e) {	e.printStackTrace(); } 
	}
	
	public void perform(ToolContext tc) {
		if (!isAssigned) {
			tc.getAvatarPath().getLastComponent().addChild(cmp);
			isAssigned=true;
		}
		m.assignFrom(tc.getTransformationMatrix(AVATAR_POINTER));
		
		try {
			MatrixBuilder.euclidean(m).scale((Double) getPortalScale.invoke(null)).assignTo(m);
		} catch (Exception e) {	e.printStackTrace(); }
		
		m.assignTo(cmp.getTransformation());
	}

	public void setVisible(boolean v) {
		cmp.setVisible(v);
	}

	public boolean isVisible() {
		return cmp.isVisible();
	}
	public Color getHighlightColor() {
		return highlightColor;
	}
	public void setHighlightColor(Color highlightColor) {
		this.highlightColor = highlightColor;
	}
	public boolean isHighlight() {
		return highlight;
	}
	public void setHighlight(boolean highlight) {
		this.highlight = highlight;
		cmp.getAppearance().setAttribute("diffuseColor", highlight ? getHighlightColor() : getDefaultColor());
	}
	public Color getDefaultColor() {
		return defaultColor;
	}
	public void setDefaultColor(Color defaultColor) {
		this.defaultColor = defaultColor;
	}

	public void setLength(double l) {
		ilsf.setVertexCoordinates(new double[][]{{0, 0, 0}, {0, 0, -l}});
		ilsf.update();
	}

}
