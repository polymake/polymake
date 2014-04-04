  package de.jreality.tutorial.viewer;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics2D;

import de.jreality.geometry.Primitives;
import de.jreality.jogl.GLJPanelViewer;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.SystemProperties;

/**
 * A simple class showing how to use a {@link GLJPanelViewer}, lightweight version
 * of the JOGL viewer which allows the user to mix ordinary Java 2D code with JOGL
 * 3D graphics.
 * <p>
 * To run the demo: you should see an icosahedron over a light green background.  
 * Rotate the icosahedron so that it continues to spin.  You should then see two
 * strings (from Java2D code) displayed, they move diagonally across the screen as 
 * the object rotates.  One (from the preRender() method) goes behind the icosahedron,
 * the other (from the postRender() method) goes in front of the icosahedron.
 * @author Charles Gunn
 *
 */
public class GLJPanelViewerExample {
	// set this to true to see how to add the viewing components of other JOGL viewers to the same panel
	static boolean showOtherViewers = false;
	public static void main(String[] args)	{
		// force the class to be used by JRViewer when constructing viewers
	    System.setProperty(SystemProperties.VIEWER, "de.jreality.jogl.GLJPanelViewer"); 
		SceneGraphComponent world = new SceneGraphComponent();
		world.setGeometry(Primitives.sharedIcosahedron);
	      
	    de.jreality.scene.Viewer v = JRViewer.display(world);
		//JRViewer va = JRViewer.getLastJRViewer();
	    // provide transparent background color, remove other background colors
		v.getSceneRoot().getAppearance().setAttribute("backgroundColor", new Color(0,255,0,128));
		v.getSceneRoot().getAppearance().setAttribute("backgroundColors", 
				Appearance.INHERITED);
		// awkward but currently only way to get the actual viewer instance we want
		final GLJPanelViewer glpv = (GLJPanelViewer) ((ViewerSwitch) v).getCurrentViewer();
		// add preRender() and postRender() methods to show how to mix Java2D
		// code with JOGL code
		glpv.addRenderListener(new GLJPanelViewer.GLJPanelListener() {
			Font font = new Font("Times", Font.BOLD, 32);
			int count = 0;
			public void postRender(Graphics2D g2) {
				if (g2 == null) return;
				g2.setColor(Color.pink);
				g2.setFont(font);
				g2.drawString("postRender", 10+count, 50+count);
				count++;
				count = count % 500;
			}

			public void preRender(Graphics2D g2) {
				if (g2 == null) return;
				g2.setColor(Color.blue);
				g2.setFont(font);
				g2.drawString("preRender", 10+count, 100+count);
			}
			
		});
		if (showOtherViewers)	{
		    System.setProperty(SystemProperties.VIEWER, "de.jreality.jogl.JOGLViewer"); 
			world = new SceneGraphComponent();
			world.setGeometry(Primitives.coloredCube());
		    de.jreality.scene.Viewer viewer  = JRViewer.display(world);
		    viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0,0,0,0));
		    viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLORS, Appearance.INHERITED);
		    Component comp = (Component) viewer.getViewingComponent();
		    comp.setSize(new Dimension(300, 200));
		    comp.repaint();
		    comp.validate();
		    glpv.getPanel().add(comp);
			world = new SceneGraphComponent();
			world.setAppearance(new Appearance());
		    world.getAppearance().setAttribute("diffuseColor", Color.white);
			world.setGeometry(Primitives.torus(1, .3, 20,20));
		    de.jreality.scene.Viewer viewer2  = JRViewer.display(world);
		    Component comp2 = (Component) viewer2.getViewingComponent();
		    comp2.setSize(new Dimension(300, 200));
		    comp2.repaint();
		    comp2.validate();
		    glpv.getPanel().add(comp2);
		    comp2.setLocation(500, 300);			
		}
//	in current configuration this doesn't work ...
//        JFrame f = new JFrame();
//        f.getContentPane().add((Component) glpv.getViewingComponent());
//
//        f.setSize(512, 512);
//        f.validate();
//        f.setVisible(true);
	}
 
}
