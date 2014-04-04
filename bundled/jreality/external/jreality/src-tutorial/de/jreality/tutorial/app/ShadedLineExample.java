package de.jreality.tutorial.app;

import java.awt.Color;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.FrameFieldType;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.geometry.PointSetUtility;
import de.jreality.geometry.PolygonalTubeFactory;
import de.jreality.geometry.Primitives;
import de.jreality.geometry.TubeFactory;
import de.jreality.geometry.TubeUtility.FrameInfo;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPolygonShader;
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
public class ShadedLineExample { 
  
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
	
 
	SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	SceneGraphComponent child = SceneGraphUtility.createFullSceneGraphComponent("child");
	world.addChild(child);
	// create the label at the center of the example

	IndexedLineSet tknot =Primitives.discreteTorusKnot(1.0, 0.6, 51, 91, 25000);
	double[][] pts = tknot.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
	tknot.setVertexAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array().createReadOnly(calculateCurveNormals(tknot)));
	child.setGeometry(tknot);

	SceneGraphComponent sgc = PointSetUtility.displayVertexNormals(tknot, .5, 0);
//	world.addChild(sgc);
	sgc.getAppearance().setAttribute("lineShading", false);
	// set up the line shader
	Appearance ap = child.getAppearance();
	DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
	dgs.setShowPoints(false);
	dgs.setShowLines(true);
	RenderingHintsShader rhs = ShaderUtility.createDefaultRenderingHintsShader(ap, true);
	DefaultLineShader dls = (DefaultLineShader) dgs.createLineShader("default");
	dls.setTubeDraw(false);
	dls.setLineWidth(3.0);
	dls.setLineLighting(true);

	// this polygon shader controls how the lines are rendered when line lighting is true
	DefaultPolygonShader dpls = (DefaultPolygonShader) dls.createPolygonShader("default");
	dpls.setDiffuseCoefficient(.5);
	dpls.setSpecularCoefficient(1.0);
	dpls.setDiffuseColor(new Color(100,100,255));
	dpls.setSpecularColor(Color.white);

	Viewer viewer = JRViewer.display(world);
	viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLOR, Color.black);
	viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLORS, Appearance.INHERITED);
  }
  
  private static double[][] calculateCurveNormals(IndexedLineSet ifs)	{
	  TubeFactory tf = new PolygonalTubeFactory(ifs, 0);
	  tf.setFrameFieldType(FrameFieldType.FRENET);
	  tf.setClosed(true);
	  tf.update();
	  FrameInfo[] frames = tf.getFrameField();
	  int n = frames.length-1;
	  double[][] nn = new double[n][3];
	  for (int i = 0; i<n; ++i)	{
		  double[] frame = Rn.transpose(null, frames[i].frame);
		  System.arraycopy(frame, 0, nn[i], 0, 3);
	  }
	  Rn.times(nn, -1, nn);
	  return nn;
  }
}
