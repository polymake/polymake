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


package de.jreality.ui.viewerapp;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.beans.Beans;
import java.beans.Expression;
import java.beans.Statement;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.net.URL;
import java.security.PrivilegedAction;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Scanner;

import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JMenuBar;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.ToolTipManager;
import javax.swing.UIManager;

import de.jreality.io.JrScene;
import de.jreality.io.JrSceneFactory;
import de.jreality.math.Pn;
import de.jreality.reader.ReaderJRS;
import de.jreality.reader.Readers;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Viewer;
import de.jreality.scene.pick.AABBPickSystem;
import de.jreality.shader.CommonAttributes;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.toolsystem.config.ToolSystemConfiguration;
import de.jreality.util.CameraUtility;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;
import de.jreality.util.RenderTrigger;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;


/**
 * Factory class for jReality Viewer applications, which display a {@link de.jreality.io.JrScene}.<br>
 * The scene is created from a given SceneGraphComponent or geometry, an existing scene graph or JrScene.<br>
 * There are several constructors for this purpose.<br>
 * Note that when using the default JrScene, some tools may already be included in the scene. 
 * To avoid this use the appropriate constructors or static methods.<br>
 * <br>
 * The application's properties can be set the following way:
 * <code><pre>
 * ViewerApp va = new ViewerApp(...);
 * va.setAttachNavigator(true);
 * va.setExternalNavigator(false);
 * va.setAttachBeanShell(false);
 * va.setCreateMenu(true);
 * va.setShowMenu(false);
 * [setting more properties]
 * </pre></code>
 * 
 * Editing the ViewerApp's default menu can be achieved via the appropriate menu methods:
 * <code><pre>
 * ViewerAppMenu menu = va.getMenu();
 * menu.removeMenu(ViewerAppMenu.APP_MENU);
 * menu.addAction(ViewerAppMenu.FILE_MENU, action);
 * [...]
 * </pre></code>
 * 
 * <b>After setting properties always call</b> {@link ViewerApp#update()}!</b><br>
 * <br>
 * To display the scene call {@link ViewerApp#display()}.<br>
 * There are static methods to display a geometry or an existing scene graph easily.<br>
 * <br>
 * To create your own viewer application use the factory to set up the viewer and additional features 
 * and extract needed components for use in a different context.
 * 
 * @author weissman, msommer
 * 
 * @deprecated use {@link JRViewer} instead
 */
public class ViewerApp {

	private Viewer[] viewers;  //containing possible viewers (jogl, soft, portal)
	private ViewerSwitch viewerSwitch;
//	private ToolSystemViewer viewer;  //the viewer used (viewer.getDelegateViewer() = viewerSwitch)
	private ToolSystem toolSystem;		// replace ToolSystemViewer with direct access to the ToolSystem
	private RenderTrigger renderTrigger;
	private boolean autoRender = true;
	private boolean synchRender = false;
	private boolean propagateSelection = true;

	private JrScene jrScene;
	private SceneGraphComponent sceneRoot;
	private SceneGraphComponent scene;
	private SceneGraphNode displayedNode;  //the node which is displayed in viewer

	private SelectionManager selectionManager;
	private BeanShell beanShell;
	private Navigator navigator;

	private JFrame frame;
	private UIFactory uiFactory = new UIFactory();  //creates the default viewerApp layout

	private boolean attachNavigator = false;  //default
	private boolean attachBeanShell = false;  //default

	private boolean externalBeanShell = false;  //default
	private JFrame externalBeanShellFrame;
	private boolean externalNavigator = false;  //default
	private JFrame externalNavigatorFrame;

	private LinkedList<Component> accessory = new LinkedList<Component>();
	private Component showFirstAccessory;
	private HashMap<Component, String> accessoryTitles = new HashMap<Component, String>();
	private HashMap<Component, JScrollPane> accessoryScrolls = new HashMap<Component, JScrollPane>();
	private JTabbedPane navigatorTabs;

	private boolean showMenu = true;  //default
	private boolean createMenu = true;  //default
	private ViewerAppMenu menu;
	
	public static Color[] defaultBackgroundColor = new Color[]{
		new Color(225, 225, 225), new Color(225, 225, 225),
		new Color(255, 225, 180), new Color(255, 225, 180), };



	/**
	 * Use this constructor to display a SceneGraphComponent or Geometry using the ViewerApp's default JrScene.<br>
	 * Note that the default scene already includes some standard tools.  
	 * @param node the SceneGraphNode (SceneGraphComponent or Geometry) to be displayed with the viewer
	 */
	public ViewerApp(SceneGraphNode node) {
		this(node, null, null, null, null);
	}


