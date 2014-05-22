package de.jreality.tutorial.intro;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * This class contains the sixth in a series of 8 simple introductory examples which mimic the
 * functionality of the 
 * <a href="http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/User_Tutorial"> jReality User Tutorial 
 *</a>.  
 *
 *	Shows a ViewerApp with a white cylinder.
 * 
 * @author Charles Gunn
 *
 */
public class Intro06 {

	private static DefaultGeometryShader dgs;
	private static DefaultPolygonShader dps;

	public static void main(String[] args)	{
		SceneGraphComponent myscene = SceneGraphUtility.createFullSceneGraphComponent("myscene");
		myscene.setGeometry(Primitives.cylinder(50));
		Appearance ap = myscene.getAppearance();
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		JRViewer.display(myscene);
	}



}
