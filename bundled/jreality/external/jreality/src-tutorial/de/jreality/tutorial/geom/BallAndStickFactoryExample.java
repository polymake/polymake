package de.jreality.tutorial.geom;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;

/**
 * An example of using the {@link BallAndStickFactory}.
 * @author Charles Gunn
 *
 */public class BallAndStickFactoryExample {

	public static void main(String[] args)	{
	   BallAndStickFactory basf = new BallAndStickFactory(Primitives.sharedIcosahedron);
	   basf.setBallRadius(.04);
	   basf.setStickRadius(.02);
	   basf.setShowArrows(true);
	   basf.setArrowScale(.1);
	   basf.setArrowSlope(3);
	   basf.setArrowPosition(.6);
	   basf.update();
	   SceneGraphComponent tubedIcosa = basf.getSceneGraphComponent();
	   JRViewer.display(tubedIcosa);
	}
}
