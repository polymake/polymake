package de.jreality.tutorial.app;

import static de.jreality.shader.CommonAttributes.BACKGROUND_COLOR;
import static de.jreality.shader.CommonAttributes.BACKGROUND_COLORS;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

import de.jreality.geometry.SphereUtility;
import de.jreality.jogl.plugin.InfoOverlay;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.GlslPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.SceneGraphUtility;

public class GLVertexArrayExample {

	private static SceneGraphComponent world;
	private final static Appearance 
		vertexArrayAp = new Appearance(), 
		displayListAp = new Appearance();
	static boolean vertexArrays = false,
		displayLists = true;
	public static void main(String[] args)	{
		world = SceneGraphUtility.createFullSceneGraphComponent("world");
		world.setGeometry(SphereUtility.sphericalPatch(0, 0.0, 180.0, 90.0, 256, 256, 1.0));
		//set up two appearances, one that uses vertex arrays and one that doesn't
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(vertexArrayAp, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		dgs.setShowFaces(true);
		GlslPolygonShader tsps = (GlslPolygonShader) dgs.createPolygonShader("glsl");
		vertexArrayAp.setAttribute("diffuseColor", Color.red);

		dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(displayListAp, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		dgs.setShowFaces(true);
		
		world.setAppearance(vertexArrays ? vertexArrayAp : displayListAp);
		System.err.println("vertex arrays = "+vertexArrays);
		
//		ViewerApp va = new ViewerApp(world);
//		va.update();
//		va.display();
		Viewer viewer = JRViewer.display(world);
//		CameraUtility.encompass(va.getCurrentViewer());	
//		Viewer viewer = va.getCurrentViewer();
		viewer.getSceneRoot().getAppearance().setAttribute(BACKGROUND_COLOR, Color.black);
		viewer.getSceneRoot().getAppearance().setAttribute(BACKGROUND_COLORS,Appearance.INHERITED);
		// add an InfoOverlay to read off the frame rates
		if (viewer instanceof ViewerSwitch && (((ViewerSwitch) viewer).getCurrentViewer()) instanceof de.jreality.jogl.JOGLViewer) {
			InfoOverlay io =InfoOverlay.perfInfoOverlayFor();
			io.setInstrumentedViewer((de.jreality.jogl.JOGLViewer)(((ViewerSwitch) viewer).getCurrentViewer()));
			io.setVisible(true);
		}
		// add a key listener to allow toggling of vertexArrays and also displayLists
		Component comp = (Component) viewer.getViewingComponent();
		comp.addKeyListener(new KeyAdapter() {
 				public void keyPressed(KeyEvent e)	{ 
					switch(e.getKeyCode())	{
						
					case KeyEvent.VK_H:
						System.err.println("	1: toggle vertex arrays");
						System.err.println("	2: toggle display lists");
						break;
		
					case KeyEvent.VK_1:
						vertexArrays = !vertexArrays;
						world.setAppearance(vertexArrays ? vertexArrayAp : displayListAp);
						System.err.println("vertex arrays = "+vertexArrays);
						break;

					case KeyEvent.VK_2:
						displayLists = !displayLists;
						world.getAppearance().setAttribute(CommonAttributes.ANY_DISPLAY_LISTS, displayLists);
						System.err.println("display lists = "+displayLists);
						break;

				}
		
				}
			});

	}

}