	/**
	 * Use this constructor to display an existing scene graph by creating a new JrScene with given parameters.
	 * @param root the scene's root
	 * @param cameraPath the scene's camera path
	 * @param emptyPick the scene's empty pick path
	 * @param avatar the scene's avatar path
	 */
	public ViewerApp(SceneGraphComponent root, SceneGraphPath cameraPath, SceneGraphPath emptyPick, SceneGraphPath avatar) {
		this(null, root, cameraPath, emptyPick, avatar);
	}


	/**
	 * Use this constructor to display an existing JrScene.
	 * @param scene the scene to be displayed with the viewer
	 */
	public ViewerApp(JrScene scene) {
		this(scene.getSceneRoot(), scene.getPath("cameraPath"), scene.getPath("emptyPickPath"), scene.getPath("avatarPath"));
	}

	public  ViewerApp(Viewer[] vs)	{
		viewers = vs;
		init(null, vs[0].getSceneRoot(), vs[0].getCameraPath(), null, null);
	}

	private ViewerApp(SceneGraphNode contentNode, SceneGraphComponent root, SceneGraphPath cameraPath, SceneGraphPath emptyPick, SceneGraphPath avatar) {
		init(contentNode, root, cameraPath, emptyPick, avatar);
	}

	private void init(SceneGraphNode contentNode, SceneGraphComponent root, SceneGraphPath cameraPath, SceneGraphPath emptyPick, SceneGraphPath avatar) {
		if (contentNode != null)  //create default scene if null
			if (!(contentNode instanceof Geometry) && !(contentNode instanceof SceneGraphComponent))
				throw new IllegalArgumentException("Only Geometry or SceneGraphComponent allowed!");

		if (root == null) this.jrScene = getDefaultScene();
		else {
			JrScene s = new JrScene();
			s.setSceneRoot(root);
			if (cameraPath!= null) s.addPath("cameraPath", cameraPath);
			if (avatar != null) s.addPath("avatarPath", avatar);
			if (emptyPick != null) s.addPath("emptyPickPath", emptyPick);
			this.jrScene = s;
		}

		displayedNode = contentNode;

		//update autoRender & synchRender
		String autoRenderProp = Secure.getProperty(SystemProperties.AUTO_RENDER, SystemProperties.AUTO_RENDER_DEFAULT);
		if (autoRenderProp.equalsIgnoreCase("false")) {
			autoRender = false;
		}
		String synchRenderProp = Secure.getProperty(SystemProperties.SYNCH_RENDER, SystemProperties.SYNCH_RENDER_DEFAULT);
		if (synchRenderProp.equalsIgnoreCase("true")) {
			synchRender = true;
		}
		if (autoRender) renderTrigger = new RenderTrigger();
		if (synchRender) {
			if (autoRender) renderTrigger.setAsync(false);
			else LoggingSystem.getLogger(this).config("Inconsistant settings: no autoRender but synchRender!!");
		}

		//load the scene depending on environment (desktop | portal)
		setupViewer(jrScene);	

		selectionManager = SelectionManagerImpl.selectionManagerForViewer(getViewer());
//		System.err.println("VA: Selection man is "+selectionManager);
//		set default selection
		SceneGraphPath p = getToolSystem().getEmptyPickPath();
		if (p==null) {
			p = new SceneGraphPath();
			p.push(getViewerSwitch().getSceneRoot());
		}
		selectionManager.setDefaultSelection(new Selection(p));
		selectionManager.setSelection(selectionManager.getDefaultSelection());

		frame = new JFrame();
	}

