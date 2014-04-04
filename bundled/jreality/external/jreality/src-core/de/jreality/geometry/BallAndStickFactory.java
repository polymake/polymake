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


package de.jreality.geometry;

import java.awt.Color;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;

/**
 * This class constructs a ball-and-stick representation of an instance of {@link IndexedLineSet} 
 * and returns
 * it in the form of an instance of {@link SceneGraphComponent}.
 * <p>
 * In the default behavior, around each edge (vertex) of the IndexedLineSet is placed
 * a cylinder (sphere).  The radius can be separately controlled for the cylinders and for the
 * spheres, as can the colors.  Additionally, one can specify that arrow symbols be positioned
 * along the tubes; here one can control the relative position, the size, and the steepness of the
 * arrow head (which is essentially a cone). 
 * <p>
 * The cross section of the tube is currently fixed
 * to be an octagon. The returned SceneGraphComponent has two children, containing the 
 * sticks and the balls, respectively.
 * <p>
 * The general cycle of use is illustrated in the following code snippet:
 * <code><b><pre>
 *		BallAndStickFactory basf = new BallAndStickFactory(Primitives.sharedIcosahedron);
 *		basf.setBallRadius(.04);
 *		basf.setStickRadius(.02);
 * 		basf.setDrawArrows(true);
 *		basf.setArrowScale(.1);
 *		basf.setArrowSlope(1.5);
 *		basf.setArrowPosition(.9);
 *		basf.update();
 *		SceneGraphComponent tubedIcosa = basf.getSceneGraphComponent();
 * </pre></b></code>
 * <p>
 * <b>Note:</b> The same effect (without the arrows) can be achieved 
 * with the standard line and point shaders ({@link de.jreality.shader.DefaultLineShader}, etc),
 * by setting {@link de.jreality.shader.CommonAttributes#TUBES_DRAW} and {@link de.jreality.shader.CommonAttributes#SPHERES_DRAW},
 * resp., to <code>true</code> and setting the other shader parameters to control the
 * color and radius of the rusulting tubes and spheres. However, there are some constraints on this;
 * for example, it isn't possible to turn on flat shading and separate control of the
 *  transparency is not possible.
 * @author Charles Gunn
 * @see TubeUtility#tubeOneEdge(double[], double[], double, double[][], int)
 *
 */public class BallAndStickFactory {
	 IndexedLineSet ils;
	 double stickRadius=.025, ballRadius=.05;
	 Color stickColor = Color.YELLOW, ballColor=Color.GREEN, arrowColor = Color.RED;
	 int metric = Pn.EUCLIDEAN;
	 boolean showBalls = true, showSticks = true, realSpheres = true, drawArrows = false;
	 SceneGraphComponent theResult;
	 double arrowPosition = .5;		// where is tip of arrow placed?
	 double arrowScale = .1;			// scale=1:  height of cone is length of edge
	 double arrowSlope = 1.0;			// bigger scale: more pointy arrow profile
	 Appearance ballsAp, sticksAp, arrowsAp, topAp;
	 Geometry stickGeometry = null, ballGeometry = null;
	 private static IndexedFaceSet urCone = null;
	 double[][] crossSection = null;
	 static double[][] octagonalCrossSection = {{1,0,0}, 
			{.707, .707, 0}, 
			{0,1,0},
			{-.707, .707, 0},
			{-1,0,0},
			{-.707, -.707, 0},
			{0,-1,0},
			{.707, -.707, 0}}; //,{1,0,0}};
	private SceneGraphComponent sticks;
	private SceneGraphComponent balls;
	private SceneGraphComponent arrow;
	 static {
		 urCone = Primitives.pyramid(octagonalCrossSection, new double[]{0,0,1});
		 IndexedFaceSetUtility.calculateAndSetVertexNormals(urCone);
	 }

	public BallAndStickFactory(IndexedLineSet i)	{
		super();
		ils = i;
		sticks = new SceneGraphComponent("sticks");
		balls = new SceneGraphComponent("balls");
		topAp =  new Appearance();
//		topAp.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.SMOOTH_SHADING, true);
		theResult = new SceneGraphComponent("BAS");
		theResult.setAppearance(topAp);
		sticksAp =  new Appearance();
		sticksAp.setAttribute(CommonAttributes.FACE_DRAW, true);
		sticksAp.setAttribute(CommonAttributes.EDGE_DRAW, false);
		sticksAp.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		sticks.setAppearance(sticksAp);
		ballsAp =  new Appearance();
		balls.setAppearance(ballsAp);				
		arrowsAp = new Appearance();
		theResult.addChild(sticks);
		theResult.addChild(balls);
	 }

	 public void update()	{
			topAp.setAttribute("metric", metric);
			sticks.setVisible(showSticks);
			balls.setVisible(showBalls);
			if (stickColor != null) sticksAp.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, stickColor);
			if (arrowColor != null) arrowsAp.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, arrowColor);
		 	// create sticks on edges
			if (showBalls)	{
				if (!realSpheres)	{
					balls.setGeometry(ils);
					ballsAp.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, ballRadius);
					ballsAp.setAttribute(CommonAttributes.FACE_DRAW, false);
					ballsAp.setAttribute(CommonAttributes.EDGE_DRAW, false);
					ballsAp.setAttribute(CommonAttributes.VERTEX_DRAW, true);
					ballsAp.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.SPHERES_DRAW, true);
					if (ballColor != null) {
						ballsAp.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, ballColor);
						ballsAp.setAttribute(CommonAttributes.POINT_SHADER+"."+
								CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, ballColor);
					}					
				} else {
					balls.setGeometry(null);
					ballsAp.setAttribute(CommonAttributes.FACE_DRAW, true);
					ballsAp.setAttribute(CommonAttributes.EDGE_DRAW, false);
					ballsAp.setAttribute(CommonAttributes.VERTEX_DRAW, false);
					if (ballColor != null) {
						ballsAp.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, ballColor);
					}					
					DataList vertices = ils.getVertexAttributes(Attribute.COORDINATES);
					DataList vertexColors = ils.getVertexAttributes(Attribute.COLORS);
					DataList vertexRadii = ils.getVertexAttributes(Attribute.POINT_SIZE);
					int n = ils.getNumPoints();
					SceneGraphUtility.removeChildren(balls);
					for (int i = 0; i<n; ++i)	{
						if (vertexRadii != null) {
							ballRadius = vertexRadii.item(i).toDoubleArray(null)[0];
						}
						double[] p1 = vertices.item(i).toDoubleArray(null);	
						SceneGraphComponent cc = new SceneGraphComponent("ball"+i);
						MatrixBuilder.init(null, metric).translate(p1).scale(ballRadius).assignTo(cc);
						if (ballGeometry != null) cc.setGeometry(ballGeometry);
						else cc.setGeometry(new Sphere());
						if (vertexColors != null) {
							Color ccc = null;
							double[] dcc = vertexColors.item(i).toDoubleArray(null);
							if (dcc.length == 4) ccc = new Color((float) dcc[0], (float) dcc[1], (float) dcc[2], (float) dcc[3]);
							else ccc = new Color((float) dcc[0], (float) dcc[1], (float) dcc[2]);
							Appearance ap = new Appearance();
							ap.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, ccc);
							cc.setAppearance(ap);
						}
						balls.addChild(cc);
					}
				}				
			}
			if (showSticks)	{
				DataList vertices = ils.getVertexAttributes(Attribute.COORDINATES);
				DataList edgeColors = ils.getEdgeAttributes(Attribute.COLORS);
				int n = ils.getNumEdges();
				SceneGraphUtility.removeChildren(sticks);
				for (int i = 0; i<n; ++i)	{
					int[] ed = ils.getEdgeAttributes(Attribute.INDICES).item(i).toIntArray(null);
					int m = ed.length;
					for (int j = 0; j<m-1; ++j)	{
						int k = ed[j];
						double[] p1 = vertices.item(k).toDoubleArray(null);	
						k = ed[j+1];
						double[] p2 = vertices.item(k).toDoubleArray(null);	
						SceneGraphComponent cc = TubeUtility.tubeOneEdge(p1, p2, stickRadius, crossSection, metric);
						if (stickGeometry != null) cc.setGeometry(stickGeometry);
						if (edgeColors != null) {
							Color ccc = null;
							double[] dcc = edgeColors.item(i).toDoubleArray(null);
							if (dcc.length == 4) ccc = new Color((float) dcc[0], (float) dcc[1], (float) dcc[2], (float) dcc[3]);
							else ccc = new Color((float) dcc[0], (float) dcc[1], (float) dcc[2]);
							Appearance ap = new Appearance();
							ap.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, ccc);
							cc.setAppearance(ap);
						}
						if (cc != null) sticks.addChild(cc);
						if (drawArrows && cc != null)		{
							arrow = new SceneGraphComponent("Arrows");
							FactoredMatrix arrowM = new FactoredMatrix(metric);
							double d;
							if (p1.length == 3) d = Rn.euclideanDistance(p1, p2);
							else d = Pn.distanceBetween(p1, p2, metric);
							double flatten = arrowSlope/(d);
							double stretch = arrowScale/stickRadius;
							arrowM.setStretch(stretch, stretch, arrowScale*flatten);
							arrowM.setTranslation(0,0,arrowPosition-.5);
							arrowM.update();
							arrowM.assignTo(arrow);
							arrow.setAppearance(arrowsAp);
							arrow.setGeometry(urCone);
							cc.addChild(arrow);
						}
					}
				}				
			}
