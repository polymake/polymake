package de.jreality.tutorial.intro;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.SceneGraphUtility;

/**
 * This class contains the eighth in a series of 8 simple introductory examples which mimic the
 * functionality of the 
 * <a href="http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/User_Tutorial"> jReality User Tutorial 
 *</a>.  
 *
 * Shows a ViewerApp with a scaled, colored cube.
 * 
 * @author Charles Gunn
 *
 */
public class Intro08 {

	public static void main(String[] args)	{
		SceneGraphComponent myscene = SceneGraphUtility.createFullSceneGraphComponent("myscene");
		myscene.setGeometry(Primitives.coloredCube());
		MatrixBuilder.euclidean().scale(2,.8,1).assignTo(myscene);
		JRViewer.display(myscene);
	}

}
