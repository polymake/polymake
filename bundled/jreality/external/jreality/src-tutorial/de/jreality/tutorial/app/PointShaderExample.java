package de.jreality.tutorial.app;

import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.POINT_SPRITE;

import java.awt.Color;

import javax.swing.SwingConstants;

import de.jreality.backends.label.LabelUtility;
import de.jreality.geometry.PointSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.TextureUtility;
import de.jreality.util.SceneGraphUtility;

public class PointShaderExample {
  
  
/**
 * Features shown:
 * 	 flat (points)
 * 		1. with a sphere sprite
 * 	 	2. with a text sprite
 *   with true spheres
 *   	3. color from appearance, opaque
 *   	4. color from appearance, transparency enabled
 *   	5. color from vertices of point set
 *   	6. ditto, with relative radii allowing varying sizes of spheres
 *   
 * Notes:
 * 	1. These examples do not use point attenuation (see {@link DefaultPointShader#setAttenuatePointSize(Boolean)}.
 * 		When this is set to true, the sprites appear more realistic, but it is more difficult
 * 		to control and in this example, I didn't have time to fine tube it.
 *   2. Size of points (Examples 1 and 2) and spheres (the rest) are set independently
 *   	since the Attributes POINT_SIZE and POINT_RADIUS are specified in different coordinate
 *   	systems.  The former is in screen pixels, and the latter in object coordinates.
 *   	This is obviously inconvenient.  A good reliable solution is welcome.
 *   3. Colors specified in the shader are ALWAYS RGB values; alpha (transparency) values
 *   	must be set using the separate TRANSPARENCY attribute.  Vertex colors however,
 *   	are RGBA values -- see Example 5.
 *   4. This example program has been tested on the JOGL backend; other backends may not yield the
 *   	same behavior, particularly where rendering hints are involved.
 * @author gunn
 *
 */
public static void main(String[] args) {
	// prepare a circle of points to be used as the example geometry
	int numPoints = 20;
	double[][] verts = new double[numPoints][], 
		vcolors = new double[numPoints][];		// optional vertex colors
	double[] relrad = new double[numPoints];
	for (int i=0; i<numPoints; ++i)	{ 
		double angle = (Math.PI*2*i)/numPoints;
		verts[i] = new double[]{Math.cos(angle), Math.sin(angle), 0};
		// make color a linear mix of red and green, based on x- and y- coordinates
		vcolors[i] = new double[]{verts[i][0]*.5+.5, verts[i][1]*.5+.5,0.0, 0.5};
		relrad[i] = verts[i][0]+1.5;
	}
	
	// each sample gets a numeric label at its geometric center point.  Prepare the appearance.
	Appearance labelAp = new Appearance();
    DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(labelAp, false);
	dgs.setShowPoints(true);
    DefaultPointShader pointShader = (DefaultPointShader)dgs.getPointShader();
    pointShader.setPointRadius(.0001);
    pointShader.setSpheresDraw(true);
    pointShader.setDiffuseColor(Color.black);
	DefaultTextShader pts = (DefaultTextShader) (pointShader).getTextShader();
	pts.setScale(.005);
	pts.setOffset(new double[]{0,0,0});
	pts.setAlignment(SwingConstants.CENTER);

	// Prepare the six different examples
	SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	int numSamples = 6;
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

		// prepare the geometry
		PointSetFactory psf = new PointSetFactory();
		psf.setVertexCount(numPoints);
		psf.setVertexCoordinates(verts);
		if (i == 4 || i == 5)	{
			psf.setVertexColors(vcolors);
		}
		if (i == 5) 
			psf.setVertexAttribute(Attribute.RELATIVE_RADII, relrad);
		psf.update();
		PointSet ps = psf.getPointSet();
		child.setGeometry(ps);
		Appearance ap = child.getAppearance();
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowPoints(true);
		RenderingHintsShader rhs = ShaderUtility.createDefaultRenderingHintsShader(ap, true);
		DefaultPointShader dps = (DefaultPointShader) dgs.createPointShader("default");
		DefaultPolygonShader dpls = (DefaultPolygonShader) dps.createPolygonShader("default");
		MatrixBuilder.euclidean().translate(0,0,-i*.5).assignTo(child);
		switch(i)	{
			case 0:		// default rendering with sprites which look like spheres
			case 1:		// sprites again, but this shows an image of the string "SPR"
				dps.setSpheresDraw(false);
				dps.setAttenuatePointSize(true);	// quality is better when set to true
				dps.setPointSize(200.0);
				dps.setDiffuseColor(Color.yellow);
				if (i == 1)	{
					ImageData id = new ImageData(LabelUtility.createImageFromString("SPR", null, Color.white));
					TextureUtility.createTexture(ap, POINT_SHADER+"."+POINT_SPRITE,id);					
				}
				break;
			case 2:		// transparency activated, but not for spheres coming from appearances, like these
			case 3:		// transparency activated, and extends to the spheres coming from appearances. like these
			case 4:		// opaque spheres with vertex colors
			case 5:		// opaque spheres with vertex colors, relative radii
				dps.setSpheresDraw(true);		// the default
				dpls.setTransparency(.6);
				rhs.setTransparencyEnabled(i==3);
				rhs.setOpaqueTubesAndSpheres(i != 3);
				dps.setPointRadius(.05);
				break;
		}
	}
    	
		JRViewer.display(world);
  }
}