	public static void main(String[] args) throws IOException {
		ViewerApp va;
		boolean navigator = true;
		boolean beanshell = false;
		boolean external = true;
		boolean callEncompass = false;

		if (args.length != 0) {  //params given
			LinkedList<String> params = new LinkedList<String>();
			for (String p : args) params.add(p);

			if (params.contains("-h") || params.contains("--help")) {
				System.out.println("Usage:  ViewerApp [-options] [file list]");
				System.out.println("\t -s \t the (single) file given is a .jrs file containing a whole scene\n" +
				"\t\t (otherwise all specified files are loaded into the default scene)");
				System.out.println("\t -n \t show navigator");
				System.out.println("\t -b \t show beanshell");
				System.out.println("\t -i \t show navigator and/or beanshell in the main frame\n" +
				"\t\t (otherwise they are opened in separate frames)");
				System.exit(0);
			}
			//read params
			boolean scene = params.remove("-s");
			navigator = params.remove("-n");
			beanshell = params.remove("-b");
			external = !params.remove("-i");

			if (scene) {  //load scene
				if (params.size() != 1) throw new IllegalArgumentException("exactly one scene file allowed");
				ReaderJRS r = new ReaderJRS();
				r.setInput(Input.getInput(params.getLast()));
				va = new ViewerApp(r.getScene());
			} 
			else {  //load node(s)
				SceneGraphComponent cmp = new SceneGraphComponent("content");
				for (String file : params) {
					try {
						cmp.addChild(Readers.read(Input.getInput(file)));
						callEncompass = true;
					} catch (IOException e) {
						System.out.println(e.getMessage());
					}
				}
				va = new ViewerApp(cmp);
			}
		} 
		else va = new ViewerApp(null, null, null, null, null);

		va.setAttachNavigator(navigator);
		va.setExternalNavigator(external);
		va.setAttachBeanShell(beanshell);
		va.setExternalBeanShell(external);
		va.update();

		if (callEncompass) {
			CameraUtility.encompass(va.getToolSystem().getAvatarPath(),
					va.getToolSystem().getEmptyPickPath(),
					va.getViewer().getCameraPath(),
					1.75, Pn.EUCLIDEAN); //va.getViewer().getMetric());
		}

		va.display();
	}


	/**
	 * Display the scene in a JFrame.
	 * @return the frame
	 */
	public JFrame display() {

		//set general properties of UI
		try {
			//use CrossPlatformLookAndFeel (SystemLookAndFeel looks ugly on windows & linux)
			String os = Secure.getProperty("os.name");
			boolean macOS = os.equalsIgnoreCase("Mac OS X");
			if (!macOS) {
				UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
			}
		} catch (Exception e) {}
		JPopupMenu.setDefaultLightWeightPopupEnabled(false);
		ToolTipManager.sharedInstance().setLightWeightPopupEnabled(false);
		frame.setLocationByPlatform(true);

		//set viewer background color if not specified already
		if (getSceneRoot().getAppearance() != null && 
				(getSceneRoot().getAppearance().getAttribute(CommonAttributes.BACKGROUND_COLORS) == Appearance.INHERITED 
						&& getSceneRoot().getAppearance().getAttribute(CommonAttributes.BACKGROUND_COLOR) == Appearance.INHERITED))
			setBackgroundColor(defaultBackgroundColor);

		//frame properties
		frame.setTitle("jReality Viewer");
		if (!Beans.isDesignTime()) 
			frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		//VIEWER SIZE IS SET TO 800x600 by UIFactory
//		Dimension size = frame.getToolkit().getScreenSize();
//		size.width*=.7;
//		size.height = (int) (size.width*3./4.);
//		((Component) currViewer.getViewingComponent()).setPreferredSize(size);

		frame.validate();
		frame.pack();  //size frame to fit preferred sizes of subcomponents

		//encompass scene before displaying
//		CameraUtility.encompass(currViewer.getAvatarPath(),
//		currViewer.getEmptyPickPath(),
//		currViewer.getCameraPath(),
//		1.75, currViewer.getMetric());

		frame.setVisible(true);

		return frame;
	}


	/**
	 * Displays a specified SceneGraphComponent or Geometry using the jReality viewer and the ViewerApp's default JrScene.
	 * @param node the SceneGraphNode (SceneGraphComponent or Geometry) to be displayed in the viewer
	 * @return the ViewerApp factory instantiated to display the node
	 */
	public static ViewerApp display(SceneGraphNode node) {

		ViewerApp va = new ViewerApp(node);
		va.setAttachNavigator(false);
		va.setExternalNavigator(false);
		va.setAttachBeanShell(false);
		va.setExternalBeanShell(false);
		va.update();
		va.display();

		return va;
	}

	/**
	 * Displays an existing scene graph by creating a new JrScene with given parameters.
	 * @param root the scene's root
	 * @param cameraPath the scene's camera path
	 * @param emptyPick the scene's empty pick path
	 * @param avatar the scene's avatar path
	 * @return the ViewerApp factory instantiated to display the scene
	 */
	public static ViewerApp display(SceneGraphComponent root, SceneGraphPath cameraPath, SceneGraphPath emptyPick, SceneGraphPath avatar) {

		ViewerApp va = new ViewerApp(null, root, cameraPath, emptyPick, avatar);
		va.setAttachNavigator(false);
		va.setExternalNavigator(false);
		va.setAttachBeanShell(false);
		va.setExternalBeanShell(false);
		va.update();
		va.display();

		return va;

	}


