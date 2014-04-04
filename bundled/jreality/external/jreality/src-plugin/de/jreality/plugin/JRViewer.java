package de.jreality.plugin;

import java.awt.Component;
import java.awt.GridLayout;
import java.awt.Image;
import java.io.File;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.swing.ImageIcon;
import javax.swing.JPopupMenu;
import javax.swing.JRootPane;
import javax.swing.ToolTipManager;
import javax.swing.UIManager;

import de.jreality.io.JrScene;
import de.jreality.math.Pn;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Inspector;
import de.jreality.plugin.basic.PropertiesMenu;
import de.jreality.plugin.basic.RunningEnvironment;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.Shell;
import de.jreality.plugin.basic.ToolSystemPlugin;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewPreferences;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.content.CenteredAndScaledContent;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.plugin.content.DirectContent;
import de.jreality.plugin.content.TerrainAlignedContent;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.menu.BackgroundColor;
import de.jreality.plugin.menu.CameraMenu;
import de.jreality.plugin.menu.DisplayOptions;
import de.jreality.plugin.menu.ExportMenu;
import de.jreality.plugin.scene.Avatar;
import de.jreality.plugin.scene.Lights;
import de.jreality.plugin.scene.SceneShrinkPanel;
import de.jreality.plugin.scene.Sky;
import de.jreality.plugin.scene.Terrain;
import de.jreality.plugin.scene.VRExamples;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.Viewer;
import de.jreality.ui.viewerapp.SelectionManagerImpl;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.EncompassFactory;
import de.jreality.util.NativePathUtility;
import de.jreality.util.Secure;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.PropertiesFlavor;
import de.jtem.jrworkspace.plugin.flavor.ShutdownFlavor;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.simplecontroller.SimpleController;
import de.jtem.jrworkspace.plugin.simplecontroller.widget.SplashScreen;

/** JRViewer is the default viewer of jReality. It replaces the older viewers: <code>ViewerVR</code> and <code>ViewerApp</code>. 
 * 
 * <p>The simplest way to use the JRViewer is to call the static method 
 * <pre>
 * 	JRViewer.display(SceneGraphNode node);
 * </pre> 
 * 
 * <p>Any JRViewer is a composition of {@link Plugin}s. There are static add* methods to add bunches of related plugins.
 * To get a virtual reality JRViewer with a movable avatar you may call the convenient static method
 * <pre>
 * 	JRViewer v=JRViewer.createJRViewerVR(SceneGraphNode contentNode);
 * 	v.startup();
 * </pre>
 * This is a shortcut for
 * <pre>
 * 	JRViewer v = new JRViewer();
 * 
 * 	v.addBasicUI();
 * 	v.addVRSupport();
 *	v.addContentSupport(ContentType.TerrainAligned);
 * 	v.registerPlugin(new ContentAppearance());
 * 	v.registerPlugin(new ContentTools());
 * 	v.setContent(SceneGraphNode contentNode);
 * 
 *	v.startup();
 * </pre>
 *
 * <h4>Behind the scenes</h4>
 * The JRViewer is the jReality front end to the {@link SimpleController}. It implements convenient methods to register jReality related plugins 
 * and delegates other methods directly to the controller. The controller may be accessed directly via {@link #getController()}.
 * 
 * <h4>Plugin properties</h4>
 * The SimpleController, which works behind the scenes of the JRViewer, allows {@link Plugin}s to read and save properties. These properties 
 * are read from and saved to a file 
 * at startup and shutdown via <a href="http://xstream.codehaus.org/">XStream</a>.
 * 
 * <p>When {@link SimpleController#shutdown()} is called (from the main windows closing method or from a plugin that implements 
 * {@link ShutdownFlavor}) the user gets the chance to decide where the properties are saved. 
 * The decisions are saved
 * via the java preferences API (see {@link #setPropertiesResource(Class, String)}. If nothing
 * else is specified the <code>SimpleController</code> tries to read the plugin properties from
 * <pre> 
 * 		System.getProperty("user.home") + "/.jrworkspace/default_simple.xml"
 * </pre>
 * and saves the user decisions in the preferences node of the package of the SimpleController.
 *  
 * <p> Besides the name (and path) of the properties file the user may choose
 * <ul>
 * 	<li> whether to load properties from the file at startup (default: <code>true</code>), </li>
 *  <li> whether to save the properties file,</li>
 *  <li> whether to remember the users decisions, which disables the dialog next time (so the 
 *  user should get the chance to revise this decision via a plugin that implements {@link PropertiesFlavor}).</li>
 * </ul>
 * The user may cancel the dialog, which also cancels the shutdown process.
 *
 * <p>It is recommended that applications call
 * <pre>
 * 		setPropertiesResource(MyClass.class,"propertiesFileName")
 * </pre>
 * before calling {@link #startup()}. Then the controller loads the plugin properties from this resource. After deployment this 
 * resource may most likely only be opened for reading, which has the effect that it will only be used
 * to call {@link #setPropertiesInputStream(InputStream)} and the properties file will retain its default 
 * value or whatever it is set to via {@link #setPropertiesFile(File)}.
 * 
 * <p>When loading properties the availability of a properties file is checked in the following order
 * <ol>
 * <li> the user properties file (from the java preferences), when loadFromUserPropertyFile is <code>true</code></li>
 * <li> the propertiesInputStream</li>
 * <li> the propertiesFile</li>
 * </ol>
 * When saving properties the availability of a properties file for output is checked in the following order
 * <ol>
 * <li> the user properties file (from the java preferences)</li>
 * <li> the propertiesFile</li>
 * </ol>
 * The user is asked when askBeforeSaveOnExit is <code>true</code> or both files above can't be opened for writing.
 * 
 * <p>Note to Eclipse developers: if you change  the path of the file to save the properties into
 * in the dialog at shutdown 
 * to point to the source folder and DISABLE the load from this file check box, then the resource will be accessed
 * to load the properties (and the situation after deployment is always tested) and the source folder file 
 * is used to save (which then may be included in version control). Copying of the properties xml file to the bin folder
 * needs to be triggered manually by refreshing the source folder 
 * 

 *
 * @author sechelmann, pinkall, weissmann, peters
 */
