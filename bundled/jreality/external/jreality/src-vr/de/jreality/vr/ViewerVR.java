/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.vr;

import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.geom.Rectangle2D;
import java.beans.Expression;
import java.beans.Statement;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.prefs.BackingStoreException;
import java.util.prefs.InvalidPreferencesFormatException;
import java.util.prefs.Preferences;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;
import javax.swing.KeyStroke;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.portal.PortalCoordinateSystem;
import de.jreality.reader.Readers;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.PointLight;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.scene.pick.AABBPickSystem;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.sunflow.RenderOptions;
import de.jreality.sunflow.Sunflow;
import de.jreality.swing.jrwindows.JRWindow;
import de.jreality.swing.jrwindows.JRWindowManager;
import de.jreality.tools.HeadTransformationTool;
import de.jreality.tools.PickShowTool;
import de.jreality.tools.PointerDisplayTool;
import de.jreality.tools.ShipNavigationTool;
import de.jreality.toolsystem.ToolUtility;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.ui.viewerapp.ViewerAppMenu;
import de.jreality.util.Input;
import de.jreality.util.PickUtility;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;


/**
 * @deprecated use {@link JRViewer} instead
 */
public class ViewerVR {

	private static final int CMD_MASK = Toolkit.getDefaultToolkit().getMenuShortcutKeyMask();
	
	PickShowTool pickShowTool = new PickShowTool();

	// defaults for light panel
	private static final double DEFAULT_SUN_LIGHT_INTENSITY = 1;
	private static final double DEFAULT_HEAD_LIGHT_INTENSITY = .3;
	private static final double DEFAULT_SKY_LIGHT_INTENSITY = .2;

	// defaults for preferences:
	private static final boolean DEFAULT_PANEL_IN_SCENE = false;

	private static final boolean DEFAULT_SHOW_PICK_IN_SCENE = false;


	// other static constants:

//	// width of control panel in meters
//	private static final double PANEL_WIDTH = 1;
//
//	// distance of all panels from avatar in meters
//	private static final double PANEL_Z_OFFSET = -2.2;
//
//	// height of upper edge of control panel in meters
//	private static final double PANEL_ABOVE_GROUND = 1.8;
//
//	// height of upper edge of file browser panel in meters
//	private static final int FILE_CHOOSER_ABOVE_GROUND = 2;
//
//	// width of file browser panel in meters
//	private static final int FILE_CHOOSER_PANEL_WIDTH = 2;

	// parts of the scene that do not change
	private SceneGraphComponent sceneRoot = new SceneGraphComponent("root"),
	sceneNode = new SceneGraphComponent("scene"),
	avatarNode = new SceneGraphComponent("avatar"),
	camNode = new SceneGraphComponent("cam"),
	defaultSkyLightNode = new SceneGraphComponent("skyLight"),
	skyLightNode,
	terrainNode = new SceneGraphComponent("terrain");
	private Appearance rootAppearance = new Appearance("app"),
	terrainAppearance = new Appearance("terrain app"),
	contentAppearance = new Appearance("content app");
	private SceneGraphComponent alignmentComponent, currentContent;

	private SceneGraphPath cameraPath, avatarPath, emptyPickPath;

	// default lights
	private DirectionalLight sunLight = new DirectionalLight();
	private PointLight headLight = new PointLight();
	private DirectionalLight skyAmbientLight = new DirectionalLight();

	// the scale of the currently loaded content
	private double objectScale=1;

	// the scale of the currently loaded terrain
	private double terrainScale=1;

	private JRWindowManager wm;
	private JRWindow sp;

	// the default panel content - the tabs containing plugin panels
	private Container defaultPanel;

	// macosx hack - split panels into two groups
	// such that for each group the tabs fit into one row
	private JTabbedPane geomTabs;
	private JTabbedPane appearanceTabs;

	// the current environment
	private ImageData[] environment;

	// flag indicating wether aabb-trees will be generated
	// when content is set
	private boolean generatePickTrees;
	// whether to perform alignment
	boolean doAlign = true;
	private JCheckBoxMenuItem panelInSceneCheckBox;

	// navigation tools
	private ShipNavigationTool shipNavigationTool;
	private HeadTransformationTool headTransformationTool;

	// content alignment
	private double contentSize=20;
	private double contentOffset=.3;
	private Matrix contentMatrix=null;

	// list of registered plugins
	protected List<PluginVR> plugins=new ArrayList<PluginVR>();

	private JCheckBoxMenuItem showPickInSceneCheckBox;