	/**
	 * Update the viewer application factory.<br>
	 * Needs to be invoked before calling display, menu or getter methods. 
	 */
	public void update() {

		showExternalBeanShell(attachBeanShell && externalBeanShell);
		showExternalNavigator(attachNavigator && externalNavigator);

		//update menu (e.g. visibility and checkboxes whose selection state depends on viewerApp properties)
		if (createMenu) {
			if (menu==null) menu = new ViewerAppMenu(this);  //uses frame, viewer, selectionManager and this
			
			if (frame.getJMenuBar()==null) {
				JMenuBar menuBar = menu.getMenuBar();
				menuBar.setBorder(BorderFactory.createEmptyBorder());  //needed for full screen mode
				frame.setJMenuBar(menuBar);
			}
			menu.update();	
		}
		else frame.setJMenuBar(null);
		

		//update content of frame
		frame.getContentPane().removeAll();
		frame.getContentPane().add(getContent());
		frame.validate();
	}


	/**
	 * Get the default Scene depending on the environment (desktop or portal or portal-remote).
	 * 
	 * @return the default scene
	 */
	private JrScene getDefaultScene() {
		String environment = Secure.getProperty(SystemProperties.ENVIRONMENT, SystemProperties.ENVIRONMENT_DEFAULT);

		if (environment.equals("desktop"))
			return JrSceneFactory.getDefaultDesktopScene();
		if (environment.equals("portal"))
			return JrSceneFactory.getDefaultPortalScene();
		if (environment.equals("portal-remote"))
			return JrSceneFactory.getDefaultPortalRemoteScene();

		throw new IllegalStateException("unknown environment: "+environment);

	}


	/**
	 * Set up the viewer. This should only be called once in the life of a ViewerApp, so some of the code
	 * probably can be omitted.
	 * @param sc the scene to load
	 */  
	private void setupViewer(JrScene sc) {
		if (viewerSwitch != null) {
			if (autoRender) {
				renderTrigger.removeViewer(viewerSwitch);
				if (viewerSwitch.getSceneRoot() != null)
					renderTrigger.removeSceneGraphComponent(viewerSwitch.getSceneRoot());
			}
			viewerSwitch.dispose();
		}
		if (toolSystem != null)	toolSystem.dispose();
		toolSystem = Secure.doPrivileged(new PrivilegedAction<ToolSystem>() {
			public ToolSystem run() {
				try {
					return createToolSystem();
				} catch (Exception exc) { throw new RuntimeException(exc); }
				//return null;
			}
		});
		if (toolSystem.getPickSystem() == null)
			toolSystem.setPickSystem(new AABBPickSystem());
		//set sceneRoot and paths of viewer
		sceneRoot = sc.getSceneRoot();
		System.err.println("scene root is "+sceneRoot.getName());
		viewerSwitch.setSceneRoot(sceneRoot);

		if (autoRender) {
			renderTrigger.addViewer(viewerSwitch);
			renderTrigger.addSceneGraphComponent(sceneRoot);
		}

		SceneGraphPath path = sc.getPath("cameraPath");
		if (path != null) viewerSwitch.setCameraPath(path);
		path = sc.getPath("avatarPath");
		if (path != null) toolSystem.setAvatarPath(path);
		path = sc.getPath("emptyPickPath");
		if (path != null) {
			//init scene 
			scene = path.getLastComponent();
			toolSystem.setEmptyPickPath(path);
		}
		// it's important that the viewer is a viewer switch since it has the "common" viewing component
		toolSystem.initializeSceneTools();

		//add node to this scene depending on its type
		if (displayedNode != null) {  //show scene even if displayedNode=null
			final SceneGraphNode node = displayedNode;
			node.accept(new SceneGraphVisitor() {
				public void visit(SceneGraphComponent sc) {
					scene.addChild(sc);
				}
				public void visit(Geometry g) {
					scene.setGeometry(g);
				}
			});
		}

	}


