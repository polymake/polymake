package de.jreality.tutorial.viewer;

import static de.jreality.plugin.icon.ImageHook.getIcon;

import java.awt.Component;

import javax.swing.JFrame;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.View;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;

/**
 * A simple class showing how to use a {@link JRViewer} to get a viewing component 
 * which is then packed into another frame.
 * @author Charles Gunn
 *
 */
public class SimpleViewer {

	static boolean local = true;
	public static void main(String[] args)	{
		SceneGraphComponent world = new SceneGraphComponent();
		world.setGeometry(Primitives.sharedIcosahedron);
		View.setTitle("The Icosahedron");
		View.setIcon(getIcon("color_swatch.png"));
		JRViewer v = JRViewer.createJRViewer(world);
		
		if (!local) {
			v.startup();
		} else {
			v.startupLocal();
			Viewer viewer = v.getViewer();
	        JFrame f = new JFrame();
	        f.getContentPane().add((Component)viewer.getViewingComponent());
	        f.setSize(512, 512);
	        f.validate();
	        f.setVisible(true);
		}
	}
 
}
