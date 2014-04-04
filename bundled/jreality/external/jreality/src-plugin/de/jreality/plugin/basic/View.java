package de.jreality.plugin.basic;


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

import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.util.LinkedList;
import java.util.List;

import javax.swing.AbstractAction;
import javax.swing.ButtonGroup;
import javax.swing.Icon;
import javax.swing.JMenu;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.KeyStroke;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.jogl.InstrumentedViewer;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.scene.tool.Tool;
import de.jreality.tools.PickShowTool;
import de.jreality.tools.PointerDisplayTool;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.SelectionManagerImpl;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.LoggingSystem;
import de.jreality.util.RenderTrigger;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;


/**
 * @author pinkall
 */
public class View extends SideContainerPerspective implements ChangeListener {

//	String viewerString = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_JOGL+" "+SystemProperties.VIEWER_DEFAULT_SOFT);
	String viewerString = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_JOGL+" "+SystemProperties.VIEWER_DEFAULT_JOGL3+" "+SystemProperties.VIEWER_DEFAULT_SOFT+" "+SystemProperties.VIEWER_JOGL_DOME);
//	String viewerString = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT+" "+SystemProperties.VIEWER_JOGL3_DOME+" "+SystemProperties.VIEWER_DEFAULT_JOGL3); 
//	String viewerString = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_JOGL_DOME+" "+SystemProperties.VIEWER_DEFAULT_SOFT); 
//	String viewerString = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_SOFT); 
	
	private ViewerSwitch viewerSwitch;
	private RenderTrigger renderTrigger;
	private boolean autoRender = true;
	private static String 
		title = "jReality";
	private static Icon
		mainIcon = ImageHook.getIcon("hausgruen.png");
	private static List<Image>
		mainIconList = null;
	
	private RunningEnvironment runningEnvironment;

	void init(Scene scene) {
		SceneGraphComponent root=scene.getSceneRoot();
		
		// determine running environment
		String environment = Secure.getProperty(SystemProperties.ENVIRONMENT, SystemProperties.ENVIRONMENT_DEFAULT);
		if ("portal".equals(environment)) {
			runningEnvironment = RunningEnvironment.PORTAL; 
		} else if ("portal-remote".equals(environment)) {
			runningEnvironment = RunningEnvironment.PORTAL_REMOTE;
		} else {
			runningEnvironment = RunningEnvironment.DESKTOP;
		}

		// retrieve autoRender & synchRender system properties
		String autoRenderProp = Secure.getProperty(SystemProperties.AUTO_RENDER, SystemProperties.AUTO_RENDER_DEFAULT);
		if (autoRenderProp.equalsIgnoreCase("false")) {
			autoRender = false;
		}
		if (autoRender) {
			renderTrigger = new RenderTrigger();
		}
		viewerSwitch = createViewerSwitch();

		if (autoRender) {
			renderTrigger.addViewer(viewerSwitch);
			// XXX: HACK!!! or maybe not...
			renderTrigger.startCollect();
			renderTrigger.addSceneGraphComponent(root);
		}

		viewerSwitch.setSceneRoot(root);
		
		getContentPanel().setLayout(new GridLayout());
		getContentPanel().add(viewerSwitch.getViewingComponent());
		getContentPanel().setPreferredSize(new Dimension(800,600));
		getContentPanel().setMinimumSize(new Dimension(300, 200));
		
		// TODO: move this into Scene Plugin...
		
		if (runningEnvironment != RunningEnvironment.DESKTOP) {
			Camera cam = (Camera) scene.getCameraPath().getLastElement();
			cam.setNear(0.01);
			cam.setFar(1500);
			cam.setOnAxis(false);
			
			boolean quadBufferedStereo = "true".equals(Secure.getProperty(SystemProperties.JOGL_QUAD_BUFFERED_STEREO));
			boolean isLeftEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_LEFT_STEREO));
			boolean isRightEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_RIGHT_STEREO));
			//andre 17.05.2012
			boolean isMasterStereo = "true".equals(Secure.getProperty(SystemProperties.JOGL_MASTER_STEREO));
			if(quadBufferedStereo){//andre 19.04.2012
				cam.setStereo(true);}//andre 19.04.2012
			else if (isLeftEye || isRightEye){
				cam.setStereo(false);//andre 19.04.2012
				cam.setLeftEye(isLeftEye);//andre 19.04.2012
				cam.setRightEye(isRightEye);//andre 19.04.2012
			}
			else if (isMasterStereo){//andre 17.05.2012
				cam.setStereo(false);//andre 17.05.2012
			}
			else {//andre 19.04.2012
				cam.setStereo(true);//andre 19.04.2012
			}
			
			SceneGraphComponent camNode = scene.getCameraComponent();
			
			// this is required to get something rendered on the floor
			// in a test setup where head tracking is not available...
			//MatrixBuilder.euclidean().translate(0, 1.7, 0).assignTo(camNode);
			//MatrixBuilder.euclidean().translate(0,0,5).assignTo(scene.getAvatarComponent());
			
			String headMoveTool;
			if (runningEnvironment == RunningEnvironment.PORTAL_REMOTE)
				headMoveTool = "de.jreality.tools.RemotePortalHeadMoveTool";
			else
				headMoveTool = "de.jreality.tools.PortalHeadMoveTool";
			try {
				Tool t = (Tool) Class.forName(headMoveTool).newInstance();
				camNode.addTool(t);
			} catch (Throwable t) {
				System.err.println("crating headMoveTool failed");
			}
			scene.getSceneRoot().addTool(new PickShowTool());
			scene.getAvatarComponent().addTool(new PointerDisplayTool());
		}
		
	}
	
	private ViewerSwitch createViewerSwitch() {
		// make viewerSwitch
		Viewer[] viewers = null;
		if (getRunningEnvironment() != RunningEnvironment.DESKTOP) {
			String viewer = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_JOGL);
			try {
				viewers = new Viewer[]{(Viewer) Class.forName(viewer).newInstance()};
			} catch (InstantiationException e) {
				e.printStackTrace();
			} catch (IllegalAccessException e) {
				e.printStackTrace();
			} catch (ClassNotFoundException e) {
				e.printStackTrace();
			}
		} else {
			String viewer = viewerString;
			// de.jreality.portal.DesktopPortalViewer");
			String[] vrs = viewer.split(" ");
			List<Viewer> viewerList = new LinkedList<Viewer>();
			String viewerClassName;
			for (int i = 0; i < vrs.length; i++) {
				viewerClassName = vrs[i];
				try {
					Viewer v = (Viewer) Class.forName(viewerClassName).newInstance();
					viewerList.add(v);
				} catch (Exception e) { // catches creation problems - i. e. no jogl in classpath
					LoggingSystem.getLogger(this).info("could not create viewer instance of ["+viewerClassName+"]");
				} catch (NoClassDefFoundError ndfe) {
					System.out.println("Possibly no jogl in classpath!");
				} catch (UnsatisfiedLinkError le) {
					System.out.println("Possibly no jogl libraries in java.library.path!");
				}
			}
			viewers = viewerList.toArray(new Viewer[viewerList.size()]);
		}
		return new ViewerSwitch(viewers);
	}

	public ViewerSwitch getViewer()	{
		return viewerSwitch;
	}

	public RunningEnvironment getRunningEnvironment() {
		return runningEnvironment;
	}

	public void stateChanged(ChangeEvent e) {
		if (e.getSource() instanceof Scene) {
			Scene scene = (Scene) e.getSource();
			updateScenePaths(scene);
		}
	}

	private void updateScenePaths(Scene scene) {
		viewerSwitch.setCameraPath(scene.getCameraPath());	
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "View";
		info.vendorName = "Ulrich Pinkall"; 
		info.icon = mainIcon;
		info.isDynamic = false;
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		Scene scene = c.getPlugin(Scene.class);
		init(scene);
		updateScenePaths(scene);
		scene.addChangeListener(this);
		if (!c.getPlugins(InfoOverlayPlugin.class).isEmpty()) {
			// get only if controller explicitly install the overlay
			iop = c.getPlugin(InfoOverlayPlugin.class);
		}
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		Scene scene = c.getPlugin(Scene.class);
		scene.removeChangeListener(this);
		super.uninstall(c);
		if (autoRender) {
			renderTrigger.removeSceneGraphComponent(viewerSwitch.getSceneRoot());
			renderTrigger.removeViewer(viewerSwitch);
		}
		if (viewerSwitch != null) {
			viewerSwitch.dispose();
		}
	}

	public Icon getIcon() {
		return getPluginInfo().icon;
	}
	public static void setIcon(Icon mainIcon) {
		View.mainIcon = mainIcon;
	}
	@Override
	public List<Image> getIconList() {
		return mainIconList;
	}
	public static void setIconList(List<Image> iconList) {
		View.mainIconList = iconList;
	}

	public String getTitle() {
		return title;
	}
	/** 
	 * Set the window title when before startup.
	 * @param title
	 */
	public static void setTitle(String title) {
		View.title = title;
	}
	
	public void setVisible(boolean visible) {
	}

	public SelectionManager getSelectionManager() {
		return SelectionManagerImpl.selectionManagerForViewer(getViewer());
	}

	RenderTrigger getRenderTrigger() {
		return renderTrigger;
	}

	InfoOverlayPlugin iop;
	public JMenu createViewerMenu() {
		JMenu menu = new JMenu("Viewer");
		final ViewerSwitch viewerSwitch = getViewer();
		String[] viewerNames = viewerSwitch.getViewerNames();
		ButtonGroup bgr = new ButtonGroup();
		for (int i=0; i<viewerSwitch.getNumViewers(); i++) {
			final int index = i;
			String name = viewerNames[i];
			name = name.substring(name.lastIndexOf('.') + 1);
			JRadioButtonMenuItem item = new JRadioButtonMenuItem(
			new AbstractAction(name) {
				private static final long serialVersionUID = 1L;
				public void actionPerformed(ActionEvent e) {
					viewerSwitch.selectViewer(index);
					viewerSwitch.getCurrentViewer().renderAsync();
					if (iop != null && viewerSwitch.getCurrentViewer() instanceof InstrumentedViewer) {
						InstrumentedViewer iv = (InstrumentedViewer) viewerSwitch.getCurrentViewer();
						iop.getInfoOverlay().setInstrumentedViewer(iv);
					}
				}
			});
			item.setSelected(index==0);
			item.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F1 + index, 0));
			bgr.add(item);
			menu.add(item);
		}
		return menu;
	}
	
}