	@SuppressWarnings("unchecked")
	private ToolSystem createToolSystem() throws IOException {
		String config = Secure.getProperty(SystemProperties.TOOL_CONFIG, SystemProperties.TOOL_CONFIG_DEFAULT);
		boolean remotePortal = config.equals("portal-remote");
		if (viewers == null) {
			if (remotePortal) {
				String viewer = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_JOGL);
				try {
					viewers = new Viewer[]{createViewer(viewer)};
				} catch (InstantiationException e) {
					e.printStackTrace();
				} catch (IllegalAccessException e) {
					e.printStackTrace();
				} catch (ClassNotFoundException e) {
					e.printStackTrace();
				}
			} else {
				String viewer = Secure.getProperty(SystemProperties.VIEWER, SystemProperties.VIEWER_DEFAULT_JOGL+" "+SystemProperties.VIEWER_DEFAULT_SOFT); // de.jreality.portal.DesktopPortalViewer");
				String[] vrs = viewer.split(" ");
				List<Viewer> viewerList = new LinkedList<Viewer>();
				String viewerClassName;
				for (int i = 0; i < vrs.length; i++) {
					viewerClassName = vrs[i];
					try {
						Viewer v = createViewer(viewerClassName);
						viewerList.add(v);
					} catch (Exception e) { // catches creation problems - i. e. no jogl in classpath
						LoggingSystem.getLogger(this).info("could not create viewer instance of ["+viewerClassName+"]");
					} catch (NoClassDefFoundError ndfe) {
						System.out.println("Possibly no jogl in classpath!");
					} catch (UnsatisfiedLinkError le) {
						System.out.println("Possibly no jogl libraries in java.library.path!");
						try {
							System.err.println("###\n"+new Scanner(new File("jni/README")).findWithinHorizon("(?s).*",0)+"\n###");                
						} catch (FileNotFoundException e) {
							System.err.println("see .../jreality/jni/README");
						} 
					}
              }	
				viewers = viewerList.toArray(new Viewer[viewerList.size()]);
			}
		} 

		if (viewerSwitch == null) 
			viewerSwitch = new ViewerSwitch(viewers);
		LoggingSystem.getLogger(this).fine("current viewer of viewer switch is "+viewerSwitch.getCurrentViewer());

		//create ToolSystemViewer with configuration corresp. to environment
		ToolSystemConfiguration cfg = loadToolSystemConfiguration();

