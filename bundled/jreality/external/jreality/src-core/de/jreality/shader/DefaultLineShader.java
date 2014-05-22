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


package de.jreality.shader;

import java.awt.Color;

import de.jreality.geometry.FrameFieldType;
import de.jreality.geometry.TubeUtility;

/**
 * This is the default line shader used in jReality. 
 * <p>
 * There is an option to draw tubes around the edges, for improved visibility and 
 * "3D readability".  The radius of the tubes can be specified using {@link #setTubeRadius(Double)}.
 * These tubes (which are of course surfaces) will be shaded using
 * with the current polygon shader. In this way you can control the surface qualities of
 * the tubes.
 * <p>
 * If tubes are not requested, then the shader draws a more primitive version of the edges,
 * perhaps using a Bresenham algorithm.  Here the parameters include the line width 
 * (in pixel coordinates, default = 1), and some options for drawing dashed lines ({@link #setLineStipplePattern(Integer)}).
 * The color of the lines is set via {@link #setDiffuseColor(Color)}.
 * <p>
 * Different backends implement different versions of this shader. For example, the
 * software renderer (@link de.jreality.soft.Viewer} always draws tubes; the specification of the tube radius is in world coordinates.
 * On the other hand, the JOGL backend {@link de.jreality.jogl.JOGLViewer} can draw traditional Bresenham edges. In this case,
 * lighting is disabled so no shading effects are present. If tubes are enabled, then lighting is enabled, --- and the 
 * tube radius is in object coordinates.  
 * <p>
 * When tubes are NOT drawn:  If the underlying geometry has vertex colors and/or edge colors attached to it, then the diffuse color is determined as
 * follows: if {@link #getVertexColors()} returns <code>true</code> then vertex colors are used and are interpolated;
 * if not, then edge colors, if present are used, and are constant per edge; otherwise the value of {@link #getDiffuseColor()}
 * is used.
 * <p>
 * Note: the above explanation of coloring is not extended to the tubes, if they are requested. 
 * Here the vertex colors are ignored and the edge colors if present are used; otherwise as above.
 * <p>
 * Note: the different backends implement this shader somewhat differently.
 * 
 * @author Charles Gunn
 * @see DefaultPolygonShader  for general remarks on these shader interfaces.
 *
 */public interface DefaultLineShader extends LineShader {

	 Object CREATE_DEFAULT=new Object();

	 public final static boolean TUBE_DRAW_DEFAULT = true;
	 public final static double TUBE_RADIUS_DEFAULT = 0.025;
	 public final static FrameFieldType TUBE_STYLE_DEFAULT = FrameFieldType.PARALLEL;
	 public static final boolean SMOOTH_LINE_SHADING_DEFAULT = false;	// if true, then interpolate vertex colors
	 public final static double LINE_WIDTH_DEFAULT = 1.0;
	 public final static boolean LINE_STIPPLE_DEFAULT = false;
	 public final static boolean VERTEX_COLORS_DEFAULT = false;
	 public final static int LINE_STIPPLE_PATTERN_DEFAULT = 0x7e7e;
	 public final static int LINE_FACTOR_DEFAULT = 	1;
	 public static final Color DIFFUSE_COLOR_DEFAULT = Color.BLACK;
	 public static final double[][] CROSS_SECTION_DEFAULT = TubeUtility.octagonalCrossSection;
	 public static final boolean LIGHTING_ENABLED_DEFAULT = false;
	 
	 public static final boolean RADII_WORLD_COORDINATES_DEFAULT = CommonAttributes.RADII_WORLD_COORDINATES_DEFAULT;
	 
	 // general attributes
	 public abstract Color getDiffuseColor();
	 public abstract void setDiffuseColor(Color c);
	 public abstract Boolean getTubeDraw();
	 public abstract void setTubeDraw(Boolean b);
	 
	 // tube-related attributes
	 public abstract Double getTubeRadius();
	 public abstract void setTubeRadius(Double d);
	 public abstract FrameFieldType getTubeStyle();
	 public abstract void setTubeStyle(FrameFieldType i);
	 public abstract double[][] getCrossSection();
	 public abstract void setCrossSection(double[][] crossSection);
	 
	 void setRadiiWorldCoordinates(Boolean b);
	 Boolean getRadiiWorldCoordinates();
	 
	 // non-tube style related attributes
	 public abstract Double getLineWidth();
	 public abstract void setLineWidth(Double d);
	 public abstract Boolean getLineStipple();
	 public abstract void setLineStipple(Boolean b);
	 public abstract Integer getLineStipplePattern();
	 public abstract void setLineStipplePattern(Integer i);
	 public abstract Integer getLineFactor();
	 public abstract void setLineFactor(Integer i);
	 public abstract Boolean getVertexColors();
	 public abstract void setVertexColors(Boolean b);
	 public abstract Boolean getLineLighting(Boolean b);
	 public abstract void setLineLighting(Boolean b);
	 
	 PolygonShader getPolygonShader();
	 PolygonShader createPolygonShader(String shaderName);

	 TextShader getTextShader();
	 TextShader createTextShader(String name);
 }