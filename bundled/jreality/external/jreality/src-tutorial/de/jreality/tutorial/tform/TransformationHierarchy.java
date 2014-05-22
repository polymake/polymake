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
 * in that instance.  In this case the scene graph is constructed hierarchically, with a branching number of 2.
 * 
 * We use direct calls to {@link Appearance#setAttribute(String, Object)} instead of using shader interfaces.
 * @author gunn
 *
 */
public class TransformationHierarchy {

	 public static void main(String[] args) throws IOException {
		Color[] faceColors = {new Color(100, 200, 100), new Color(100, 100, 200), new Color(100,200,200), new Color(200,100,100)};
	    IndexedFaceSet ico = Primitives.sharedIcosahedron;
	    SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	    // set up the colors and sizes for edge and point rendering
		Appearance ap = world.getAppearance();
		setupAppearance(ap);
		SceneGraphComponent sgc0, sgc1, sgc2;
	    for (int i = 0; i<2; ++i)	{
			sgc0 = SceneGraphUtility.createFullSceneGraphComponent("sgc"+i);
			MatrixBuilder.euclidean().translate(-2+4*i, 0, 0).assignTo(sgc0);
			world.addChild(sgc0);
	    	for (int j = 0; j<2; ++j)	{
    			sgc1 = SceneGraphUtility.createFullSceneGraphComponent("sgc"+i+j);
    			MatrixBuilder.euclidean().translate(0,-2+4*j, 0).assignTo(sgc1);
    			sgc0.addChild(sgc1);
	    		for (int k = 0; k<2; ++k)	{
	    			sgc2 = SceneGraphUtility.createFullSceneGraphComponent("sgc"+i+j+k);
	    			sgc1.addChild(sgc2);
	    			// set translation onto corner of a cube
	    			MatrixBuilder.euclidean().translate(0, 0, -2+4*k).scale(1.5).assignTo(sgc2);
	    			// set same geometry 
	    			sgc2.setGeometry(ico);
	    			// set appearance individually
	    			sgc2.getAppearance().setAttribute(DIFFUSE_COLOR, faceColors[2*j+k]);
	    			DefaultGeometryShader dgs;
	    			dgs = ShaderUtility.createDefaultGeometryShader(sgc2.getAppearance(), true);
	    			dgs.setShowFaces(i==0);
	    			dgs.setShowLines(j==0);
	    			dgs.setShowPoints(k==0);
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