	@SuppressWarnings("serial")
	public ViewerVR() {

		// find out where we are running
		boolean portalRemote = "portal-remote".equals(Secure.getProperty(SystemProperties.ENVIRONMENT));
		boolean portal = "portal".equals(Secure.getProperty(SystemProperties.ENVIRONMENT));
		//benjamin 29.3.12
		boolean quadBufferedStereo = "true".equals(Secure.getProperty(SystemProperties.JOGL_QUAD_BUFFERED_STEREO));
		boolean isLeftEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_LEFT_STEREO));
		boolean isRightEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_RIGHT_STEREO));

		// build basic scene graph
		sceneRoot.setName("root");
		sceneNode.setName("scene");
		avatarNode.setName("avatar");
		camNode.setName("camNode");
		MatrixBuilder.euclidean().rotateX(-Math.PI/2).assignTo(sceneNode);
		sceneNode.getTransformation().setName("alignment");

		// root appearance
		rootAppearance.setName("root app");
		ShaderUtility.createRootAppearance(rootAppearance);
		rootAppearance.setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
		rootAppearance.setAttribute(CommonAttributes.LINE_SHADER + "."
				+ CommonAttributes.PICKABLE, false);
		rootAppearance.setAttribute(CommonAttributes.POINT_SHADER + "."
				+ CommonAttributes.PICKABLE, false);

		rootAppearance.setAttribute(CommonAttributes.RMAN_SHADOWS_ENABLED, true);
		rootAppearance.setAttribute(CommonAttributes.RMAN_RAY_TRACING_REFLECTIONS,true);
		//rootAppearance.setAttribute(CommonAttributes.RMAN_RAY_TRACING_VOLUMES,true);

		sceneRoot.setAppearance(rootAppearance);
		Camera cam = new Camera();
		cam.setNear(0.01);
		cam.setFar(1500);
		if (portal || portalRemote) {
			cam.setOnAxis(false);
			if(quadBufferedStereo){//benjamin 29.3.12
				System.out.println("true");
				cam.setStereo(true);}
			else{//benjamin 29.3.12
				System.out.println("false");
				cam.setStereo(false);//benjamin 29.3.12
				cam.setLeftEye(isLeftEye);//benjamin 29.3.12
				cam.setRightEye(isRightEye);//benjamin 29.3.12
			}
			cam.setViewPort(new Rectangle2D.Double(-1, -1, 2, 2));
		}

		// paths
		sceneRoot.addChild(avatarNode);
		avatarNode.addChild(camNode);
		camNode.setCamera(cam);
		cameraPath = new SceneGraphPath();
		cameraPath.push(sceneRoot);
		emptyPickPath = cameraPath.pushNew(sceneNode);
		cameraPath.push(avatarNode);
		cameraPath.push(camNode);
		avatarPath = cameraPath.popNew();
		cameraPath.push(cam);

		MatrixBuilder.euclidean().translate(0, PortalCoordinateSystem.convertMeters(1.7), 0).rotateX(5*Math.PI/180).assignTo(camNode);

		shipNavigationTool = new ShipNavigationTool();
		avatarNode.addTool(shipNavigationTool);
		if (portal || portalRemote) {
			avatarNode.addTool(new PointerDisplayTool() {
				{
					setVisible(false);
					setHighlight(true);
				}
				@Override
				public void perform(ToolContext tc) {
					PickResult currentPick = tc.getCurrentPick();
					boolean visible = currentPick != null && currentPick.getPickPath().startsWith(tc.getAvatarPath());
					setVisible(visible);
					if (visible) {
						super.perform(tc);
						// compute length:
						 double[] pickAvatar = ToolUtility.worldToAvatar(tc, currentPick.getWorldCoordinates());
						 Matrix pointer = new Matrix(tc.getTransformationMatrix(AVATAR_POINTER));
						 double f = pointer.getEntry(3, 3);
						 pickAvatar[0]-=pointer.getEntry(0, 3)/f;
						 pickAvatar[1]-=pointer.getEntry(1, 3)/f;
						 pickAvatar[2]-=pointer.getEntry(2, 3)/f;
						 setLength(Rn.euclideanNorm(pickAvatar));
					} 
				}
			});
		}
		if (!portal && !portalRemote) {
			headTransformationTool = new HeadTransformationTool();
			camNode.addTool(headTransformationTool);
		} 
		if (portal) {
			try {
				Tool t = (Tool) Class.forName("de.jreality.tools.PortalHeadMoveTool").newInstance();
				camNode.addTool(t);
			} catch (Throwable t) {
				t.printStackTrace();
			}
		}
		if (portalRemote) {
			try {
				Tool t = (Tool) Class.forName("de.jreality.tools.RemotePortalHeadMoveTool").newInstance();
				camNode.addTool(t);
			} catch (Throwable t) {
				t.printStackTrace();
			}
		}

		terrainAppearance.setAttribute("showLines", false);
		terrainAppearance.setAttribute("showPoints", false);
		terrainAppearance.setAttribute("diffuseColor", Color.white);
		terrainAppearance.setAttribute(CommonAttributes.BACK_FACE_CULLING_ENABLED, true);
		terrainAppearance.setAttribute(CommonAttributes.SPECULAR_COEFFICIENT, 0.0);
		terrainAppearance.setAttribute(CommonAttributes.SPECULAR_COLOR, Color.black);
		terrainNode.setAppearance(terrainAppearance);
		sceneRoot.addChild(terrainNode);

		// content appearearance
		contentAppearance.setName("contentApp");
		
