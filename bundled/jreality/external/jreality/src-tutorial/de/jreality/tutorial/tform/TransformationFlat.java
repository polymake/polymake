package de.jreality.tutorial.tform;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;

import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * This tutorial demonstrates how the same geometry can be reused in different {@link de.jreality.scene.SceneGraphComponent} instances,
 * each time being rendered according to the {@link de.jreality.scene.Transformation} and {@link de.jreality.scene.Appearance} contained
 * in that instance. In this case the scene graph is flat, with 8 children below the  root.
 * 
 * We use direct calls to {@link Appearance#setAttribute(String, Object)} instead of using shader interfaces.
 * @author gunn
 *
 */
public class TransformationFlat {

	 public static void main(String[] args) throws IOException {
		Color[] faceColors = {new Color(100, 200, 100), new Color(100, 100, 200), new Color(100,200,200), new Color(200,100,100)};
	    IndexedFaceSet ico = Primitives.sharedIcosahedron;
	    SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	    // set up the colors and sizes for edge and point rendering
		Appearance ap = world.getAppearance();
		setupAppearance(ap);
	    for (int i = 0; i<2; ++i)	{
	    	for (int j = 0; j<2; ++j)	{
	    		for (int k = 0; k<2; ++k)	{
	    			SceneGraphComponent sgc = SceneGraphUtility.createFullSceneGraphComponent("sgc"+i+j+k);
	    			// set translation onto corner of a cube
	    			MatrixBuilder.euclidean().translate(-2+4*i, -2+4*j, -2+4*k).scale(1.5).assignTo(sgc);
	    			// set same geometry 
	    			sgc.setGeometry(ico);
	    			// set appearance individually
	    			sgc.getAppearance().setAttribute(DIFFUSE_COLOR, faceColors[2*j+k]);
	    			DefaultGeometryShader dgs;
	    			dgs = ShaderUtility.createDefaultGeometryShader(sgc.getAppearance(), true);
	    			dgs.setShowFaces(i==0);
	    			dgs.setShowLines(j==0);
	    			dgs.setShowPoints(k==0);
	    			world.addChild(sgc);
	    		}
	    	}
	    }
	    JRViewer.display(world);
	}

	private static void setupAppearance(Appearance ap) {
		DefaultGeometryShader dgs;
		DefaultLineShader dls;
		DefaultPointShader dpts;
		RenderingHintsShader rhs;
		DefaultPolygonShader dps;
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(true);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.03);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.05);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setSmoothShading(false);
		dps.setTransparency(.5);
		rhs = ShaderUtility.createDefaultRenderingHintsShader(ap, true);
		rhs.setTransparencyEnabled(true);
		rhs.setOpaqueTubesAndSpheres(true);
	}

}
