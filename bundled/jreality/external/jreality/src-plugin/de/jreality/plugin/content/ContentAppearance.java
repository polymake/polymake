package de.jreality.plugin.content;

import java.awt.Color;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.InputEvent;
import java.util.HashMap;
import java.util.Map;

import javax.swing.AbstractAction;
import javax.swing.KeyStroke;

import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewPreferences;
import de.jreality.plugin.basic.ViewPreferences.ColorPickerModeChangedListener;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.scene.SceneShrinkPanel;
import de.jreality.scene.Appearance;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.AppearanceInspector;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

/** A plugin that adds an inspector for the appearance of the viewers contents.
 *
 */
public class ContentAppearance extends SceneShrinkPanel implements ColorPickerModeChangedListener {

	public static final boolean DEFAULT_SHOW_POINTS = true;
	public static final boolean DEFAULT_SHOW_POINT_LABELS = true;
	public static final boolean DEFAULT_POINTS_REFLECTING = true;
	public static final double DEFAULT_POINT_RADIUS = .5;
	public static final Color DEFAULT_POINT_COLOR = Color.blue;
	public static final Color DEFAULT_POINT_LABEL_COLOR = Color.black;
	public static final double DEFAULT_POINT_LABEL_SIZE = 0.5;
	public static final int DEFAULT_POINT_LABEL_RESOLUTION = 48;
	public static final double[] DEFAULT_POINT_LABEL_OFFSET = {0, 0, 0};
	public static final boolean DEFAULT_SHOW_LINES = true;
	public static final boolean DEFAULT_SHOW_LINE_LABELS = true;
	public static final boolean DEFAULT_LINES_REFLECTING = true;
	public static final double DEFAULT_TUBE_RADIUS = .4;
	public static final Color DEFAULT_LINE_COLOR = Color.red;
	public static final Color DEFAULT_LINE_LABEL_COLOR = Color.black;
	public static final double DEFAULT_LINE_LABEL_SIZE = 0.5;
	public static final int DEFAULT_LINE_LABEL_RESOLUTION = 48;
	public static final double[] DEFAULT_LINE_LABEL_OFFSET = {0, 0, 0};
	public static final boolean DEFAULT_SHOW_FACES = true;
	public static final boolean DEFAULT_SHOW_FACE_LABELS = true;
	public static final boolean DEFAULT_FACES_REFLECTING = true;
	public static final double DEFAULT_FACE_REFLECTION = .5;
	public static final double DEFAULT_LINE_REFLECTION = .3;
	public static final double DEFAULT_POINT_REFLECTION = .3;
	public static final Color DEFAULT_FACE_COLOR = Color.white;
	public static final Color DEFAULT_FACE_LABEL_COLOR = Color.black;
	public static final double DEFAULT_FACE_LABEL_SIZE = 0.5;
	public static final int DEFAULT_FACE_LABEL_RESOLUTION = 48;
	public static final double[] DEFAULT_FACE_LABEL_OFFSET = {0, 0, 0};
	public static final boolean DEFAULT_TRANSPARENCY_ENABLED = false;
	public static final double DEFAULT_TRANSPARENCY = .7;
	public static final boolean DEFAULT_FACES_FLAT = false;
	public static final boolean DEFAULT_TUBES = true;
	public static final boolean DEFAULT_SPHERES = true;
	public static final String DEFAULT_TEXTURE = "none";
	public static final double DEFAULT_TEXTURE_SCALE_U = 1.0;
	public static final double DEFAULT_TEXTURE_SCALE_V = 1.0;
	public static final boolean DEFAULT_TEXTURE_IS_SCALE_UNIFORM = true;
	public static final double DEFAULT_TEXTURE_TRANSLATION_U = 0.0;
	public static final double DEFAULT_TEXTURE_TRANSLATION_V = 0.0;
	public static final double DEFAULT_TEXTURE_ROTATION_ANGLE = 0.0;
	public static final double DEFAULT_TEXTURE_SHEAR_ANGLE = 0.0;