public class JRViewer {

	private SimpleController
		c = new SimpleController("jReality Plug-In Controller");
	private View
		view = new View();
	private static RunningEnvironment
		env;
	private ViewPreferences
		viewPreferences = new ViewPreferences();
	private static WeakReference<JRViewer>
		lastViewer = new WeakReference<JRViewer>(null);
	
	static {
		NativePathUtility.set("jni");
		NativePathUtility.set("../jreality/jni");
		String lnfClass = UIManager.getSystemLookAndFeelClassName();
		if (lnfClass.contains("Aqua") || lnfClass.contains("Windows")) {
			if (lnfClass.contains("Aqua")) {
				System.setProperty("com.apple.mrj.application.apple.menu.about.name", "jReality");
				setApplicationIcon(ImageHook.getImage("hausgruen.png"));
				Secure.setProperty("apple.laf.useScreenMenuBar", "true");
			}
			try {
				UIManager.setLookAndFeel(lnfClass);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		JPopupMenu.setDefaultLightWeightPopupEnabled(false);
		ToolTipManager.sharedInstance().setLightWeightPopupEnabled(false);
	}
	
	static {
		ToolTipManager.sharedInstance().setDismissDelay(20000);
	}
	
	
	public static void setApplicationTitle(String title) {
		Secure.setProperty("apple.laf.useScreenMenuBar", "true");
		System.setProperty("com.apple.mrj.application.apple.menu.about.name", title);
		View.setTitle(title);
	}
	
	public static void setApplicationIcon(Image icon) {
		ImageIcon image = new ImageIcon(icon, "Application Icon");
		View.setIcon(image);
		String lnfClass = UIManager.getSystemLookAndFeelClassName();
		if (lnfClass.contains("Aqua")) {
			try {
				Class<?> clazz = Class.forName("com.apple.eawt.Application");
				Method getAppMethod = clazz.getMethod("getApplication");
				Method setDockIconMethod = clazz.getMethod("setDockIconImage", Image.class);
				Object app = getAppMethod.invoke(clazz);
				setDockIconMethod.invoke(app, icon);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	
	
	public static enum ContentType {
		Raw,
		CenteredAndScaled,
		TerrainAligned,
		Custom
	}
	
	
	/**
	 * Create a JRViewer with default scene and lights.
	 */
	public JRViewer() {
		this(true);
	}
	
	/**
	 * Create a JRViewer with default scene. Flag indicates
	 * whether to add the standard lights (plugin Lights) or not.
	 * @param addLights if true, standard lights are added.
	 */
	public JRViewer(boolean addLights) {
		this(null);
		if (addLights) {
			c.registerPlugin(new Lights());
		}
	}

	/**
	 * create a JRViewer with a custom scene.
	 * 
	 * @param s the scene
	 */
	public JRViewer(JrScene s) {
		setShowPanelSlots(false, true, false, false);
		c.setManageLookAndFeel(false);
		c.registerPlugin(view);
		c.registerPlugin(viewPreferences);
		c.registerPlugin(new Scene(s));
		c.registerPlugin(new ToolSystemPlugin());
		lastViewer = new WeakReference<JRViewer>(this);
	}
	

	/**
	 * Returns the last created instance of JRViewer
	 * @return a JRViewer or null
	 */
	public static JRViewer getLastJRViewer() {
		return lastViewer.get();
	}
	
	
	/**
	 * Adds a plug-in to this JTViewer's registered plug-ins. The
	 * viewer application is then assembled on startup by these plug-ins.
	 * @param p
	 */
	public void registerPlugin(Plugin p) {
		c.registerPlugin(p);
	}
	
	/**
	 * Adds a plug-in to this JTViewer's registered plug-ins. The
	 * viewer application is then assembled on startup by these plug-ins.
	 * @param p
	 */
	public void registerPlugin(Class<? extends Plugin> p) {
		c.registerPlugin(p);
	}
	
	
	/**
	 * Registered a set of plug-ins at once
	 * @param pSet a set of plug-ins
	 */
	public void registerPlugins(Set<Plugin> pSet) {
		for (Plugin p : pSet) {
			registerPlugin(p);
		}
	}
	
	
	/**
	 * Returns a previously registered plug in instance
	 * @param <T>
	 * @param clazz the class of the plug-in
	 * @return a plug-in instance or null if no such plug-in
	 * was registered
	 */
	public <T extends Plugin> T getPlugin(Class<T> clazz) {
		return c.getPlugin(clazz);
	}
	
	/**
	 * @return the {@link Viewer} of this <code>JRViewer</code> (only works 
	 * after startup).
	 */
	public Viewer getViewer() {
		return getPlugin(View.class).getViewer();
	}
	
	/**
	 * Sets a content node. The content node will be added to the
	 * scene graph on startup
	 * @param node
	 */
	public void setContent(SceneGraphNode node) {
		if (node == null) {
			return;
		}
		if (!(node instanceof Geometry) && !(node instanceof SceneGraphComponent)) {
			throw new IllegalArgumentException("Only Geometry or SceneGraphComponent allowed in JRViewer.setContent()");
		}
		c.registerPlugin(new ContentInjectionPlugin(node));
	}
	
	/** The provided resource serves 2 purposes: 
	 * <ol>
	 * <li>to set the properties <code>File</code> and <code>InputStream</code> via {@link #setPropertiesFile(File)} 
	 * (if this resource allows write access) and {@link #setPropertiesInputStream(InputStream)} 
	 * (if this resource allows read access),</li>
	 * <li>to save and read user decisions about the reading and loading of the property file in a package specific node, via
	 * the <a href="http://java.sun.com/javase/6/docs/technotes/guides/preferences/index.html">Java Preferences API</a>.  
	 * </li>
	 * </ol> 
	 * 
	 * @param clazz the class from which the resource may be obtained, the 
	 * properties node of the package of this class is used to save the user decisions. 
	 * @param propertiesFileName name of the resource that contains the plugin properties. This argument may 
	 * be null, then only the second purpose is served and the properties <code>File</code> and 
	 * <code>InputStream</code> are NOT set to null and may be set independently (e.g. to specify a default file, when
	 * the resource gets read only after deployment).
	 */
	public void setPropertiesResource(Class<?> clazz, String propertiesFileName) {
		c.setPropertiesResource(clazz, propertiesFileName);
	}
	
	/**
	 * <p>Sets the properties File of the SimpleController. 
	 * 
	 * <p>WARNING: This method does not overwrite a file chosen by the the user. Moreover the user decisions are saved
	 * in the same place for all JRViewer applications, unless you use {@link #setPropertiesResource(Class, String)},
	 * where the class parameter allows to identify an application specific place to save user preferences.
	 * 
	 * @param filename a file or null
	 * @see #setPropertiesResource(Class, String)
	 */
	public void setPropertiesFile(String filename) {
		File f = new File(filename);
		setPropertiesFile(f);
	}
	

	/**
	 * <p>Sets the properties File of the SimpleController. This does not overwrite a file chosen by the the user
	 * and persisted as user properties.
	 * 
	 * <p>WARNING: This method does not overwrite a file chosen by the the user. Moreover the user decisions are saved
	 * in the same place for all JRViewer applications, unless you use {@link #setPropertiesResource(Class, String)},
	 * where the class parameter allows to identify an application specific place to save user preferences.
	 *
	 * @param file a file or null
	 * @see #setPropertiesResource(Class, String)
	 */
	public void setPropertiesFile(File file) {
		c.setPropertiesFile(file);
	}
	
	/**
	 * Sets the properties InputStream of the SimpleController. If also a properties <code>File</code> is provided
	 * the <code>InputStream</code> is used for reading the properties.
	 * 
	 * <p>WARNING: This method does not overwrite a file chosen by the the user. Moreover the user decisions are saved
	 * in the same place for all JRViewer applications, unless you use {@link #setPropertiesResource(Class, String)},
	 * where the class parameter allows to identify an application specific place to save user preferences.
	 *
	 * @param in an InputStream or null
	 * @see #setPropertiesResource(Class, String)
	 */
	public void setPropertiesInputStream(InputStream in) {
		c.setPropertiesInputStream(in);
	}
	
	/**
	 * Returns the controller of this JRViewer which is a SimpleController
	 * @return the SimpleController
	 */
	public SimpleController getController() {
		return c;
	}
	
	
	/**
	 * Starts this JRViewer's controller and installs all registered 
	 * plug-ins. Not registered but dependent plug-ins will be added
	 * automatically.
	 */
	public void startup() {
		c.startup();
	}
	
	/**
	 * Starts this JRViewer's controller and installs all registered 
	 * plug-ins. Not registered but dependent plug-ins will be added
	 * automatically.
	 * This method does not open the main window. Instead it returns the 
	 * root pane.
	 * @return JRootPane with the created viewer
	 */
	public JRootPane startupLocal() {
		return c.startupLocal();
	}
	
	
	/**
	 * Calls the dispose methods on the View and the ToolSystem Plug-ins
	 * to stop running Threads and free the resources.
	 * This JRViewer cannot be used again after calling this method.
	 * Do not call this method before startup.
	 */
	public void dispose() {
		ViewerSwitch view = getPlugin(View.class).getViewer();
		view.dispose();
        getPlugin(ToolSystemPlugin.class).getToolSystem().dispose();
        SelectionManagerImpl.disposeForViewer(view);
        getController().dispose();
	}
	
	
	/**
	 * Configures the visibility of the shrink panels slots
	 * @param left
	 * @param right
	 * @param top
	 * @param bottom
	 */
	public void setShowPanelSlots(boolean left, boolean right, boolean top, boolean bottom) {
		view.setShowLeft(left);
		view.setShowRight(right);
		view.setShowTop(top);
		view.setShowBottom(bottom);
	}
	
	/**
	 * Show or hide the menu bar
	 * @param show
	 */
	public void setShowMenuBar(boolean show) {
		viewPreferences.setShowMenuBar(show);
	}
	
	/**
	 * Show or hide the tool bar 
	 * @param show
	 */
	public void setShowToolBar(boolean show) {
		viewPreferences.setShowToolBar(show);
	}
	
	
	/**
	 * Registers a custom {@link Content} plug-in which is
	 * an implementation of the abstract class {@link Content}
	 * @param contentPlugin a content plug-in
	 */
	public void registerCustomContent(Content contentPlugin) {
		c.registerPlugin(contentPlugin);
	}
	
	
	/**
	 * Registers one of the predefined content plug-ins
	 * @param type a content enumeration type
	 */
	public void addContentSupport(ContentType type) {
		switch (type) {
			case Raw:
				c.registerPlugin(new DirectContent());
				break;
			case CenteredAndScaled:
				c.registerPlugin(new CenteredAndScaledContent());
				break;
			case TerrainAligned:
				c.registerPlugin(new TerrainAlignedContent());
				break;
			case Custom:
				break;
		}
	}
	
	/**
	 * Registers advanced content tools. Includes an appearance
	 * inspector, transformation tools, and file loaders
	 */
	public void addContentUI() {
		c.registerPlugin(new ContentTools());
		c.registerPlugin(new ContentAppearance());
		c.registerPlugin(new ContentLoader());
	}
	
	/**
	 * Basic UI support: scene graph inspector, bean shell,
	 * menu bar, view menu (with display options and background color), 
	 * export menu, camera settings menu, and properties menu.
	 */
	public void addBasicUI() {
		c.registerPlugin(new Inspector());
		
		c.registerPlugin(new BackgroundColor());
		c.registerPlugin(new DisplayOptions());
		c.registerPlugin(new ViewMenuBar());
		c.registerPlugin(new ViewToolBar());
		
		c.registerPlugin(new ExportMenu());
		c.registerPlugin(new CameraMenu());
		c.registerPlugin(new PropertiesMenu());
	}
	
	
	/**
	 * Adds a java bean shell to the lower slot
	 * Warning: this plug-in is incompatible with the python console plug-in
	 */
	public void addBeanShellSupport() {
		c.registerPlugin(Shell.class);
	}


	/**
	 * Virtual reality support. A sky box and environment map, 
	 * a terrain and a movable avatar
	 */
	public void addVRSupport() {
		c.registerPlugin(new Avatar());
		c.registerPlugin(new Terrain());
		c.registerPlugin(new Sky());
		//what does this do?
		getPlugin(Scene.class);
	}
	
	
	/**
	 * Sets the splash screen used during startup
	 * @param screen
	 */
	public void setSplashScreen(SplashScreen screen) {
		c.setSplashScreen(screen);
	}
	
	
	/**
	 * Quick display method with encompass
	 * @param node
	 */
	public static Viewer display(SceneGraphNode node) {
		JRViewer v = createJRViewer(node);
		v.startup();
		return v.getViewer();
	}
	
	
	/**
	 * Quick display method with encompass
	 * @param node
	 */
	public static JRViewer createJRViewer(SceneGraphNode node) {
		JRViewer v = new JRViewer();
		v.setPropertiesFile("JRViewer.xml");
		v.setPropertiesResource(JRViewer.class, "JRViewer.xml");
		v.registerPlugin(new DirectContent());
		v.registerPlugin(new ContentTools());
		
		if (node != null) {
			if (env != RunningEnvironment.DESKTOP)
				v.registerPlugin(new ContentInjectionPlugin(node, false));
			else v.registerPlugin(new ContentInjectionPlugin(node, true));
		} else {
			v.registerPlugin(new ContentLoader());
		}
		v.addBasicUI();
//		v.registerPlugin(new InfoOverlayPlugin());
		return v;
	}
	
	
	/** 
	 * Create a JRViewer that displays the provided <code>SceneGraphNode</code> in a
	 * virtual reality environment with movable avatar etc. ({@link #addBasicUISupport()}, 
	 * {@link #addVRSUpport()}, terrain aligned content, {@link ContentAppearance}, {@link ContentTools}).
	 * The created viewer is not started yet, so you need to call {@link #startup()} on the returned 
	 * <code>JRViewer</code>.
	 * 
	 * @see <a href="http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/ViewerVR/Navigation">ViewerVR Manual</a>
	 * 
	 * @param contentNode the scene graph component to be displayed or <code>null</code>.
	 * @return the created JRViewer.
	 */
	public static JRViewer createJRViewerVR(SceneGraphNode contentNode) {
		JRViewer v = new JRViewer();
		v.setPropertiesFile("JRViewer.xml");
		v.setPropertiesResource(JRViewer.class, "JRViewer.xml");
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentTools());
		if (contentNode != null) {
			v.setContent(contentNode);
		}
		return v;
	}

	
	/**
	 * Call after startup. Encompasses the view
	 */
	public void encompassEuclidean() {
		Scene scene = getPlugin(Scene.class);
		EncompassFactory ef = Scene.encompassFactoryForScene(scene);
		ef.setMetric(Pn.EUCLIDEAN);
		ef.setMargin(1.75);
		ef.update();
	}
	
	
	
	private static class ContentInjectionPlugin extends Plugin {

		private SceneGraphNode
			content = null;
		private boolean
			encompass = true;
		
		
		public ContentInjectionPlugin(SceneGraphNode content) {
			this.content = content; 
		}
		
		
		public ContentInjectionPlugin(SceneGraphNode content, boolean encompass) {
			this(content);
			this.encompass = encompass;
		}
		
		@Override
		public PluginInfo getPluginInfo() {
			return new PluginInfo("JRViewer Content Plugin", "jReality Group");
		}
		
		@Override
		public void install(Controller c) throws Exception {
			super.install(c);
			if (content == null) {
				return;
			}
			Content mc = JRViewerUtility.getContentPlugin(c);
			if (mc == null) {
				System.err.println("No content plug-in registered");
				return;
			}
			mc.setContent(content);
			if (encompass) {
				Scene scene = c.getPlugin(Scene.class);
				List<Terrain> list = c.getPlugins(Terrain.class);
				scene.setAutomaticClippingPlanes(list.size() == 0);
				JRViewerUtility.encompassEuclidean(scene);
				//check for Terrain plugin. If it is installed, don't cut it off
				//by an all to distant near clipping plane.
				//this fixes the a problem caused by v.setContent()
//				List<Terrain> list = c.getPlugins(Terrain.class);
//				if(list.size() == 0)
//					JRViewerUtility.encompassEuclidean(scene, true);
//				else
//					JRViewerUtility.encompassEuclidean(scene, false);
			}
		}
		
	}

	/**
	 * Starts the default plug-in viewer
	 * @param args no arguments are read
	 */
	public static void main(String[] args) {
		Set<String> params = new HashSet<String>();
		for (String param : args) params.add(param.toLowerCase());
		JRViewer v = new JRViewer();
		v.setPropertiesFile("JRViewer.xml");
		v.setPropertiesResource(JRViewer.class, "JRViewer.xml");
		v.addBasicUI();
//		v.registerPlugin(InfoOverlayPlugin.class);
		if (params.contains("-vr")) {
			v.addContentUI();
			v.addVRSupport();
			v.addContentSupport(ContentType.TerrainAligned);
			v.setShowPanelSlots(true, false, false, false);
			VRExamples vrExamples = new VRExamples();
			vrExamples.getShrinkPanel().setShrinked(false);
			v.registerPlugin(vrExamples);
		} else {
			v.registerPlugin(new ContentLoader());
			v.registerPlugin(new ContentTools());
			v.registerPlugin(new ContentAppearance());
			v.getPlugin(Inspector.class).setInitialPosition(ShrinkPanelPlugin.SHRINKER_LEFT);
			v.addContentSupport(ContentType.CenteredAndScaled);
			v.setShowPanelSlots(true, false, false, false);
		}
		v.startup();
	}

	/**
	 * @deprecated extend {@link SceneShrinkPanel} or {@link ViewShrinkPanelPlugin}
	 */
	public static SceneShrinkPanel createSceneShrinkPanel( final Component c, final String title) {
		SceneShrinkPanel p = new SceneShrinkPanel() {
			
			{
				GridLayout gl = new GridLayout();
				gl.setRows(1);
				setInitialPosition(SHRINKER_LEFT);
				shrinkPanel.setName(title);
				shrinkPanel.setLayout(gl);
				shrinkPanel.add(c);
			}
			
			@Override
			public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
				return View.class;
			}

			@Override
			public PluginInfo getPluginInfo() {
				return new PluginInfo(title);
			}
		};
		return p;

	}


}
