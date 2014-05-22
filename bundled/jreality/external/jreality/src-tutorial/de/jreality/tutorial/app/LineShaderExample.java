package de.jreality.tutorial.app;

import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.SMOOTH_SHADING;

import java.awt.Color;

import javax.swing.SwingConstants;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.geometry.PointSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * Features shown:
 * 	 without tubes (bresenham)
 * 		1. normal
 * 	 	2. with stippling (JOGL backend only)
 *   with tubes
 *   	edges: list of line segments
 *   		3. color from appearance, opaque
 *   		4. color from appearance, transparency enabled
 *   		5. color from vertices of line set
 *   			6. with relative radii allowing varying thickness
 *   	edges: a single long "edge" 
 *   		7. color from vertices of line set
 *   			8. with relative radii, and a custom "cross section" curve
 *   
 *   Notes:
 *   1. Textures cannot be applied to these tubes.
 *   	See {@link BallAndStickFactory} for a class which allows this.
 *   2. Width of lines (Examples 1 and 2) and tubes (the rest) are set independently
 *   	since linewidth is specified in pixels, and tube radius in object coordinates.
 *   3. Note that the indexed line sets demonstrate different ways to specify the edges
 *      of the line set:
 *      	a. as many line segments, or
 *      	b. as a single long curve
 *      See the method {@link IndexedLineSetFactory#setEdgeIndices()}.
 *      The resulting tubes will appear differently.
 *   4. Colors specified in the shader are ALWAYS RGB values; alpha (transparency) values
 *   	must be set using the separate "transparency" attribute.
 *   5. This example has been tested on the JOGL backend; other backends may not yield the
 *   	same behavior, particularly where rendering hints are involved.
 * @author gunn
 *
 */
public class LineShaderExample { 
  
  public static void main(String[] args) {
	int numPoints = 20;
	// data needed for the various versions of the line set created here 
	double[][] verts = new double[numPoints][], 
		edgeColors = new double[numPoints][];
	double[] relrad = new double[numPoints];
	int[][] edges = new int[numPoints][2], 
		singleEdge = new int[1][numPoints+1];
	// create a ring and associated data
	for (int i=0; i<numPoints; ++i)	{ 
		double angle = (Math.PI*2*i)/numPoints;
		verts[i] = new double[]{Math.cos(angle), Math.sin(angle), 0};
		edgeColors[i] = new double[]{verts[i][0]*.5+.5, verts[i][1]*.5+.5,0.0};
		relrad[i] = verts[i][0]+1.5;
		edges[i][0] = i;
		edges[i][1] = (i+1)%numPoints;
		singleEdge[0][i] = i;
	}
	singleEdge[0][numPoints] = 0;
	
	// each sample gets a numeric label at its geometric center point.  Prepare the appearance.
	Appearance labelAp = new Appearance();
    DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(labelAp, false);
	dgs.setShowPoints(true);
    DefaultPointShader pointShader = (DefaultPointShader)dgs.getPointShader();
    pointShader.setPointRadius(.0001);
	DefaultTextShader pts = (DefaultTextShader) (pointShader).getTextShader();
	pts.setScale(.005);
	pts.setOffset(new double[]{0,0,0});
	pts.setAlignment(SwingConstants.CENTER);
 
	SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	world.getAppearance().setAttribute(LINE_SHADER+"."+POLYGON_SHADER+"."+SMOOTH_SHADING, true);
	world.getAppearance().setAttribute(POLYGON_SHADER+"."+SMOOTH_SHADING, false);
	int numSamples = 8;
	for (int i = 0; i<numSamples; ++i)	{
		SceneGraphComponent child = SceneGraphUtility.createFullSceneGraphComponent("child"+i);
		world.addChild(child);
		// create the label at the center of the example
		SceneGraphComponent label = SceneGraphUtility.createFullSceneGraphComponent("label"+i);
		PointSetFactory labelFac = new PointSetFactory();
		labelFac.setVertexCount(1);
		labelFac.setVertexCoordinates(new double[]{0,0,0});
		labelFac.setVertexLabels(new String[]{""+(i+1)});
		labelFac.update();
		label.setGeometry(labelFac.getPointSet());
		label.setAppearance(labelAp);
		child.addChild(label);
		
		// set up the indexed line set for this example
		IndexedLineSetFactory lsf = new IndexedLineSetFactory();
		lsf.setVertexCount(numPoints);
		lsf.setVertexCoordinates(verts);
		if (i == 1) lsf.setVertexNormals(verts);	// outward pointing normals for lighting enabled 
		lsf.setEdgeCount(numPoints);
		lsf.setEdgeIndices(edges);
		lsf.setVertexColors(edgeColors);
		// add edge colors
		if (i == 4 || i == 5)	{
			lsf.setEdgeColors(edgeColors);
			if (i == 5)	{
				lsf.setEdgeAttribute(Attribute.RELATIVE_RADII, relrad);
			}
		}
		// make a line set with vertex colors and a single long edge
		// and show how to use the RELATIVE_RADII attribute to modulate size of tube
		else if (i == 6 || i == 7)	{
			lsf.setEdgeCount(1);
			lsf.setEdgeIndices(singleEdge);
			if (i == 7)	{
				lsf.setVertexAttribute(Attribute.RELATIVE_RADII, relrad);
			}
		}
		lsf.update();
		IndexedLineSet ils = lsf.getIndexedLineSet();			
		child.setGeometry(ils);
		
		// set up the line shader
		Appearance ap = child.getAppearance();
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowPoints(false);
		dgs.setShowLines(true);
		RenderingHintsShader rhs = ShaderUtility.createDefaultRenderingHintsShader(ap, true);
		DefaultLineShader dls = (DefaultLineShader) dgs.createLineShader("default");
		// this polygon shader controls how the tubes are rendered
		DefaultPolygonShader dpls = (DefaultPolygonShader) dls.createPolygonShader("default");
		MatrixBuilder.euclidean().translate(0,0,-i*.5).assignTo(child);
		switch(i)	{
			case 0:		// default rendering with bresenham lines
			case 1:		// bresenham again, but with "stippling" (dots and dashes) -JOGL only
				dls.setTubeDraw(false);
				dls.setLineWidth(2.0);
				if (i == 1) {	// activate stippling and lighting
					dls.setDiffuseColor(Color.red);
					dls.setLineLighting(true);
					dls.setLineStipple(true);
					dls.setLineStipplePattern(0xf0f0);
					dls.setLineFactor(2);	// scales the dots and dashes
				}
				break;
			case 2:		// ordinary opaque tubes
			case 3:		// transparency activated for the tubes coming from appearances. like these
			case 4:		// opaque tubes with edge colors
			case 5:		// opaque tubes with vertex colors and relative radii
			case 6:		// opaque tubes with vertex colors and a single, long edge
			case 7:		// same as 6 with a non-default cross section
				dls.setTubeDraw(true);		// the default
				dls.setTubeRadius(.05);
				if (i==7) ap.setAttribute("crossSection", verts);
				dpls.setTransparency(.6);
				dpls.setDiffuseColor(Color.cyan);
				rhs.setTransparencyEnabled(true);
				rhs.setOpaqueTubesAndSpheres(i != 3);
				if (i == 5 || i == 6 || i == 7)	{	// use vertex colors
					dls.setVertexColors(true);
				} 
				if (i == 6) dpls.setSmoothShading(false);
				break;
		}
	}
    	
		JRViewer.display(world);
  }
}