	private ViewPreferences
		viewPreferences = null;
	private AppearanceInspector 
		appearanceInspector = null;
	private AbstractJrToggleAction
		showVerticesAction = new AbstractJrToggleAction("Vertices") {
			private static final long 
				serialVersionUID = 1L;

			{
				setIcon(ImageHook.getIcon("shape_handles.png"));
				putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('1'));
			}
		
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean wasMouceClick = (e.getModifiers() & ActionEvent.MOUSE_EVENT_MASK) != 0;
				long keyMods = e.getModifiers() & ~ActionEvent.MOUSE_EVENT_MASK;
				if (!wasMouceClick && keyMods != 0) return;
				appearanceInspector.setShowPoints(showVerticesAction.isSelected());				
			}
		},
		showEdgesAction = new AbstractJrToggleAction("Edges") {
			private static final long 
				serialVersionUID = 1L;

			{
				setIcon(ImageHook.getIcon("shape_edges.png"));
				putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('2'));
			}
		
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean wasMouceClick = (e.getModifiers() & ActionEvent.MOUSE_EVENT_MASK) != 0;
				long keyMods = e.getModifiers() & ~ActionEvent.MOUSE_EVENT_MASK;
				if (!wasMouceClick && keyMods != 0) return;
				appearanceInspector.setShowLines(showEdgesAction.isSelected());				
			}
		},
		showFacesAction = new AbstractJrToggleAction("Faces") {
			private static final long 
				serialVersionUID = 1L;

			{
				setIcon(ImageHook.getIcon("shape_square.png"));
				putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('3'));
			}
		
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean wasMouceClick = (e.getModifiers() & ActionEvent.MOUSE_EVENT_MASK) != 0;
				long keyMods = e.getModifiers() & ~ActionEvent.MOUSE_EVENT_MASK;
				if (!wasMouceClick && keyMods != 0) return;
				appearanceInspector.setShowFaces(showFacesAction.isSelected());				
			}
		};
		private AbstractAction
			increasePointRadiusAction = new AbstractAction() {
				private static final long 
					serialVersionUID = 1L;
		
				{
					putValue(NAME, "Increase Point Size");
					putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('1', InputEvent.CTRL_DOWN_MASK));
				}
			
				public void actionPerformed(ActionEvent e) {
					double ps = appearanceInspector.getPointRadius();
					ps *= 1.2;
					appearanceInspector.setPointRadius(ps);
				}
			},
		decreasePointRadiusAction = new AbstractAction() {
				private static final long 
					serialVersionUID = 1L;
		
				{
					putValue(NAME, "Decrease Point Size");
					putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('1', InputEvent.ALT_DOWN_MASK));
				}
			
				public void actionPerformed(ActionEvent e) {
					double ps = appearanceInspector.getPointRadius();
					ps /= 1.2;
					appearanceInspector.setPointRadius(ps);
				}
			},
		increaseTubeRadiusAction = new AbstractAction() {
				private static final long 
					serialVersionUID = 1L;
		
				{
					putValue(NAME, "Increase Tube Size");
					putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('2', InputEvent.CTRL_DOWN_MASK));
				}
			
				public void actionPerformed(ActionEvent e) {
					double ps = appearanceInspector.getTubeRadius();
					ps *= 1.2;
					appearanceInspector.setTubeRadius(ps);
				}
			},
		decreaseTubeRadiusAction = new AbstractAction() {
				private static final long 
					serialVersionUID = 1L;
		
				{
					putValue(NAME, "Decrease Tube Size");
					putValue(ACCELERATOR_KEY, KeyStroke.getKeyStroke('2', InputEvent.ALT_DOWN_MASK));
				}
			
				public void actionPerformed(ActionEvent e) {
					double ps = appearanceInspector.getTubeRadius();
					ps /= 1.2;
					appearanceInspector.setTubeRadius(ps);
				}
			};
	
	private Map<String, String> 
		textures = new HashMap<String, String>();

	public AppearanceInspector getPanel() {
		return appearanceInspector;
	}

	public ContentAppearance() {
		textures.put("1 Metal Grid", "textures/boysurface.png");
		textures.put("2 Metal Floor", "textures/metal_basic88.png");
		textures.put("3 Chain-Link Fence", "textures/chainlinkfence.png");
		appearanceInspector = new AppearanceInspector();
		restoreDefaults();
	}
	double worldSize = 1.0;
	
	
	
	public void install(Scene scene) {
		final Appearance contentApp = scene.getContentAppearance();
		contentApp.setAttribute(CommonAttributes.RADII_WORLD_COORDINATES, true);
		contentApp.setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
		appearanceInspector.setAppearance(contentApp);
		contentApp.addAppearanceListener(new AppearanceListener() {
			public void appearanceChanged(AppearanceEvent ev) {
				boolean showV = (Boolean)contentApp.getAttribute(CommonAttributes.VERTEX_DRAW);
				showVerticesAction.setSelected(showV);
				boolean showE = (Boolean)contentApp.getAttribute(CommonAttributes.EDGE_DRAW);
				showEdgesAction.setSelected(showE);
				boolean showF = (Boolean)contentApp.getAttribute(CommonAttributes.FACE_DRAW);
				showFacesAction.setSelected(showF);
			}
		});
	}

	public void colorPickerModeChanged(int mode) {
		getPanel().setColorPickerMode(mode);
	}
	
	public void restoreDefaults() {
		appearanceInspector.setShowPoints(DEFAULT_SHOW_POINTS);
		appearanceInspector.setShowPointLabels(DEFAULT_SHOW_POINT_LABELS);
		appearanceInspector.setPointsReflecting(DEFAULT_POINTS_REFLECTING);
		appearanceInspector.setPointReflection(DEFAULT_POINT_REFLECTION);
		appearanceInspector.setPointRadius(DEFAULT_POINT_RADIUS);
		appearanceInspector.setPointColor(DEFAULT_POINT_COLOR);
		appearanceInspector.setPointLabelColor(DEFAULT_POINT_LABEL_COLOR);
		appearanceInspector.setPointLabelSize(DEFAULT_POINT_LABEL_SIZE);
		appearanceInspector.setPointLabelResolution(DEFAULT_POINT_LABEL_RESOLUTION);
		appearanceInspector.setPointLabelOffset(DEFAULT_POINT_LABEL_OFFSET);
		
		appearanceInspector.setShowLines(DEFAULT_SHOW_LINES);
		appearanceInspector.setShowLineLabels(DEFAULT_SHOW_LINE_LABELS);
		appearanceInspector.setLinesReflecting(DEFAULT_LINES_REFLECTING);
		appearanceInspector.setLineReflection(DEFAULT_LINE_REFLECTION);
		appearanceInspector.setTubeRadius(DEFAULT_TUBE_RADIUS);
		appearanceInspector.setLineColor(DEFAULT_LINE_COLOR);
		appearanceInspector.setLineLabelColor(DEFAULT_LINE_LABEL_COLOR);
		appearanceInspector.setLineLabelSize(DEFAULT_LINE_LABEL_SIZE);
		appearanceInspector.setLineLabelResolution(DEFAULT_LINE_LABEL_RESOLUTION);
		appearanceInspector.setLineLabelOffset(DEFAULT_LINE_LABEL_OFFSET);
		
		appearanceInspector.setShowFaces(DEFAULT_SHOW_FACES);
		appearanceInspector.setShowFaceLabels(DEFAULT_SHOW_FACE_LABELS);
		appearanceInspector.setFacesReflecting(DEFAULT_FACES_REFLECTING);
		appearanceInspector.setFaceReflection(DEFAULT_FACE_REFLECTION);
		appearanceInspector.setFaceColor(DEFAULT_FACE_COLOR);
		appearanceInspector.setFaceLabelColor(DEFAULT_FACE_LABEL_COLOR);
		appearanceInspector.setFaceLabelSize(DEFAULT_FACE_LABEL_SIZE);
		appearanceInspector.setFaceLabelResolution(DEFAULT_FACE_LABEL_RESOLUTION);
		appearanceInspector.setFaceLabelOffset(DEFAULT_FACE_LABEL_OFFSET);
		
		appearanceInspector.setTransparencyEnabled(DEFAULT_TRANSPARENCY_ENABLED);
		appearanceInspector.setTransparency(DEFAULT_TRANSPARENCY);
		appearanceInspector.setFacesFlat(DEFAULT_FACES_FLAT);
		appearanceInspector.setTubes(DEFAULT_TUBES);
		appearanceInspector.setSpheres(DEFAULT_SPHERES);
		appearanceInspector.setTexture(DEFAULT_TEXTURE);
		restoreTextureDefaults();
	}

	public void restoreTextureDefaults() {
		appearanceInspector.setTextureScaleLock(DEFAULT_TEXTURE_IS_SCALE_UNIFORM);
		appearanceInspector.setTextureScaleU(DEFAULT_TEXTURE_SCALE_U); 
		appearanceInspector.setTextureScaleV(DEFAULT_TEXTURE_SCALE_V);
		appearanceInspector.setTextureTranslationU(DEFAULT_TEXTURE_TRANSLATION_U);
		appearanceInspector.setTextureTranslationV(DEFAULT_TEXTURE_TRANSLATION_V);
		appearanceInspector.setTextureRotationAngle(DEFAULT_TEXTURE_ROTATION_ANGLE);
		appearanceInspector.setTextureShearAngle(DEFAULT_TEXTURE_SHEAR_ANGLE);
	}
	
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		appearanceInspector.setShowPoints(c.getProperty(getClass(), "showPoints", DEFAULT_SHOW_POINTS));
		appearanceInspector.setShowPointLabels(c.getProperty(getClass(), "showPointLabels", DEFAULT_SHOW_POINT_LABELS));
		appearanceInspector.setPointsReflecting(c.getProperty(getClass(), "pointsReflecting", DEFAULT_POINTS_REFLECTING));
		appearanceInspector.setPointReflection(c.getProperty(getClass(), "pointReflection", DEFAULT_POINT_REFLECTION));
		appearanceInspector.setPointRadius(c.getProperty(getClass(), "pointRadius", DEFAULT_POINT_RADIUS));
		appearanceInspector.setPointColor(c.getProperty(getClass(), "pointColor", DEFAULT_POINT_COLOR));
		appearanceInspector.setPointLabelColor(c.getProperty(getClass(), "pointLabelColor", DEFAULT_POINT_LABEL_COLOR));
		appearanceInspector.setPointLabelSize(c.getProperty(getClass(), "pointLabelSize", DEFAULT_POINT_LABEL_SIZE));
		appearanceInspector.setPointLabelResolution(c.getProperty(getClass(), "pointLabelResolution", DEFAULT_POINT_LABEL_RESOLUTION));
		appearanceInspector.setPointLabelOffset(c.getProperty(getClass(), "pointLabelOffset", DEFAULT_POINT_LABEL_OFFSET));
		
		appearanceInspector.setShowLines(c.getProperty(getClass(), "showLines", DEFAULT_SHOW_LINES));
		appearanceInspector.setShowLineLabels(c.getProperty(getClass(), "showLineLabels", DEFAULT_SHOW_LINE_LABELS));
		appearanceInspector.setLinesReflecting(c.getProperty(getClass(), "linesReflecting", DEFAULT_LINES_REFLECTING));
		appearanceInspector.setLineReflection(c.getProperty(getClass(), "lineReflection", DEFAULT_LINE_REFLECTION));
		appearanceInspector.setTubeRadius(c.getProperty(getClass(), "tubeRadius", DEFAULT_TUBE_RADIUS));
		appearanceInspector.setLineColor(c.getProperty(getClass(), "lineColor", DEFAULT_LINE_COLOR));
		appearanceInspector.setLineLabelColor(c.getProperty(getClass(), "lineLabelColor", DEFAULT_LINE_LABEL_COLOR));
		appearanceInspector.setLineLabelSize(c.getProperty(getClass(), "lineLabelSize", DEFAULT_LINE_LABEL_SIZE));
		appearanceInspector.setLineLabelResolution(c.getProperty(getClass(), "lineLabelResolution", DEFAULT_LINE_LABEL_RESOLUTION));
		appearanceInspector.setLineLabelOffset(c.getProperty(getClass(), "lineLabelOffset", DEFAULT_LINE_LABEL_OFFSET));
		
		appearanceInspector.setShowFaces(c.getProperty(getClass(), "showFaces", DEFAULT_SHOW_FACES));
		appearanceInspector.setShowFaceLabels(c.getProperty(getClass(), "showFaceLabels", DEFAULT_SHOW_FACE_LABELS));
		appearanceInspector.setFacesReflecting(c.getProperty(getClass(), "facesReflecting", DEFAULT_FACES_REFLECTING));
		appearanceInspector.setFaceReflection(c.getProperty(getClass(), "faceReflection", DEFAULT_FACE_REFLECTION));
		appearanceInspector.setFaceColor(c.getProperty(getClass(), "faceColor", DEFAULT_FACE_COLOR));
		appearanceInspector.setFaceLabelColor(c.getProperty(getClass(), "faceLabelColor", DEFAULT_FACE_LABEL_COLOR));
		appearanceInspector.setFaceLabelSize(c.getProperty(getClass(), "faceLabelSize", DEFAULT_FACE_LABEL_SIZE));
		appearanceInspector.setFaceLabelResolution(c.getProperty(getClass(), "faceLabelResolution", DEFAULT_FACE_LABEL_RESOLUTION));
		appearanceInspector.setFaceLabelOffset(c.getProperty(getClass(), "faceLabelOffset", DEFAULT_FACE_LABEL_OFFSET));
		
		appearanceInspector.setTransparencyEnabled(c.getProperty(getClass(), "transparencyEnabled", DEFAULT_TRANSPARENCY_ENABLED));
		appearanceInspector.setTransparency(c.getProperty(getClass(), "transparency", DEFAULT_TRANSPARENCY));
		appearanceInspector.setFacesFlat(c.getProperty(getClass(), "facesFlat", DEFAULT_FACES_FLAT));
		appearanceInspector.setTubes(c.getProperty(getClass(), "tubes", DEFAULT_TUBES));
		appearanceInspector.setSpheres(c.getProperty(getClass(), "spheres", DEFAULT_SPHERES));
		appearanceInspector.setTextures(c.getProperty(getClass(), "textures", textures));
		appearanceInspector.setTexture(c.getProperty(getClass(), "texture", DEFAULT_TEXTURE));
		appearanceInspector.setTextureScaleLock(c.getProperty(getClass(), "textureScaleLock", DEFAULT_TEXTURE_IS_SCALE_UNIFORM));
		appearanceInspector.setTextureScaleU(c.getProperty(getClass(), "textureScaleU", DEFAULT_TEXTURE_SCALE_U));
		appearanceInspector.setTextureScaleV(c.getProperty(getClass(), "textureScaleV", DEFAULT_TEXTURE_SCALE_V));
		appearanceInspector.setTextureTranslationU(c.getProperty(getClass(), "textureTranslationU", DEFAULT_TEXTURE_TRANSLATION_U));
		appearanceInspector.setTextureTranslationV(c.getProperty(getClass(), "textureTranslationV", DEFAULT_TEXTURE_TRANSLATION_V));
		appearanceInspector.setTextureRotationAngle(c.getProperty(getClass(), "textureRotation", DEFAULT_TEXTURE_ROTATION_ANGLE));
		appearanceInspector.setTextureShearAngle(c.getProperty(getClass(), "textureShear", DEFAULT_TEXTURE_SHEAR_ANGLE));
		
		appearanceInspector.updateAll();
		super.restoreStates(c);
	}
	
	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "showPoints", appearanceInspector.isShowPoints());
		c.storeProperty(getClass(), "showPointLabels", appearanceInspector.isShowPointLabels());
		c.storeProperty(getClass(), "pointsReflecting", appearanceInspector.isPointsReflecting());
		c.storeProperty(getClass(), "pointReflection", appearanceInspector.getPointReflection());
		c.storeProperty(getClass(), "pointRadius", appearanceInspector.getPointRadius());
		c.storeProperty(getClass(), "pointColor", appearanceInspector.getPointColor());
		c.storeProperty(getClass(), "pointLabelColor", appearanceInspector.getPointLabelColor());
		c.storeProperty(getClass(), "pointLabelSize", appearanceInspector.getPointLabelSize());
		c.storeProperty(getClass(), "pointLabelResolution", appearanceInspector.getPointLabelResolution());
		c.storeProperty(getClass(), "pointLabelOffset", appearanceInspector.getPointLabelOffset());
		
		c.storeProperty(getClass(), "showLines", appearanceInspector.isShowLines());
		c.storeProperty(getClass(), "showLineLabels", appearanceInspector.isShowLineLabels());
		c.storeProperty(getClass(), "linesReflecting", appearanceInspector.isLinesReflecting());
		c.storeProperty(getClass(), "lineReflection", appearanceInspector.getLineReflection());
		c.storeProperty(getClass(), "tubeRadius", appearanceInspector.getTubeRadius());
		c.storeProperty(getClass(), "lineColor", appearanceInspector.getLineColor());
		c.storeProperty(getClass(), "lineLabelColor", appearanceInspector.getLineLabelColor());
		c.storeProperty(getClass(), "lineLabelSize", appearanceInspector.getLineLabelSize());
		c.storeProperty(getClass(), "lineLabelResolution", appearanceInspector.getLineLabelResolution());
		c.storeProperty(getClass(), "lineLabelOffset", appearanceInspector.getLineLabelOffset());
		
		c.storeProperty(getClass(), "showFaces", appearanceInspector.isShowFaces());
		c.storeProperty(getClass(), "showFaceLabels", appearanceInspector.isShowFaceLabels());
		c.storeProperty(getClass(), "facesReflecting", appearanceInspector.isFacesReflecting());
		c.storeProperty(getClass(), "faceReflection", appearanceInspector.getFaceReflection());
		c.storeProperty(getClass(), "faceColor", appearanceInspector.getFaceColor());
		c.storeProperty(getClass(), "faceLabelColor", appearanceInspector.getFaceLabelColor());
		c.storeProperty(getClass(), "faceLabelSize", appearanceInspector.getFaceLabelSize());
		c.storeProperty(getClass(), "faceLabelResolution", appearanceInspector.getFaceLabelResolution());		
		c.storeProperty(getClass(), "faceLabelOffset", appearanceInspector.getFaceLabelOffset());		
		
		c.storeProperty(getClass(), "transparencyEnabled", appearanceInspector.isTransparencyEnabled());
		c.storeProperty(getClass(), "transparency", appearanceInspector.getTransparency());
		c.storeProperty(getClass(), "facesFlat", appearanceInspector.isFacesFlat());
		c.storeProperty(getClass(), "tubes", appearanceInspector.isTubes());
		c.storeProperty(getClass(), "spheres", appearanceInspector.isSpheres());
		c.storeProperty(getClass(), "textures", appearanceInspector.getTextures());
		c.storeProperty(getClass(), "texture", appearanceInspector.getTexture());
		c.storeProperty(getClass(), "textureScale", appearanceInspector.getTextureScaleU());
		c.storeProperty(getClass(), "textureScaleLock", appearanceInspector.isTextureScaleLock());
		c.storeProperty(getClass(), "textureScaleU", appearanceInspector.getTextureScaleU());
		c.storeProperty(getClass(), "textureScaleV", appearanceInspector.getTextureScaleV());
		c.storeProperty(getClass(), "textureTranslationU", appearanceInspector.getTextureTranslationU());
		c.storeProperty(getClass(), "textureTranslationV", appearanceInspector.getTextureTranslationV());
		c.storeProperty(getClass(), "textureRotation", appearanceInspector.getTextureRotationAngle());
		c.storeProperty(getClass(), "textureShear", appearanceInspector.getTextureShearAngle());
		super.storeStates(c);
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		install(c.getPlugin(Scene.class));
		viewPreferences = c.getPlugin(ViewPreferences.class);
		viewPreferences.addColorPickerChangedListener(this);
		getPanel().setColorPickerMode(viewPreferences.getColorPickerMode());
		if (viewPreferences.isResetTextureTransform()) {
			restoreTextureDefaults();
		}
		shrinkPanel.setLayout(new GridLayout());
		shrinkPanel.add(appearanceInspector);
		
		ViewToolBar mainToolbar = c.getPlugin(ViewToolBar.class);
		mainToolbar.addSeparator(getClass(), 1.705);
		mainToolbar.addTool(getClass(), 1.71, showVerticesAction.createToolboxItem());
		mainToolbar.addTool(getClass(), 1.72, showEdgesAction.createToolboxItem());
		mainToolbar.addTool(getClass(), 1.73, showFacesAction.createToolboxItem());
		ViewMenuBar menuBar = c.getPlugin(ViewMenuBar.class);
		menuBar.addMenuItem(getClass(), 0, showVerticesAction.createMenuItem(), "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 1, showEdgesAction.createMenuItem(), "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 2, showFacesAction.createMenuItem(), "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 0.1, increasePointRadiusAction, "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 0.2, decreasePointRadiusAction, "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 1.1, increaseTubeRadiusAction, "Content", "Appearance");
		menuBar.addMenuItem(getClass(), 1.2, decreaseTubeRadiusAction, "Content", "Appearance");
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		viewPreferences.removeColorPickerChangedListener(this);
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Content Appearance";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("lupeblau.png");
		return info; 
	}
	
	public AppearanceInspector getAppearanceInspector() {
		return appearanceInspector;
	}
	
	
	@Override
	public String getHelpDocument() {
		return "ContentAppearance.html";
	}
	
	@Override
	public String getHelpPath() {
		return "/de/jreality/plugin/help/";
	}
	
	@Override
	public Class<?> getHelpHandle() {
		return getClass();
	}

}

