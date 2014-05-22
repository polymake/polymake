package de.jreality.plugin.scene;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.lang.ref.WeakReference;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.IndexedLineSetUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.RunningEnvironment;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.portal.PortalCoordinateSystem;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.swing.JFakeFrameWithGeometry;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class WindowManager extends Plugin implements ChangeListener {

	RunningEnvironment env;
	
	SceneGraphComponent parent;
	
	SceneGraphComponent windowRoot=new SceneGraphComponent("window root");
	public SceneGraphComponent getWindowRoot() {
		return windowRoot;
	}

	private int
		screenResolutionXPortal=1600,//1600 for CAVE
		screenResolutionX=1920, 
		screenResolutionY=1200;
	
	public int getScreenResolutionX() {
		return screenResolutionX;
	}

	public void setScreenResolutionX(int screenResolutionX) {
		this.screenResolutionX = screenResolutionX;
	}

	public int getScreenResolutionY() {
		return screenResolutionY;
	}

	public void setScreenResolutionY(int screenResolutionY) {
		this.screenResolutionY = screenResolutionY;
	}

	private double screenWidth=2.4798528;//2.48;
	private double screenHeight=1.9997928;//2.0;
	private double screenHeightOffset = 0.4;
	
	private boolean showDesktopBorder=false;
	SceneGraphComponent desktopBorder=new SceneGraphComponent("desktop bounds");
	ActionListener portalScaleListener;
	
	List<WeakReference<JFakeFrameWithGeometry>> frameRefs = new LinkedList<WeakReference<JFakeFrameWithGeometry>>();
	
	public WindowManager() {
		desktopBorder.setAppearance(new Appearance());
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(desktopBorder.getAppearance(), false);
		DefaultLineShader dls = (DefaultLineShader) dgs.getLineShader();
		dls.setTubeRadius(1.0);
		if (env != RunningEnvironment.DESKTOP){
			screenResolutionX=screenResolutionXPortal;//setting portal conditions
		}
		desktopBorder.setGeometry(IndexedLineSetUtility.createCurveFromPoints(new double[][]{{0,0,0},{screenResolutionX,0,0},{screenResolutionX,screenResolutionY,0},{0,screenResolutionY,0}}, true));
	}
	
	private void updateWindowRootTransformation(double near) {
		if (env != RunningEnvironment.DESKTOP) {
			double yTranslation = 0;
			double ps = 1.0;
			ps = PortalCoordinateSystem.getPortalScale();
			yTranslation = ps*(screenHeight/2+ screenHeightOffset);
			MatrixBuilder.euclidean().translate(0,yTranslation,-ps*screenWidth/2.0).scale(ps*screenWidth/screenResolutionX).rotateX(Math.PI).translate(-screenResolutionX/2, -screenResolutionY/2, 0).assignTo(windowRoot);
		} else {
			double tan = Math.tan(Math.PI/6); //assuming field of view == pi/3
			near *= 1.1; // move behind the near clipping plane
			double scale = 2*tan*near/screenResolutionY;
			MatrixBuilder.euclidean().translate(0, 0, -near).scale(scale).rotateX(Math.PI).translate(-screenResolutionX/2, -screenResolutionY/2, 0).assignTo(windowRoot);
		}
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Window Manager", "jReality Group");
	}
	@Override
	public void install(Controller c) throws Exception {
		env = c.getPlugin(View.class).getRunningEnvironment();
		double near = c.getPlugin(Scene.class).getCameraComponent().getCamera().getNear();
		updateWindowRootTransformation(near);
		updateParent(c.getPlugin(Scene.class));
		c.getPlugin(Scene.class).addChangeListener(this);
		setShowDesktopBorder(getShowDesktopBorder());
		if (env != RunningEnvironment.DESKTOP)
			PortalCoordinateSystem.addChangeListener(portalScaleListener = new ActionListener() {
			
			public void actionPerformed(ActionEvent arg0) {
				updateWindowRootTransformation(-1);
				System.err.println("WM: Updating portal scale");
			}
		});
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		c.getPlugin(Scene.class).removeChangeListener(this);
		if (env != RunningEnvironment.DESKTOP && portalScaleListener != null)
			PortalCoordinateSystem.removeChangeListener(portalScaleListener);
		frameRefs.clear();
		setParent(null);
	}

	private void updateParent(Scene scene) {
		SceneGraphComponent newParent = null;
		if (env == RunningEnvironment.DESKTOP) {
			newParent = scene.getCameraComponent();
		} else {
			newParent = scene.getAvatarComponent();
		}
		setParent(newParent);
	}

	private void setParent(SceneGraphComponent newParent) {
		if (parent == newParent) return;
		if (parent != null) parent.removeChild(windowRoot);
		if (newParent != null) newParent.addChild(windowRoot);
		parent = newParent;
	}
	
	@SuppressWarnings("serial")
	public JFakeFrameWithGeometry createFrame(String title) {
		JFakeFrameWithGeometry ff = new JFakeFrameWithGeometry(title) {
			@Override
			public void toFront() {
				int curLayer=0;
				for (Iterator<WeakReference<JFakeFrameWithGeometry>> frames = frameRefs.iterator(); frames.hasNext(); ) {
					JFakeFrameWithGeometry f = frames.next().get();
					if (f == null || f == this) frames.remove();
					else f.setLayer(curLayer++);
				}
				frameRefs.add(new WeakReference<JFakeFrameWithGeometry>(this));
				setLayer(curLayer);
			}
		};
		ff.setDesktopComponent(windowRoot);
		ff.setDesktopWidth(screenResolutionX);
		ff.setDesktopHeight(screenResolutionY);
		ff.setLayer(frameRefs.size());
		frameRefs.add(new WeakReference<JFakeFrameWithGeometry>(ff));
		return ff;
	}
	
	public void setShowDesktopBorder(boolean showDesktopBorder) {
		this.showDesktopBorder = showDesktopBorder;
		windowRoot.removeChild(desktopBorder);
		if (showDesktopBorder) windowRoot.addChild(desktopBorder);
	}

	public boolean getShowDesktopBorder() {
		return showDesktopBorder;
	}

	public void stateChanged(ChangeEvent e) {
		if (e.getSource() instanceof Scene) {
			Scene scene = (Scene) e.getSource();
			updateParent(scene);
		}
	}

}
