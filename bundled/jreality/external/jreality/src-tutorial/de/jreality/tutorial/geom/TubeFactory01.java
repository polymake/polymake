package de.jreality.tutorial.geom;

import java.awt.Color;

import de.jreality.geometry.PolygonalTubeFactory;
import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * A simple example of using a tubing factory
 * @author Charles Gunn
 *
 */
public class TubeFactory01 {

	private static SceneGraphComponent torussgc;
	
	public static void main(String[] args)	{
		torussgc = SceneGraphUtility.createFullSceneGraphComponent("torus knot");
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
		   		ShaderUtility.createDefaultGeometryShader(torussgc.getAppearance(), true);
		dgs.setShowLines(true);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		DefaultLineShader dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.01);
		IndexedLineSet torus1 = Primitives.discreteTorusKnot(1, .25, 2, 9, 250);
		PolygonalTubeFactory ptf = new PolygonalTubeFactory(torus1, 0);
		ptf.setClosed(true);
		ptf.setRadius(.1);
		ptf.setGenerateEdges(true);
		ptf.update();
		IndexedFaceSet torus1Tubes = ptf.getTube();
		torussgc.setGeometry(torus1Tubes);
		JRViewer.display(torussgc);
	}

	

}