		if (!remotePortal) {
			if (toolSystem == null) {
				toolSystem = new ToolSystem(viewerSwitch, cfg, synchRender ? renderTrigger : null);
			}
		}
		else  {
			try {
				Class portalToolSystem = Class.forName("de.jreality.toolsystem.PortalToolSystemImpl");
				Constructor<? extends ToolSystem> cc = portalToolSystem.getConstructor(new Class[]{de.jreality.jogl.JOGLViewer.class, ToolSystemConfiguration.class});
				de.jreality.jogl.JOGLViewer cv = (de.jreality.jogl.JOGLViewer) getCurrentViewer();
				toolSystem = cc.newInstance(new Object[]{cv, cfg});
			} catch (Throwable t) {
				t.printStackTrace();
			}
		}
		// here we make sure that all the viewers are attached to the same tool system
		viewerSwitch.setToolSystem(toolSystem);
		return toolSystem;
	}


	private ToolSystemConfiguration loadToolSystemConfiguration() {
		return Secure.doPrivileged(new PrivilegedAction<ToolSystemConfiguration>() {
			public ToolSystemConfiguration run() {
				String config = Secure.getProperty(SystemProperties.TOOL_CONFIG, SystemProperties.TOOL_CONFIG_DEFAULT);
				ToolSystemConfiguration cfg=null;
				// HACK: only works for "regular" URLs
				try {
					if (config.contains("://")) {
						cfg = ToolSystemConfiguration.loadConfiguration(new Input(new URL(config)));
					} else {
						if (config.equals("default")) cfg = ToolSystemConfiguration.loadDefaultDesktopConfiguration();
						if (config.equals("portal")) cfg = ToolSystemConfiguration.loadDefaultPortalConfiguration();
						if (config.equals("portal-remote")) cfg = ToolSystemConfiguration.loadRemotePortalConfiguration();
						if (config.equals("default+portal")) cfg = ToolSystemConfiguration.loadDefaultDesktopAndPortalConfiguration();
					}
				} catch (IOException e) {
					// should not happen
					e.printStackTrace();
				}
				if (cfg == null) throw new IllegalStateException("couldn't load config ["+config+"]");
				return cfg;
			}
		});
	}


	private Viewer createViewer(String viewer) 
	throws InstantiationException, IllegalAccessException, ClassNotFoundException, IOException
	{
		return (Viewer) Class.forName(viewer).newInstance();
	}


	/**
	 * Set up the BeanShell.
	 */
	private void setupBeanShell() {

		beanShell = new BeanShell(selectionManager);

		beanShell.eval("import de.jreality.geometry.*;");
		beanShell.eval("import de.jreality.math.*;");    
		beanShell.eval("import de.jreality.scene.*;");
		beanShell.eval("import de.jreality.scene.data.*;");
		beanShell.eval("import de.jreality.scene.tool.*;");
		beanShell.eval("import de.jreality.shader.*;");
		beanShell.eval("import de.jreality.tools.*;");
		beanShell.eval("import de.jreality.util.*;");

		//set some objects to be accessible from within the beanShell
		try {
			Object bshEval = new Expression(beanShell, "getBshEval", null).getValue();
			Object interpreter = new Expression(bshEval, "getInterpreter", null).getValue();
      new Statement(interpreter, "set", new Object[]{"_viewer", viewerSwitch}).execute();
      new Statement(interpreter, "set", new Object[]{"_toolSystem", toolSystem}).execute();
      new Statement(interpreter, "set", new Object[]{"_viewerApp", this}).execute();
		} 
		catch (Exception e) { e.printStackTrace(); }

//		beanShell.setSelf(sceneRoot);  //already set default in constructor

		Component beanShell = this.beanShell.getComponent();
		//init sizes
		beanShell.setPreferredSize(new Dimension(0, 100));
		beanShell.setMinimumSize(new Dimension(10, 100));
	}


	/**
	 * Set up the navigator (sceneTree and inspector).
	 */
	private void setupNavigator() {

		navigator = new Navigator(getViewerSwitch(), frame);
		navigator.setPropagateSelections(propagateSelection);
		Component navigator = this.navigator.getComponent();
		//init sizes
		navigator.setPreferredSize(new Dimension(200, 0));
		navigator.setMinimumSize(new Dimension(200, 10));
		//init divider location within navigator
		try { ((JSplitPane)navigator).setDividerLocation(350); }
		catch (ClassCastException e) {}
	}


	public void showExternalNavigator(boolean show) {

		if (externalNavigatorFrame == null) {
			if (show == false) return;
			externalNavigatorFrame = new JFrame("jReality Navigator");
			externalNavigatorFrame.setSize(new Dimension(300, 800));
//			try {	externalNavigatorFrame.setAlwaysOnTop(true); }
//			catch (SecurityException se) {}  //webstart

			externalNavigatorFrame.addComponentListener(new ComponentAdapter(){
				public void componentHidden(ComponentEvent e) {  //externalNavigatorFrame is closed or set to invisible
					if (externalNavigator) { 
						setAttachNavigator(false);
						if (createMenu) menu.update();
					}
				}
			});
		}

		if (show == externalNavigatorFrame.isVisible()) 
			return;  //nothing to do

		if (show) {
			externalNavigatorFrame.remove(externalNavigatorFrame.getContentPane());
			externalNavigatorFrame.getContentPane().add(getNavigatorWithAccessories());
			externalNavigatorFrame.setVisible(true);
		}
		else externalNavigatorFrame.setVisible(false);
	}


	public void showExternalBeanShell(boolean show) {

		if (externalBeanShellFrame == null) {
			if (show == false) return;
			externalBeanShellFrame = new JFrame("jReality BeanShell");
			externalBeanShellFrame.setSize(new Dimension(800, 150));
//			try {	externalBeanShellFrame.setAlwaysOnTop(true); }
//			catch (SecurityException se) {}  //webstart
			
			externalBeanShellFrame.addComponentListener(new ComponentAdapter(){
				public void componentHidden(ComponentEvent e) {
					if (externalBeanShell) {
						setAttachBeanShell(false);
						if (createMenu) menu.update();
					}
				}
			});
		}

		if (show == externalBeanShellFrame.isVisible()) 
			return;  //nothing to do

		if (show) {
			externalBeanShellFrame.remove(externalBeanShellFrame.getContentPane());
			externalBeanShellFrame.getContentPane().add(getBeanShell());
			externalBeanShellFrame.setVisible(true);
		}
		else externalBeanShellFrame.setVisible(false);
	}


	/**
	 * Use to attach a navigator (sceneTree and inspector) to the viewer.
	 * @param b true iff navigator is to be attached
	 */
	public void setAttachNavigator(boolean b) {
		attachNavigator = b;
	}


	/**
	 * Use to attach a bean shell to the viewer. 
	 * @param b true iff bean shell is to be attached
	 */
	public void setAttachBeanShell(boolean b) {
		attachBeanShell = b;
	}


	/**
	 * Get current frame displaying the scene.
	 * @return the frame
	 */
	public JFrame getFrame() {
		return frame;
	}


	/**
	 * Get the ViewerApp frame's content.
	 * @return the content
	 * @deprecated renamed to {@link ViewerApp#getContent()}
	 */
	public Component getComponent() {
		return getContent();
	}


	/**
	 * Get the ViewerApp frame's content.
	 * @return the content
	 * @see ViewerApp#getViewingComponent()
	 */
	public Component getContent() {

		uiFactory.setViewer(getViewingComponent());

		boolean includeNavigator = attachNavigator && !externalNavigator;
		uiFactory.setAttachNavigator(includeNavigator);
		if (includeNavigator) uiFactory.setNavigator(getNavigatorWithAccessories());

		boolean includeBeanShell = attachBeanShell && !externalBeanShell;
		uiFactory.setAttachBeanShell(includeBeanShell);
		if (includeBeanShell) uiFactory.setBeanShell(getBeanShell());

//		//add accessories
//		if (!accessory.isEmpty()) uiFactory.removeAccessories();
//		for (Component c : accessory) 
//		uiFactory.addAccessory(c, accessoryTitles.get(c));

		Component cmp;

		try {	cmp = uiFactory.getDefaultUI(); } 
		catch (UnsupportedOperationException e) {
			throw new UnsupportedOperationException("No viewer instantiated, call update()");
		}

		return cmp;
	}


	/**
	 * Get the SelectionManager managing selections in the ViewerApp.
	 * @return the SelectionManager
	 */
	public SelectionManager getSelectionManager() {
		return selectionManager;
	}


	/**
	 * Get the navigator. 
	 */
	public Component getNavigator() {
		if (navigator == null) setupNavigator();

		return navigator.getComponent();
	}

	public void setPropagateSelection(boolean b)	{
		propagateSelection = b;
		if (navigator != null) navigator.setPropagateSelections(b);
	}

	/**
	 * Get the bean shell. 
	 */
	public Component getBeanShell() {
		if (beanShell == null) setupBeanShell();

		return beanShell.getComponent();
	}


	/**
	 * Add accessory components as tabs to navigator component.
	 * @return tabbed pane with navigator and accessory components
	 * @see ViewerApp#addAccessory(Component, String) 
	 */
	public Component getNavigatorWithAccessories() {

		if (navigatorTabs == null) {
			navigatorTabs = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
		}
		navigatorTabs.removeAll();
		navigatorTabs.add("Scene graph", getNavigator());
		for (Component c : accessory) {  //add accessories 
			if (accessoryScrolls.get(c) != null)	{
				navigatorTabs.addTab(accessoryTitles.get(c), accessoryScrolls.get(c));				
			} else
				navigatorTabs.addTab(accessoryTitles.get(c), c);				
		}
		if (showFirstAccessory != null) 	{
			JScrollPane jsp = accessoryScrolls.get(showFirstAccessory);
			if (jsp != null) navigatorTabs.setSelectedComponent(jsp);
			else if (navigatorTabs.isAncestorOf(showFirstAccessory))
					navigatorTabs.setSelectedComponent(showFirstAccessory); //scroll);
		}


		return navigatorTabs;
	}


	/**
	 *
	 * @return {@link #getViewerSwitch()}.
	 * @deprecated Use {@link #getViewerSwitch()} {@link #getCurrentViewer()}.
	 */
	public Viewer getViewer() {
		return getViewerSwitch();
	}

	/**
	 * It's useful to get a hold of the active backend.
	 * @return
	 */
	public Viewer getCurrentViewer()	{
		return viewerSwitch.getCurrentViewer();
	}

	public ViewerSwitch getViewerSwitch()	{
		return viewerSwitch;
	}

	public ToolSystem getToolSystem()	{
		return toolSystem;
	}

	/**
	 * Get the viewing component only.
	 * @return the viewing component
	 * @see ViewerApp#getContent()
	 */
	public Component getViewingComponent() {
		return (Component) getViewerSwitch().getViewingComponent();  
	}

	/**
	 * Get the scene displayed.
	 * @return the JrScene
	 */
	public JrScene getJrScene() {
		return jrScene;
	}


	/**
	 * Get the scene's root node.
	 * @return the root
	 */
	public SceneGraphComponent getSceneRoot() {
		return sceneRoot;
	}


	/**
	 * Returns true iff a bean shell is attached to the viewer.
	 */
	public boolean isAttachBeanShell() {
		return attachBeanShell;
	}


	/**
	 * Returns true iff a navigator is attached to the viewer. 
	 */
	public boolean isAttachNavigator() {
		return attachNavigator;
	}


	/**
	 * @deprecated renamed into {@link ViewerApp#setCreateMenu(boolean)}
	 */
	public void setIncludeMenu(boolean b) {
		setCreateMenu(b);
	}


	/**
	 * @deprecated renamed into {@link ViewerApp#isCreateMenu()} 
	 */
	public boolean isIncludeMenu() {
		return isCreateMenu();
	}
	
	
	/**
	 * Specify whether to create a menu bar for the ViewerApp's frame.
	 * @param b true iff menu is to be created
	 * @see ViewerApp#setShowMenu(boolean)
	 */
	public void setCreateMenu(boolean b) {
		createMenu = b;
	}


	public boolean isCreateMenu() {
		return createMenu;
	}


	/**
	 * Specify whether to show or to hide the ViewerApp's menu bar.
	 * @param b true iff menu bar is to be shown
	 * @see ViewerApp#setCreateMenu(boolean)
	 */
	public void setShowMenu(boolean b) {
		showMenu = b;
	}


	public boolean isShowMenu() {
		return showMenu;
	}


	/**
	 * Use this method to edit the ViewerApp's menu bar 
	 * (add/remove menus, add/remove items or actions to special menus).
	 * @return the viewerApp's menu
	 */
	public ViewerAppMenu getMenu() {
		if (createMenu) {
			if (menu==null) menu = new ViewerAppMenu(this);  //uses frame, viewer, selectionManager and this
		}
		else throw new IllegalStateException("You chose to not create the menu.");
		
		return menu;
	}


	public void addAccessory(Component c) {
		addAccessory(c, c.getName());
	}


	public void addAccessory(Component c, String title) {
		addAccessory(c, c.getName(), true);
	}

	public void addAccessory(Component c, String title, boolean scrolling) {
		accessory.add(c);
		accessoryTitles.put(c, title);
		if (scrolling)	{
			JScrollPane scroll = new JScrollPane(c);
			scroll.setBorder(BorderFactory.createEmptyBorder());
			accessoryScrolls.put(c, scroll);			
		}
	}

	public void setFirstAccessory(Component c)	{
		showFirstAccessory = c;
	}
	
	public void removeAccessory(Component c)	{
		accessory.remove(c);
	}

	public boolean isExternalNavigator() {
		return externalNavigator;
	}


	/**
	 * Specify whether to display the navigator in the viewerApp's frame 
	 * or in an external frame.
	 * @param externalBeanShell true iff navigator is to be displayed in an external frame
	 */
	public void setExternalNavigator(boolean externalNavigator) {
		this.externalNavigator = externalNavigator;
	}


	public boolean isExternalBeanShell() {
		return externalBeanShell;
	}


	/**
	 * Specify whether to display the bean shell in the viewerApp's frame 
	 * or in an external frame.
	 * @param externalBeanShell true iff bean shell is to be displayed in an external frame
	 */
	public void setExternalBeanShell(boolean externalBeanShell) {
		this.externalBeanShell = externalBeanShell;
	}


	/**
	 * Sets the scene root's background color.
	 * @param colors list of colors with length = 1 or 4
	 */
	public void setBackgroundColor(Color... colors) {
		if (colors == null || (colors.length!=1 && colors.length!=4)) 
			throw new IllegalArgumentException("illegal length of colors[]");
		if (sceneRoot.getAppearance() == null) sceneRoot.setAppearance(new Appearance());

		//trim colors[] if it contains the same 4 colors
		if (colors.length == 4) {
			boolean equal = true;
			for (int i = 1; i < colors.length; i++)
				if (colors[i] != colors[0]) equal = false;
			if (equal) colors = new Color[]{ colors[0] };
		}

		sceneRoot.getAppearance().setAttribute("backgroundColor", (colors.length==1)? colors[0] : Appearance.INHERITED);
		sceneRoot.getAppearance().setAttribute("backgroundColors", (colors.length==4)? colors : Appearance.INHERITED); 
	}

	public void dispose() {
		if (autoRender) {
			renderTrigger.removeSceneGraphComponent(sceneRoot);
			renderTrigger.removeViewer(viewerSwitch);
		}
		viewerSwitch.dispose();
		
		for (Viewer v : viewers) {
			v.setSceneRoot(null);
			v.setCameraPath(null);
			v.setAuxiliaryRoot(null);
		}
		
		if (toolSystem != null) toolSystem.dispose();

		SelectionManagerImpl.disposeForViewer(viewerSwitch);
		
		if (menu != null) menu.dispose();
		
		frame.getContentPane().removeAll();
		
		frame.dispose();
		if (externalNavigatorFrame!=null) externalNavigatorFrame.dispose();
		if (externalBeanShellFrame!=null) externalBeanShellFrame.dispose();
	}

}