//			if (showBalls)	{
//				// we should allow the user to specify "real" balls, not via the appearance.
//			}
	 }
	 
	 public void setStickRadius(double r)	{
		 stickRadius = r;
	 }
	 
	public void setBallColor(Color ballColor) {
		this.ballColor = ballColor;
	}

	public void setBallRadius(double ballRadius) {
		this.ballRadius = ballRadius;
	}

	public void setStickColor(Color stickColor) {
		this.stickColor = stickColor;
	}

	public void setArrowColor(Color arrowColor) {
		this.arrowColor = arrowColor;
	}

	public void setMetric(int metric) {
		this.metric = metric;
	}
	
	public SceneGraphComponent getSceneGraphComponent()	{
		return theResult;
	}

	protected static SceneGraphComponent sticks(IndexedLineSet ifs, double rad, int metric)	{
		return sticks(null, ifs, rad, metric);
	}
	public static SceneGraphComponent sticks(SceneGraphComponent sgc, IndexedLineSet ifs, double rad, int metric)	{
		if (sgc == null) sgc = new SceneGraphComponent();
		DataList vertices = ifs.getVertexAttributes(Attribute.COORDINATES);
		int n = ifs.getNumEdges();
		for (int i = 0; i<n; ++i)	{
			int[] ed = ifs.getEdgeAttributes(Attribute.INDICES).item(i).toIntArray(null);
			int m = ed.length;
			for (int j = 0; j<m-1; ++j)	{
				int k = ed[j];
				double[] p1 = vertices.item(k).toDoubleArray(null);	
				k = ed[j+1];
				double[] p2 = vertices.item(k).toDoubleArray(null);	
				SceneGraphComponent cc = TubeUtility.tubeOneEdge(p1, p2, rad, null, metric);
				if (cc != null) sgc.addChild(cc);
			}
		}
		return sgc;
	}

	public double getArrowPosition() {
		return arrowPosition;
	}

	public void setArrowPosition(double arrowPosition) {
		this.arrowPosition = arrowPosition;
	}

	public double getArrowScale() {
		return arrowScale;
	}

	public void setArrowScale(double arrowScale) {
		this.arrowScale = arrowScale;
	}

	public double getArrowSlope() {
		return arrowSlope;
	}

	public void setArrowSlope(double arrowSlope) {
		this.arrowSlope = arrowSlope;
	}

	public boolean isShowArrows() {
		return drawArrows;
	}

	public void setShowArrows(boolean drawArrows) {
		this.drawArrows = drawArrows;
	}

	public boolean isShowBalls() {
		return showBalls;
	}

	public void setShowBalls(boolean showBalls) {
		this.showBalls = showBalls;
	}

	public boolean isShowSticks() {
		return showSticks;
	}

	public void setShowSticks(boolean showSticks) {
		this.showSticks = showSticks;
	}

	public double[][] getCrossSection() {
		return crossSection;
	}

	public void setCrossSection(double[][] crossSection) {
		this.crossSection = crossSection;
	}

	public Geometry getBallGeometry() {
		return ballGeometry;
	}

	public void setBallGeometry(Geometry ballGeometry) {
		this.ballGeometry = ballGeometry;
	}

	public Geometry getStickGeometry() {
		return stickGeometry;
	}

	public void setStickGeometry(Geometry stickGeometry) {
		this.stickGeometry = stickGeometry;
	}


}