//		contentAppearance.setAttribute("sunflowShader", "glass");
		sceneNode.setAppearance(contentAppearance);

		sceneRoot.addChild(sceneNode);

		if (portal || portalRemote) {
			wm = new JRWindowManager(avatarNode);
			wm.setPosition(new double[]{0, PortalCoordinateSystem.convertMeters(1.24), PortalCoordinateSystem.convertMeters(-1.24)});
		} else {
			wm = new JRWindowManager(camNode);
			wm.setPosition(new double[]{0, 0, -2});
		}
		
		// swing widgets
		makeControlPanel();

		panelInSceneCheckBox = new JCheckBoxMenuItem( new AbstractAction("Show frames in scene") {
			public void actionPerformed(ActionEvent e) {
				setPanelInScene(panelInSceneCheckBox.getState());
			}
		});

		showPickInSceneCheckBox = new JCheckBoxMenuItem( new AbstractAction("Show pick in scene") {
			public void actionPerformed(ActionEvent e) {
				setShowPickInScene(showPickInSceneCheckBox.getState());
			}
		});

		defaultSkyLightNode = new SceneGraphComponent();
		sunLight = new DirectionalLight();
		sunLight.setName("sun light");
		SceneGraphComponent sunNode = new SceneGraphComponent("sun");
		sunNode.setLight(sunLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 1 }).assignTo(sunNode);
		defaultSkyLightNode.addChild(sunNode);

		SceneGraphComponent skyAmbientNode = new SceneGraphComponent();
		skyAmbientLight = new DirectionalLight();
		skyAmbientLight.setAmbientFake(true);
		skyAmbientLight.setName("sky light");
		skyAmbientNode.setLight(skyAmbientLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 0 }).assignTo(skyAmbientNode);
		defaultSkyLightNode.addChild(skyAmbientNode);
		
		setSkyLightNode(defaultSkyLightNode);

		headLight.setAmbientFake(true);
		headLight.setFalloff(1, 0, 0);
		headLight.setName("camera light");
		headLight.setColor(new Color(255,255,255,255));
		getCameraPath().getLastComponent().setLight(headLight);

		setHeadLightIntensity(DEFAULT_HEAD_LIGHT_INTENSITY);
		setSunIntensity(DEFAULT_SUN_LIGHT_INTENSITY);
		setSkyLightIntensity(DEFAULT_SKY_LIGHT_INTENSITY);

		setAvatarPosition(0, 0, 25);

		setTerrain(TerrainPluginVR.FLAT_TERRAIN);
	}

	protected void setShowPickInScene(boolean state) {
		if (state && !getSceneRoot().getTools().contains(pickShowTool)) {
			getSceneRoot().addTool(pickShowTool);
		}
		if (!state && getSceneRoot().getTools().contains(pickShowTool)) {
			getSceneRoot().removeTool(pickShowTool);
		}
		showPickInSceneCheckBox.setSelected(state);
	}

	public SceneGraphComponent getTerrain() {
		return terrainNode.getChildNodes().size() > 0 ? terrainNode.getChildComponent(0) : null;
	}

	public void setTerrain(final SceneGraphComponent c) {
		while (terrainNode.getChildComponentCount() > 0) terrainNode.removeChild(terrainNode.getChildComponent(0));
		if (c==null) return;
		shipNavigationTool.setCenter(false);
		Matrix avatarTrans=new Matrix(avatarNode.getTransformation());
		MatrixBuilder.euclidean().rotateFromTo(avatarTrans.getColumn(1), new double[]{0,1,0,0}).times(avatarTrans).assignTo(avatarTrans);
		avatarTrans.setEntry(1, 3, 0); // lift to zero level
		avatarNode.setTransformation(new Transformation(avatarTrans.getArray()));
		
		Scene.executeWriter(terrainNode, new Runnable() {
			public void run() {
				//while (terrainNode.getChildComponentCount() > 0) terrainNode.removeChild(terrainNode.getChildComponent(0));
				MatrixBuilder.euclidean().assignTo(terrainNode);
				terrainNode.addChild(c);
				Rectangle3D bounds = BoundingBoxUtility.calculateBoundingBox(terrainNode);
				// scale
				double[] extent = bounds.getExtent();
				double maxExtent = Math.max(extent[0], extent[2]);
				if (maxExtent != 0) {
					//terrainScale = TERRAIN_SIZE / maxExtent;
					terrainScale=1;
					double[] translation = bounds.getCenter();

					// determine offset in y-direction (up/down)
					AABBPickSystem ps = new AABBPickSystem();
					ps.setSceneRoot(terrainNode);
					
					List<PickResult> picks = ps.computePick(Pn.homogenize(null, translation), new double[]{0,-1,0,0});
					if (picks.isEmpty()) {
						picks = ps.computePick(translation, new double[]{0,1,0,0});
					}
					final double offset=picks.isEmpty() ? bounds.getMinY() : picks.get(0).getWorldCoordinates()[1];

					translation[1] = -terrainScale * offset;
					translation[0] *= -terrainScale;
					translation[2] *= -terrainScale;

					MatrixBuilder mb = MatrixBuilder.euclidean().translate(
							translation).scale(terrainScale);
					if (terrainNode.getTransformation() != null)
						mb.times(terrainNode.getTransformation().getMatrix());
					mb.assignTo(terrainNode);
				}
			}
		});
		if (getShipNavigationTool().getGravity() != 0) {
			avatarToGround();
		}
		for (PluginVR plugin : plugins) plugin.terrainChanged();
	}

	public void avatarToGround() {
		// move the avatar onto the next floor
		Matrix m = new Matrix(avatarNode.getTransformation());
		AABBPickSystem ps = new AABBPickSystem();
		ps.setSceneRoot(terrainNode);
		double[] pos = m.getColumn(3);
		List<PickResult> picks = ps.computePick(pos, new double[]{0,-1,0,0});
		if (picks.isEmpty()) {
			picks = ps.computePick(pos, new double[]{0,1,0,0});
		}
		if (!picks.isEmpty()) {
			setAvatarHeight(picks.get(0).getWorldCoordinates()[1]);
		}
	}
	
	public void setTerrainWithCenter(final SceneGraphComponent c) {
		while (terrainNode.getChildComponentCount() > 0) terrainNode.removeChild(terrainNode.getChildComponent(0));
		if (c==null) return;		
		Scene.executeWriter(terrainNode, new Runnable() {
			public void run() {
				MatrixBuilder.euclidean().assignTo(terrainNode);
				terrainNode.addChild(c);
				Rectangle3D bounds = BoundingBoxUtility.calculateBoundingBox(terrainNode);
				// scale
				double[] extent = bounds.getExtent();
				double maxExtent = Math.max(extent[0], extent[2]);
				if (maxExtent != 0) {
					terrainScale=1;
					if(maxExtent>1000) terrainScale=1000/maxExtent;
					double[] translation = bounds.getCenter();
					// determine offset in y-direction (up/down)
					AABBPickSystem ps = new AABBPickSystem();
					ps.setSceneRoot(terrainNode);
					List<PickResult> picks = ps.computePick(translation, new double[]{0,bounds.getMaxY(),0,1});
					double sign=1;
					if (picks.isEmpty()) {
						picks = ps.computePick(translation, new double[]{0,bounds.getMinY(),0,1});
						sign=-1;
					}
					final double offset=picks.isEmpty() ? bounds.getMaxY() : picks.get(0).getWorldCoordinates()[1];
					translation[1] = -terrainScale * offset;
					translation[0] *= -terrainScale;
					translation[2] *= -terrainScale;
					
					MatrixBuilder mb = MatrixBuilder.euclidean().scale(1,sign,1).translate(
							translation).scale(terrainScale);
					if (terrainNode.getTransformation() != null)
						mb.times(terrainNode.getTransformation().getMatrix());
					mb.assignTo(terrainNode);	
				}
			}
		});
		if (getShipNavigationTool().getGravity() != 0) {
			// move the terrain under avatar
			Matrix avatarTrans = new Matrix(avatarNode.getTransformation());
			Matrix terrainTrans = new Matrix(terrainNode.getTransformation());
			
			terrainTrans.multiplyOnLeft(avatarTrans);				
			terrainNode.setTransformation(new Transformation(terrainTrans.getArray()));	
			
			Rectangle3D bounds = BoundingBoxUtility.calculateBoundingBox(terrainNode);	
			shipNavigationTool.setCenter(true);
			shipNavigationTool.setCenter(bounds.getCenter());
		}
		for (PluginVR plugin : plugins) plugin.terrainChanged();
	}

	public ImageData[] getEnvironment() {
		return environment;
	}

	public void setEnvironment(ImageData[] datas) {
		environment = datas;
		for (PluginVR plugin : plugins) plugin.environmentChanged();
	}

	private void makeControlPanel() {
		
		sp = wm.createFrame();
		
		
//		sp = AccessController.doPrivileged(new PrivilegedAction<ScenePanel>() {
//			public ScenePanel run() {
//				return new ScenePanel();
//			}
//		});
//		sp.setPanelWidth(PANEL_WIDTH);
//		sp.setAboveGround(PANEL_ABOVE_GROUND);
//		sp.setBelowGround(0);
//		sp.setZOffset(PANEL_Z_OFFSET);

		JTabbedPane tabs = new JTabbedPane();

		String os = Secure.getProperty("os.name");
		boolean macOS = os.equalsIgnoreCase("Mac OS X");
		if (macOS) {
			geomTabs = new JTabbedPane();
			appearanceTabs = new JTabbedPane();
			tabs.add("geometry", geomTabs);
			tabs.add("appearance", appearanceTabs);
		} else {
			geomTabs = tabs;
			appearanceTabs = tabs;
		}
		sp.getFrame().setTitle("VR settings");
		sp.getFrame().getContentPane().add(tabs);
		getTerrainNode().addTool(sp.getPanelTool());
		defaultPanel = sp.getFrame().getContentPane();
	}

	public void registerPlugin(PluginVR plugin) {
		plugin.setViewerVR(this);
		JPanel panel = plugin.getPanel();
		if (panel != null) {
			// XXX: macosx hack...
			((plugins.size() % 2) == 0 ? appearanceTabs:geomTabs).add(plugin.getName(), panel);
			sp.getFrame().pack();
		}
		plugins.add(plugin);
	}

	public void switchToDefaultPanel() {
		sp.getFrame().setVisible(false);
//		sp.setPanelWidth(PANEL_WIDTH);
//		sp.setAboveGround(PANEL_ABOVE_GROUND);
		sp.getFrame().setContentPane(defaultPanel);
		sp.getFrame().pack();
		sp.getFrame().setVisible(true);
	}

	public void switchTo(JPanel panel) {
		sp.getFrame().setVisible(false);
		sp.getFrame().setVisible(false);
//		sp.setPanelWidth(PANEL_WIDTH);
//		sp.setAboveGround(PANEL_ABOVE_GROUND);
		sp.getFrame().setContentPane(panel);
		sp.getFrame().pack();
		sp.getFrame().setVisible(true);
	}

	public void switchToFileChooser(JComponent fileChooser) {
		sp.getFrame().setVisible(false);
		sp.getFrame().setVisible(false);
//		sp.setPanelWidth(FILE_CHOOSER_PANEL_WIDTH);
//		sp.setAboveGround(FILE_CHOOSER_ABOVE_GROUND);
		sp.getFrame().setContentPane(fileChooser);
		sp.getFrame().pack();
		sp.getFrame().setVisible(true);
	}

	public void showPanel() {
		//sp.show(getSceneRoot(), new Matrix(avatarPath.getMatrix(null)));
		sp.getFrame().setVisible(true);
	}

	public void setContent(SceneGraphComponent content) {
		if (alignmentComponent != null
				&& sceneNode.getChildNodes().contains(alignmentComponent)) {
			sceneNode.removeChild(alignmentComponent);
		}
		SceneGraphComponent parent = new SceneGraphComponent();
		parent.setName("content");
		parent.addChild(content);
		alignmentComponent = parent;
		currentContent = content;
		if (isGeneratePickTrees()) PickUtility.assignFaceAABBTrees(content);
		Rectangle3D bounds = BoundingBoxUtility
		.calculateChildrenBoundingBox(alignmentComponent);
		// scale
		double[] extent = bounds.getExtent();
		objectScale = Math.max(Math.max(extent[0], extent[2]), extent[1]);
		sceneNode.addChild(alignmentComponent);
		alignContent();
		for (PluginVR plugin : plugins) plugin.contentChanged();
	}

	public void alignContent() {
		if (alignmentComponent == null || !doAlign) return;
		final double diam=getContentSize();
		final double offset=getContentOffset();
		final Matrix rotation=getContentMatrix();
		Scene.executeWriter(sceneNode, new Runnable() {
			public void run() {
				if (rotation != null) {
					rotation.assignTo(alignmentComponent);
				}
				Rectangle3D bounds = BoundingBoxUtility
				.calculateBoundingBox(sceneNode);
				// scale
				double[] extent = bounds.getExtent();
				double maxExtent = Math.max(extent[0], extent[2]);
				if (maxExtent != 0) {
					double scale = diam / maxExtent;
					double[] translation = bounds.getCenter();
					translation[1] = -scale * bounds.getMinY() + offset;
					translation[0] *= -scale;
					translation[2] *= -scale;

					MatrixBuilder mb = MatrixBuilder.euclidean().translate(
							translation).scale(scale);
					if (sceneNode.getTransformation() != null)
						mb.times(sceneNode.getTransformation().getMatrix());
					mb.assignTo(sceneNode);
				}
			}
		});
	}

	/**
	 * Initializes a ViewerApp to display the scene. Restores
	 * preferences for all plugins. Set custom values after
	 * calling this method!
	 * 
	 * @return A ViewerApp to display the scene.
	 */
	public ViewerApp initialize() {
		restorePreferences(getPreferences());
		ViewerApp viewerApp = new ViewerApp(sceneRoot, cameraPath, emptyPickPath, avatarPath);
		viewerApp.setExternalBeanShell(true);
		viewerApp.setExternalNavigator(true);
		tweakMenu(viewerApp);
		return viewerApp;
	}

	/**
	 * @deprecated use {@link ViewerVR.initialize()}
	 */
	public ViewerApp display() {
		return initialize();
	}

	public void setAvatarPosition(double x, double y, double z) {
		MatrixBuilder.euclidean().translate(x, y, z).assignTo(avatarNode);
	}

	public void setAvatarHeight(double y) {
		Matrix m = new Matrix(avatarNode.getTransformation());
		//double delta = y-m.getEntry(1, 3);
		m.setEntry(1, 3, y);
		m.assignTo(avatarNode);
		//sp.adjustHeight(delta);
	}

	public void restoreDefaults() {
		setPanelInScene(DEFAULT_PANEL_IN_SCENE);	
		setShowPickInScene(DEFAULT_SHOW_PICK_IN_SCENE);	
		for (PluginVR plugin : plugins) plugin.restoreDefaults();

	}

	public void savePreferences(Preferences prefs) {
		prefs.putBoolean("panelInScene", isPanelInScene());
		prefs.putBoolean("showPickInScene", isShowPickInScene());
		for (PluginVR plugin : plugins) {
			Preferences p = prefs.node(plugin.getName());
			plugin.storePreferences(p);
		}
		try {
			prefs.flush();
		} catch(BackingStoreException e){
			e.printStackTrace();
		}
	}

	private boolean isShowPickInScene() {
		return showPickInSceneCheckBox.isSelected();
	}

	protected Preferences getPreferences() {
		return Secure.doPrivileged(new PrivilegedAction<Preferences>() {
			public Preferences run() {
				return Preferences.userNodeForPackage(ViewerVR.class);
			}
		});
	}

	public void restorePreferences(Preferences prefs) {
		if (prefs == null) {
			restoreDefaults();
			return;
		}
		setPanelInScene(prefs.getBoolean("panelInScene", DEFAULT_PANEL_IN_SCENE));
		setShowPickInScene(prefs.getBoolean("showPickInScene", DEFAULT_SHOW_PICK_IN_SCENE));
		for (PluginVR plugin : plugins) {
			Preferences p = prefs.node(plugin.getName());
			plugin.restorePreferences(p);
		}
	}

	public void exportPreferences(File file) {
		UnboundPreferences prefs = UnboundPreferences.createRoot();
		savePreferences(prefs);
		try {
			prefs.exportSubtree(new FileOutputStream(file));
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (BackingStoreException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public void importPreferences(File file) throws IOException, InvalidPreferencesFormatException {
		importPreferences(new FileInputStream(file));
	}
	
	public void importPreferences(InputStream is) throws IOException, InvalidPreferencesFormatException {
		UnboundPreferences prefs = UnboundPreferences.createRoot();
		prefs.localImportPreferences(is);
		restorePreferences(prefs);
	}
	
	public double getObjectScale() {
		return objectScale;
	}

	public double getTerrainScale() {
		return terrainScale;
	}

	public boolean isGeneratePickTrees() {
		return generatePickTrees;
	}

	public void setGeneratePickTrees(boolean generatePickTrees) {
		this.generatePickTrees = generatePickTrees;
	}
	
	@SuppressWarnings("serial")
	private void tweakMenu(final ViewerApp vapp) {
		ViewerAppMenu menu = vapp.getMenu();
		//edit File menu
		JMenu fileMenu = menu.getMenu(ViewerAppMenu.FILE_MENU);
		if (fileMenu != null) {
			for (int i=0; i<fileMenu.getItemCount(); i++) {
				JMenuItem item = fileMenu.getItem(i);
				String name = (item == null)? null : item.getActionCommand();
				if (!(ViewerAppMenu.SAVE_SCENE.equals(name) ||
						ViewerAppMenu.EXPORT.equals(name) ||
						ViewerAppMenu.QUIT.equals(name))) {
					fileMenu.remove(i--);
				}
			}
			fileMenu.insertSeparator(2);
			fileMenu.insertSeparator(1);
		}

		JMenu settings = new JMenu("ViewerVR");

		Action panelPopup = new AbstractAction("Toggle panel") {
			{
				putValue(SHORT_DESCRIPTION, "Toggle the ViewerVR panel");
				putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_P, CMD_MASK | InputEvent.SHIFT_MASK));
			}
			public void actionPerformed(ActionEvent e) {
				sp.getFrame().setVisible(!sp.getFrame().isVisible());
			}
		};
		settings.add(panelPopup);

		Action bakeTerrain = new AbstractAction("Bake") {
			{
				putValue(SHORT_DESCRIPTION, "Bake terrain lightmap");
				putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke(KeyEvent.VK_B, CMD_MASK));
			}
			public void actionPerformed(ActionEvent e) {
				bakeTerrain(vapp.getCurrentViewer());
			}
		};
		settings.add(bakeTerrain);

		settings.add(panelInSceneCheckBox);
		settings.add(showPickInSceneCheckBox);

		settings.addSeparator();

		Action defaults = new AbstractAction("Restore defaults") {
			public void actionPerformed(ActionEvent e) {
				restoreDefaults();
			}
		};
		settings.add(defaults);
		Action restorePrefs = new AbstractAction("Restore preferences") {
			public void actionPerformed(ActionEvent e) {
				restorePreferences(getPreferences());
			}
		};
		settings.add(restorePrefs);
		Action savePrefs = new AbstractAction("Save preferences") {
			public void actionPerformed(ActionEvent e) {
				savePreferences(getPreferences());
			}
		};
		settings.add(savePrefs);
		
		Action exportPrefs = new AbstractAction("Export preferences...") {
			public void actionPerformed(ActionEvent e) {
				File f=FileLoaderDialog.selectTargetFile(null, "xml", "Preferences files");
				if (f!=null) exportPreferences(f);
			}
		};
		settings.add(exportPrefs);
		
		Action importPrefs = new AbstractAction("Import preferences...") {
			public void actionPerformed(ActionEvent e) {
				File f=FileLoaderDialog.loadFile(null, "xml", "Preferences file");
				if (f!=null)
					try {
						importPreferences(f);
					} catch (IOException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					} catch (InvalidPreferencesFormatException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
			}
		};
		settings.add(importPrefs);
		
		menu.addMenu(settings);

		//setup Help menu
		JMenu helpMenu = new JMenu("Help");
		helpMenu.add(new AbstractAction("Help"){
			private static final long serialVersionUID = 3770710651980089282L;
			public void actionPerformed(ActionEvent e) {
				URL helpURL = null;
				try {
					helpURL = new URL("http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/ViewerVR_User_Manual");
				} catch (MalformedURLException e1) { e1.printStackTrace(); }

				try {
					new Statement(
							new Expression(Class.forName("java.awt.Desktop"), "getDesktop", null).getValue(),
							"browse",
							new Object[]{
						helpURL.toURI()
					}).execute();
				} catch(Exception e2) {
//					JOptionPane.showMessageDialog(null, "Please visit "+helpURL);
					makeHelpPopup();
				}
			}

		});
		menu.addMenu(helpMenu);
	}

	public void bakeTerrain(Viewer v) {
		RenderOptions opts = new RenderOptions();
		opts.setThreadsLowPriority(true);
		List<SceneGraphPath> paths = SceneGraphUtility.getPathsBetween(
				getSceneRoot(),
				getTerrain().getChildComponent(0)
		);
		SceneGraphPath bakingPath = paths.get(0);
		Sunflow.renderToTexture(
				v,
				new Dimension(1024,1024),
				opts,
				bakingPath,
				getTerrainAppearance()
		);
	}
	
//	public JFrame getExternalFrame() {
//		return sp.getExternalFrame();
//	}

	public boolean isPanelInScene() {
		return panelInSceneCheckBox.isSelected();
	}

	public void setPanelInScene(boolean b) {
		panelInSceneCheckBox.setState(b);
		wm.setWindowsInScene(b);//, sceneRoot,  new Matrix(avatarPath.getMatrix(null)));
	}

	public SceneGraphComponent getSceneRoot() {
		return sceneRoot;
	}

	public SceneGraphComponent getTerrainNode() {
		return terrainNode;
	}

	public Appearance getContentAppearance() {
		return contentAppearance;
	}

	public ShipNavigationTool getShipNavigationTool() {
		return shipNavigationTool;
	}

	public HeadTransformationTool getHeadTransformationTool() {
		return headTransformationTool;
	}

	public SceneGraphComponent getCurrentContent() {
		return currentContent;
	}

	public Appearance getRootAppearance() {
		return rootAppearance;
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public Appearance getTerrainAppearance() {
		return terrainAppearance;
	}

	public Matrix getContentMatrix() {
		return contentMatrix;
	}

	public void setContentMatrix(Matrix contentMatrix) {
		this.contentMatrix = contentMatrix;
		alignContent();
	}

	public double getContentOffset() {
		return contentOffset;
	}

	public void setContentOffset(double contentOffset) {
		this.contentOffset = contentOffset;
		alignContent();
	}

	public double getContentSize() {
		return contentSize;
	}

	public void setContentSize(double contentSize) {
		this.contentSize = contentSize;
		alignContent();
	}

	public Color getSunLightColor() {
		return sunLight.getColor();
	}

	public void setSunLightColor(Color c) {
		sunLight.setColor(c);
	}

	public Color getHeadLightColor() {
		return headLight.getColor();
	}

	public void setHeadLightColor(Color c) {
		headLight.setColor(c);
	}

	public Color getSkyLightColor() {
		return skyAmbientLight.getColor();
	}

	public void setSkyLightColor(Color c) {
		skyAmbientLight.setColor(c);
	}

	public double getSunIntensity() {
		return sunLight.getIntensity();
	}

	public void setSunIntensity(double x) {
		sunLight.setIntensity(x);
	}

	public double getHeadLightIntensity() {
		return headLight.getIntensity();
	}

	public void setHeadLightIntensity(double x) {
		headLight.setIntensity(x);
	}

	public double getSkyLightIntensity() {
		return skyAmbientLight.getIntensity();
	}

	public void setSkyLightIntensity(double x) {
		skyAmbientLight.setIntensity(x);
	}

	public void setLightIntensity(double intensity) {
		sunLight.setIntensity(intensity);
	}

	public double getLightIntensity() {
		return sunLight.getIntensity();
	}

	public void addEnvTab() {
		registerPlugin(new EnvironmentPluginVR());
	}

	public void addTerrainTab() {
		registerPlugin(new TerrainPluginVR());
	}

	public void addAppTab() {
		registerPlugin(new AppearancePluginVR());
	}

	public void addAlignTab() {
		registerPlugin(new AlignPluginVR());
	}

	public void addLoadTab(final String[][] examples) {
		registerPlugin(new LoadPluginVR(examples));
	}

	public void addToolTab() {
		registerPlugin(new ToolPluginVR());
	}

	public void addTexTab() {
		registerPlugin(new TexturePluginVR());
	}

	/**
	 * @deprecated
	 */
	public void addHelpTab() {
	}

	public static void main(String[] args) {
		mainImpl(args);
	}
	
	public static ViewerApp remoteMain(String[] args) {
		return mainImpl(args);
	}
	
	public static ViewerVR createDefaultViewerVR(String[][] loadableResources) {
		ViewerVR vr = new ViewerVR();
		if (loadableResources != null) vr.addLoadTab(loadableResources);
		vr.addAlignTab();
		AppearancePluginVR appPlugin = new AppearancePluginVR();
		vr.registerPlugin(appPlugin);
		vr.addEnvTab();
		vr.addTerrainTab();
		vr.addToolTab();
		vr.addTexTab();
		vr.registerPlugin(new AvatarAppearancePluginVR("avapp"));
		vr.setGeneratePickTrees(true);
		return vr;
	}
	
	public static ViewerApp mainImpl(String[] args) {

		boolean navigator = false;
		boolean beanshell = false;
		boolean external = true;
		SceneGraphComponent cmp = null;
		Input prefsFile=null;

		if (args.length != 0) {  //params given
			LinkedList<String> params = new LinkedList<String>();
			for (String p : args) params.add(p);

			if (params.contains("-h") || params.contains("--help")) {
				System.out.println("Usage:  ViewerVR [-options] [file list]");
				System.out.println("\t -n \t show navigator");
				System.out.println("\t -b \t show beanshell");
				System.out.println("\t -i \t show navigator and/or beanshell in the main frame\n" +
				"\t\t (otherwise they are opened in separate frames)");
				System.out.println("\t [file list] \t a list of 3D data files and an optional xml preferences file");
				System.exit(0);
			}

			navigator = params.remove("-n");
			beanshell = params.remove("-b");
			external = !params.remove("-i");
			
			if (params.size() != 0) cmp = new SceneGraphComponent();
			for (String file : params) {
				// work-around to handle different ways that webstart parses arguments to application
				// (on mac // becomes /, so URL specification is difficult
				if (file.startsWith("\"")) 
					file = file.substring(1, file.length()-1);
				try {
					if (file.toLowerCase().endsWith(".xml")) {
						prefsFile = Input.getInput(file);
					}
					else cmp.addChild(Readers.read(Input.getInput(file)));
				} catch (IOException e) {
					System.out.println(e.getMessage());
				}
			}
		}
		
		String[][] examples = new String[][] {
				{ "Boy surface", "jrs/boy.jrs" },
				{ "Chen-Gackstatter surface", "obj/Chen-Gackstatter-4.obj" },
				{ "helicoid with 2 handles", "jrs/He2WithBoundary.jrs" },
				{ "tetranoid", "jrs/tetranoid.jrs" },
				{ "Wente torus", "jrs/wente.jrs" },
				{ "Schwarz P", "jrs/schwarz.jrs" },
				{ "Matheon bear", "jrs/baer.jrs" }
		};
		
		ViewerVR vr = createDefaultViewerVR(examples);
		
		if (cmp != null) vr.setContent(cmp);
		
		vr.showPanel();
		
		ViewerApp vApp = vr.initialize();
		if (prefsFile != null)
			try {
				vr.importPreferences(prefsFile.getInputStream());
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (InvalidPreferencesFormatException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		vApp.setAttachNavigator(navigator);
		vApp.setExternalNavigator(external);
		vApp.setAttachBeanShell(beanshell);
		vApp.setExternalBeanShell(external);
		
//		appPlugin.setFaceColor(new Color(64, 222, 64));
		
		vApp.update();

		JFrame f = vApp.display();
		f.setSize(800, 600);
		f.validate();
//		JFrame externalFrame = vr.getExternalFrame();
//		externalFrame.setLocationRelativeTo(f);
		
		return vApp;
	}

	public SceneGraphComponent getSkyLightNode() {
		return skyLightNode;
	}

	public void setSkyLightNode(SceneGraphComponent s) {
		if (s == null) s = defaultSkyLightNode;
		if (s != skyLightNode) {
			if (skyLightNode != null) sceneRoot.removeChild(skyLightNode);
			skyLightNode = s;
			sceneRoot.addChild(s);
		}
	}
	
	public SceneGraphPath getAvatarPath() {
		return avatarPath;
	}
	
	public JRWindowManager getWindowManager() {
		return wm;
	}

	public boolean isDoAlign() {
		return doAlign;
	}

	public void setDoAlign(boolean doAlign) {
		this.doAlign = doAlign;
	}

	public SceneGraphComponent getAvatarNode() {
		return avatarNode;
	}

	private void makeHelpPopup() {
		final JFrame jf = new JFrame("viewerVR help link");
		Box vbox = Box.createVerticalBox();
		JTextField tf = new JTextField("Please visit http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/ViewerVR_User_Manual");
		tf.setEditable(false);
		vbox.add(Box.createVerticalStrut(5));
		vbox.add(tf);
		JButton okb = new JButton("OK");
		okb.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e) {
				jf.dispose();
			}
			
		});
		Box hbox = Box.createHorizontalBox();
		hbox.add(Box.createHorizontalGlue());
		hbox.add(okb);
		hbox.add(Box.createHorizontalGlue());
		vbox.add(Box.createVerticalStrut(5));
		vbox.add(hbox);
		vbox.add(Box.createVerticalStrut(5));
		jf.add(vbox);
		jf.pack();
		jf.setVisible(true);
	}
}
