/*
 * Created on Jun 17, 2004
 *
 */
package de.jreality.plugin.experimental;

import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.event.InputEvent;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.logging.Level;

import javax.swing.JColorChooser;
import javax.swing.KeyStroke;

import de.jreality.geometry.FrameFieldType;
import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.plugin.HelpOverlay;
import de.jreality.jogl.plugin.InfoOverlay;
import de.jreality.math.P3;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.StereoViewer;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.SelectionManagerImpl;
import de.jreality.util.CameraUtility;
import de.jreality.util.DefaultMatrixSupport;
import de.jreality.util.LoggingSystem;
import de.jreality.util.SceneGraphUtility;
import de.jtem.jrworkspace.plugin.annotation.Experimental;

/**
 * @author Charles Gunn
 *
 */
@Experimental
public class ViewerKeyListener extends KeyAdapter {
	Viewer viewer;
	de.jreality.jogl.JOGLViewer jViewer = null;
	StereoViewer stereoViewer = null;
	SelectionManager sm;
	boolean motionToggle = false;
	boolean fullScreenToggle = false;
	HelpOverlay helpOverlay;
	InfoOverlay infoOverlay;
	SceneGraphPath selection = null;
	
	public ViewerKeyListener(Viewer v, HelpOverlay ho, InfoOverlay io) {
		viewer = v;
//		System.err.println("viewer is "+viewer.getClass().getName());
		if (viewer instanceof de.jreality.jogl.JOGLViewer) {
			jViewer = (de.jreality.jogl.JOGLViewer) viewer;
		}
		if (viewer instanceof StereoViewer) {
			stereoViewer = (StereoViewer) viewer;
		}
		sm = SelectionManagerImpl.selectionManagerForViewer(v);
		selection = sm.getSelection().getSGPath();
		
//		if (sm instanceof SelectionManager) 
//			ism = (SelectionManager) sm;
		if (jViewer != null) {
			if (ho != null)
				helpOverlay = ho;
			else
				helpOverlay = new HelpOverlay(jViewer);
		}
		if (jViewer != null) {
			if (io != null)	
				infoOverlay = io;
			else {
				infoOverlay = InfoOverlay.perfInfoOverlayFor();
				infoOverlay.setInstrumentedViewer(jViewer);
			}
		}
		if (helpOverlay != null)	{
			// numeric keys are reserved for applications
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_A,0), "Increase alpha (1-transparency)");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_A,InputEvent.SHIFT_DOWN_MASK), "Decrease alpha");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_B,0), "Toggle backplane display");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_B,InputEvent.SHIFT_DOWN_MASK), "Toggle selection bound display");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_C,0), "Set polygon diffuse color in selected appearance");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_C,InputEvent.SHIFT_DOWN_MASK), "Set background color");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_D,0), "Toggle use of display lists (JOGL)");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_E,0), "Encompass");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_E,InputEvent.SHIFT_DOWN_MASK), "Toggle edge drawing");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_F,0), "Flip normals");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_F,InputEvent.SHIFT_DOWN_MASK), "Toggle face drawing");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_G,InputEvent.SHIFT_DOWN_MASK), "Toggle parallel/frenet tubes");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_H,0), "Toggle help overlay");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_H,InputEvent.SHIFT_DOWN_MASK), "Halt all motions");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_I,0), "Toggle info overlay");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_I,InputEvent.SHIFT_DOWN_MASK), "Toggle continued motion in tools");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_J,0), "Increase sphere radius");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_J,InputEvent.SHIFT_DOWN_MASK), "Decrease sphere radius");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_K,0), "Cycle through selection list");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_L,0), "Toggle lighting enabled");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_M,0), "Reset Matrices to default");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_M,InputEvent.SHIFT_DOWN_MASK), "Set default Matrices with current state");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_N,0), "Add current selection to selection list");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_N,InputEvent.SHIFT_DOWN_MASK), "Remove current selection from selection list");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_O,0), "Update selection from selection manager");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_P,0), "Toggle perspective/orthographic view");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_P,InputEvent.SHIFT_DOWN_MASK), "Toggle fog");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Q,0), "Toggle interpolate vertex colors in line shader");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Q,InputEvent.SHIFT_DOWN_MASK), "Orthonormalize camera transform");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_R,0), "Activate rotation tool");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_R, InputEvent.SHIFT_DOWN_MASK), "Toggle GLSL shader");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_S,0), "Toggle smooth shading");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_S,InputEvent.SHIFT_DOWN_MASK), "Toggle sphere drawing");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_T,0), "Activate translation tool");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_T,InputEvent.SHIFT_DOWN_MASK), "Toggle tube drawing");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_U,0), "Increase fog factor");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_U,InputEvent.SHIFT_DOWN_MASK), "Decrease fog factor");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_V,0), "Print frame rate");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_V,InputEvent.SHIFT_DOWN_MASK), "Toggle vertex drawing");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_W,0), "Increase line width/ tube radius");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_W,InputEvent.SHIFT_DOWN_MASK), "Decrease line width/tube radius");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_X,0), "Toggle transparency enabled");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_X,InputEvent.SHIFT_DOWN_MASK), "Toggle thread-safe");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Y,0), "Activate selection tool");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Y,InputEvent.SHIFT_DOWN_MASK), "Activate stereo tool");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Z,0), "Toggle stereo/mono camera");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_Z,InputEvent.SHIFT_DOWN_MASK), "Cycle stereo modes");
//			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_COMMA,0), "Toggle fullscreen mode");
			helpOverlay.registerKeyStroke(KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE,0), "Quit");
			//if ((viewer.getViewingComponent() instanceof GLCanvas))			
		}
		if (jViewer != null) jViewer.getDrawable().addGLEventListener(helpOverlay);
	}

	Container c = null;
    boolean encompassToggle = true;
	@Override
	public void keyPressed(KeyEvent e)	{
			//System.err.println("handling keyboard event");
			if (e.isControlDown() || e.isMetaDown()) return;
			switch(e.getKeyCode())	{
			
				case KeyEvent.VK_A:		// transparency
					if (e.isAltDown())  
						if (!e.isShiftDown()) toggleValue(viewer, "clearColorBuffer", viewer.getSceneRoot().getAppearance());
					else modulateValueAdditive(CommonAttributes.TRANSPARENCY,  CommonAttributes.TRANSPARENCY_DEFAULT, .05, 0.0, 1.0, e.isShiftDown());
					break;

				case KeyEvent.VK_B:		// toggle backplane
					if (e.isShiftDown()) {
						sm.setRenderSelection( !sm.isRenderSelection());
					} else
						toggleBackPlane();
					viewer.renderAsync();
					break;

				case KeyEvent.VK_C:		// select a color
					java.awt.Color color = JColorChooser.showDialog((Component) viewer.getViewingComponent(), "Select background color",  null);
					if (color == null) break;
					if (e.isShiftDown())	
						viewer.getSceneRoot().getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLOR, color);
					else  {
						if (getSelectedAppearance() == null )return;
						getSelectedAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
//						getSelectedAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
//						getSelectedAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
					}
					viewer.renderAsync();
					break;
					
				case KeyEvent.VK_D:		// toggle use of display lists
					if (e.isShiftDown()) break;
					toggleValue(viewer, CommonAttributes.ANY_DISPLAY_LISTS, viewer.getSceneRoot().getAppearance());
					viewer.renderAsync();
					break;

				case KeyEvent.VK_E:		
					if (!e.isShiftDown()) 	{
						CameraUtility.encompass(viewer);
						encompassToggle = !encompassToggle;
					}
					else				toggleValue(CommonAttributes.EDGE_DRAW);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_F:		// toggle face drawing
					if (e.isShiftDown())		toggleValue(CommonAttributes.FACE_DRAW);
					else 
						toggleValue(CommonAttributes.FLIP_NORMALS_ENABLED);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_G:		// toggle face drawing
					if (e.isShiftDown())		toggleValue(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_STYLE);
					//else toggleValue(viewer,CommonAttributes.FOG_ENABLED, viewer.getSceneRoot().getAppearance());
					viewer.renderAsync();
					break;

				case KeyEvent.VK_H:		// toggle help
//					if (e.isShiftDown()) MotionManager.motionManagerForViewer(viewer).clearMotions();
//					else 
					{
						helpOverlay.printOut();
						helpOverlay.setVisible(!helpOverlay.isVisible());
					}
					viewer.renderAsync();
					break;

				case KeyEvent.VK_I:		// toggle info
//					if (e.isShiftDown()) MotionManager.motionManagerForViewer(viewer).setAllowMotion(
//							!MotionManager.motionManagerForViewer(viewer).isAllowMotion());
//					else 
						infoOverlay.setVisible(!infoOverlay.isVisible());
					viewer.renderAsync();
					break;

				case KeyEvent.VK_J:		// line width
					modulateValue(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, 0.05,!e.isShiftDown());
					modulateValue(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_SIZE, 2.0,!e.isShiftDown());
				    viewer.renderAsync();
					break;

				case KeyEvent.VK_K:		
					if (e.isShiftDown()) break;
					sm.cycleSelection();
					viewer.renderAsync();
					break;

				case KeyEvent.VK_L:		// toggle lighting
					if (e.isShiftDown()) break;
					toggleValue(CommonAttributes.LIGHTING_ENABLED);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_M:		// reset matrices
					if (e.isShiftDown())
						DefaultMatrixSupport.getSharedInstance().storeDefaultMatrices(viewer.getSceneRoot());
					else  {
//						MotionManager.motionManagerForViewer(viewer).stopMotions();
						DefaultMatrixSupport.getSharedInstance().restoreDefaultMatrices(viewer.getSceneRoot(), true);
					}
					viewer.renderAsync();
					break;

				case KeyEvent.VK_N:	
						if (e.isShiftDown()) sm.removeSelection(sm.getSelection().getSGPath());
						else sm.addSelection(sm.getSelection().getSGPath());
						viewer.renderAsync();
					break;

				case KeyEvent.VK_O:		
					selection = new SceneGraphPath(sm.getSelection().getSGPath());
					viewer.renderAsync();
					break;

				case KeyEvent.VK_P:		// toggle perspective
					if (e.isShiftDown()) break;
					boolean val = CameraUtility.getCamera(viewer).isPerspective();
					CameraUtility.getCamera(viewer).setPerspective(!val);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_Q:		
					if (e.isShiftDown()){
						int metric = SceneGraphUtility.getMetric(viewer.getCameraPath());		
						Transformation tt =  CameraUtility.getCameraNode(viewer).getTransformation();
						double[] clean = P3.orthonormalizeMatrix(null, tt.getMatrix(), 10E-10, metric);
						if (clean != null)	tt.setMatrix(clean);
					}
					else toggleValue(CommonAttributes.LINE_SHADER+"."+CommonAttributes.SMOOTH_LINE_SHADING);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_R:		// activate rotation tool
					if (e.isShiftDown()) toggleValue("useGLSL");
					break;
				
				case KeyEvent.VK_S:		//smooth shading
					if (e.isShiftDown()) toggleValue(CommonAttributes.POINT_SHADER+"."+CommonAttributes.SPHERES_DRAW);
					else {
						toggleValue(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.SMOOTH_SHADING);
					}
				    viewer.renderAsync();
					break;

				case KeyEvent.VK_T:		// activate translation tool
					if (e.isShiftDown()) toggleValue(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW);
//					else if (tm != null) tm.activateTool(ToolManager.TRANSLATION_TOOL);
				    viewer.renderAsync();
					break;
				
				case KeyEvent.VK_U:		// line width
					modulateValue(viewer, "fogDensity", .2, !e.isShiftDown(), 1.2, viewer.getSceneRoot().getAppearance());
				    viewer.renderAsync();
					break;

				case KeyEvent.VK_V:		// draw vertices
					if (e.isShiftDown()) 					toggleValue(CommonAttributes.VERTEX_DRAW);
					viewer.renderAsync();
					break;

				case KeyEvent.VK_W:		// line width
					modulateValue(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_WIDTH, 1.0, !e.isShiftDown());
					modulateValue(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, .02, !e.isShiftDown());
				    viewer.renderAsync();
					break;

				case KeyEvent.VK_X:		// toggle fast and dirty
					if (e.isShiftDown()) {
						SceneGraphNode.setThreadSafe(!SceneGraphNode.getThreadSafe());
						System.err.println("Thread safe is now "+SceneGraphNode.getThreadSafe());
					}
					else toggleValue(CommonAttributes.TRANSPARENCY_ENABLED);
					break;
					
				case KeyEvent.VK_Y:		// activate tool
//					if (e.isShiftDown()) {	
//						if (tm != null) tm.activateTool(ToolManager.CAMERA_STEREO_TOOL);
//					} else
//						if (tm != null) tm.activateTool(ToolManager.SELECTION_TOOL);
					break;
				
				
				case KeyEvent.VK_Z:		
					if (e.isShiftDown() && stereoViewer != null) {		// cycle stereo types
						int which = stereoViewer.getStereoType();
						which = (which+1) % de.jreality.jogl.JOGLViewer.STEREO_TYPES;
						jViewer.setStereoType(which);						
					} else {						// toggle stereo/mono
						Camera cam = CameraUtility.getCamera(viewer);
						cam.setStereo(!cam.isStereo());
					}
					viewer.renderAsync();
					break;
				
				case KeyEvent.VK_ESCAPE:		// toggle lighting
					if (e.isShiftDown()) break;
					System.exit(0);
					break;

			}
		}

	public void setSelection(SceneGraphPath selection) {
		this.selection = new SceneGraphPath(selection);
	}

	static Color[] defaultCorners = { new Color(.5f,.5f,1f), new Color(.5f,.5f,.5f),new Color(1f,.5f,.5f),new Color(.5f,1f,.5f) };
	Color[] savedColors = null;
	boolean firstTime = true, showColors = false;
	private void toggleBackPlane() {
	  		Appearance rootAp = viewer.getSceneRoot().getAppearance();
	  		if (rootAp == null) {
	  			rootAp = new Appearance();
	  			viewer.getSceneRoot().setAppearance(rootAp);
	  		}
	  		if (firstTime)	{
				Object bp = rootAp.getAttribute(CommonAttributes.BACKGROUND_COLORS);
				if (bp != null && bp instanceof Color[])	{
					savedColors = (Color[]) bp;
					showColors = true;
				} else savedColors = defaultCorners;
	  			firstTime = false;
	  		}
	  		showColors = !showColors;
	  		rootAp.setAttribute(CommonAttributes.BACKGROUND_COLORS, showColors ? savedColors : Appearance.INHERITED);
	}

	private void toggleValue(String  name)	{
		toggleValue(viewer, name,getSelectedAppearance());
	}
	
	public  void toggleValue(Viewer viewer, String  name)	{
		Appearance ap = getSelectedAppearance();
		toggleValue(viewer, name, ap);
	}
		
	public  static void toggleValue(Viewer viewer, String  name, Appearance ap)	{

		if (ap == null) return;
		Object obj = ap.getAttribute(name);
		if (obj != null && obj instanceof Boolean)	{
			boolean newVal = true;
			newVal = !((Boolean) obj).booleanValue();
			LoggingSystem.getLogger("de.jreality.plugin.experimental").log(Level.INFO,"Turning "+(newVal ? "on" : "off")+" property "+name);
			ap.setAttribute(name, newVal);
			viewer.renderAsync();
			return;
		} else if (name.indexOf(CommonAttributes.TUBE_STYLE) != -1)	{
			FrameFieldType newV = FrameFieldType.PARALLEL;
			if (obj != null && obj instanceof FrameFieldType)		{
				if (obj == FrameFieldType.PARALLEL)	newV = FrameFieldType.FRENET;
				else newV = FrameFieldType.PARALLEL;
			}
			ap.setAttribute(name, newV);
			viewer.renderAsync();
			return;
		}
		JOGLConfiguration.getLogger().log(Level.INFO,"Turning on property "+name);
		ap.setAttribute(name, true);
		viewer.renderAsync();
	}

	private void modulateValue(String name, double val, boolean increase)	{
		Appearance ap = getSelectedAppearance();
		modulateValue(viewer, name, val, increase, 1.2, ap);
	}
	
	public static void modulateValue(Viewer viewer, String name, double val, boolean increase, double factor, Appearance ap)	{
		//Appearance ap = getSelectedAppearance();
		if (ap == null) return;
		Object obj = ap.getAttribute(name);
		double newVal = val;
		if (obj != null && obj instanceof Double)	{
			newVal = ((Double) obj).doubleValue();
			if (increase) newVal *= factor;
			else newVal /= factor;
		}
		JOGLConfiguration.getLogger().log(Level.INFO,"Setting value "+name+"Object is "+obj+"New value is "+newVal);
		System.err.println("Setting value "+name+"Object is "+obj+"New value is "+newVal);
			
		ap.setAttribute(name, newVal);
		
		viewer.renderAsync();		
	}

	/**
	 * @param string
	 * @param d
	 * @param e
	 * @param f
	 * @param g
	 * @param b
	 */
	public void modulateValueAdditive(String name, double def, double inc, double min, double max, boolean increase) {
		modulateValueAdditive(viewer, name, def, inc, min, max, increase);
	}
		
	public  static void modulateValueAdditive(Viewer viewer, String name, double defawlt, double inc, double min, double max, boolean increase) {
		SceneGraphPath selection = SelectionManagerImpl.selectionManagerForViewer(viewer).getSelectionPath();
		Appearance ap = SceneGraphUtility.findDeepestAppearance(selection);
		if (ap == null) return;
		modulateValueAdditive(ap, name, defawlt, inc, min, max, increase);
//		viewer.renderAsync();		
	}

	public  static void modulateValueAdditive(Appearance ap, String name, double defawlt, double inc, double min, double max, boolean increase) {
		Object obj = ap.getAttribute(name);
		double newVal = defawlt;
		if (obj != null && obj instanceof Double)	{
			newVal = ((Double) obj).doubleValue();
			if (increase) newVal +=  inc;
			else newVal -= inc;
		}
		//System.err.println("Setting value "+name+"Object is "+obj+"New value is "+newVal);
		if (newVal < min) newVal = min;
		if (newVal > max) newVal = max;
		ap.setAttribute(name, newVal);
		System.err.println("Setting value "+name+"Object is "+obj+"New value is "+newVal);
		
	}

	private Appearance getSelectedAppearance()	{
//		System.err.println("Selection is  "+sm.getSelection().getSGPath());
//		return SelectionManager.findDeepestAppearance(sm.getSelection().getSGPath());
		selection = SelectionManagerImpl.selectionManagerForViewer(viewer).getSelectionPath();
		return SceneGraphUtility.findDeepestAppearance(selection);
	}

	public static void setDefaultCorners(Color[] defaultCorners) {
		ViewerKeyListener.defaultCorners = defaultCorners;
	}
	
}
