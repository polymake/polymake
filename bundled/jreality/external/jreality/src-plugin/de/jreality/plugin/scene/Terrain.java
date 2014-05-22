package de.jreality.plugin.scene;

import static java.awt.GridBagConstraints.BOTH;
import static java.awt.GridBagConstraints.RELATIVE;
import static java.awt.GridBagConstraints.REMAINDER;
import static java.awt.GridBagConstraints.VERTICAL;
import static java.awt.GridBagConstraints.WEST;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Arrays;
import java.util.HashMap;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import com.bric.swing.ColorPicker;

import de.jreality.geometry.QuadMeshFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ViewPreferences;
import de.jreality.plugin.basic.ViewPreferences.ColorPickerModeChangedListener;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.ColorChooseJButton;
import de.jreality.ui.JSliderVR;
import de.jreality.ui.TextureInspector;
import de.jreality.util.PickUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel.MinSizeGridBagLayout;

public class Terrain extends Plugin implements ActionListener, ChangeListener, ColorPickerModeChangedListener {

	private static final boolean DEFAULT_TERRAIN_VISIBLE = true;
	private static final boolean DEFAULT_FACES_REFLECTING = true;
	private static final double DEFAULT_TRANSPARENCY = .5;
	private static final boolean DEFAULT_TRANSPARENCY_ENABLED = false;
	private static final double DEFAULT_FACE_REFLECTION = .5;
	private static final Color DEFAULT_TERRAIN_COLOR = Color.white;
	private static final double DEFAULT_TEXTURE_SCALE = .1;
	private static final String DEFAULT_TEXTURE = "3 Black Grid";

	// maximal value of texture scale
	private static final double 
		MAXIMAL_TEXTURE_SCALE = 1;
	// ratio of maximal value and minimal value of texture scale
	private static final double 
		LOGARITHMIC_RANGE = 200;
	
	// Plug-ins
	private ViewPreferences
		viewPreferences = null;
	
	// scene graph
	private SceneGraphComponent 
		terrain = new SceneGraphComponent("Terrain"),
		plane = new SceneGraphComponent("Terrain Plane");
	private Appearance 
		appearance = new Appearance("Terrain Appearance");
	private MirrorAppearance
		mirrorAppearance = new MirrorAppearance();
	
	// swing layout
	private ShrinkPanel
		shrinkPanel = new ShrinkPanel("Terrain");
	private JPanel 
		panel = new JPanel(),
		colorPanel = new JPanel();
	private JButton 
		closeButton = new JButton("<-- Back");
	private ColorChooseJButton
		faceColorButton = new ColorChooseJButton(false);
	private JCheckBox 
		facesReflecting = new JCheckBox("Reflection"),
		reflectScene = new JCheckBox("Reflect Scene Content"),
		transparency = new JCheckBox("Transp."),
		visibleCheckBox = new JCheckBox("Visible");
	private JSliderVR 
		faceReflectionSlider = new JSliderVR(SwingConstants.HORIZONTAL, 0, 100, 0),
		transparencySlider = new JSliderVR(SwingConstants.HORIZONTAL, 0, 100, 1);
	private ColorPicker 
		faceColorChooser = new ColorPicker(false, false);
	private TextureInspector 
		textureInspector = new TextureInspector(54);
	private ShrinkPanel 
		textureShrinker = new ShrinkPanel("Texture");

	private HashMap<String, String> 
		textures = new HashMap<String, String>();
	

	public Terrain() {
		appearance.setAttribute(CommonAttributes.EDGE_DRAW, false);
		appearance.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		terrain.setAppearance(appearance);

		MatrixBuilder.euclidean().rotateX(Math.PI/2).assignTo(plane);
		plane.setGeometry(bigMesh(50, 50, 2000));
		plane.getGeometry().setGeometryAttributes("infinite plane", Boolean.TRUE);
		PickUtility.assignFaceAABBTrees(plane);
		terrain.addChild(plane);

		textures.put("2 Grid", "textures/grid.jpeg");
		textures.put("3 Black Grid", "textures/gridBlack.jpg");
		textures.put("4 Tiles", "textures/recycfloor1_clean2.png");
		textures.put("5 Rust","textures/outfactory3.png");
		textures.put("1 None", null);

		shrinkPanel.setLayout(new GridLayout());
		shrinkPanel.setIcon(getPluginInfo().icon);
		shrinkPanel.setShrinked(true);
		shrinkPanel.add(panel);
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(1,0,1,0);
		c.fill = BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.gridwidth = REMAINDER;
		c.anchor = WEST;
		colorPanel.setLayout(new GridBagLayout());
		faceColorChooser.setMinimumSize(new Dimension(10, 230));
		faceColorChooser.setPreferredSize(new Dimension(10, 230));
		colorPanel.add(faceColorChooser, c);
		c.fill = VERTICAL;
		c.weighty = 0.0;
		colorPanel.add(closeButton, c);
		
		// panel
		panel.setLayout(new MinSizeGridBagLayout());
		c.fill = BOTH;

		visibleCheckBox.setSelected(DEFAULT_TERRAIN_VISIBLE);
		c.weightx = 0.0;
		c.gridwidth = RELATIVE;
		panel.add(visibleCheckBox, c);
		c.weightx = 1.0;
		c.gridwidth = REMAINDER;
		panel.add(faceColorButton, c);
		
		c.weightx = 0.0;
		c.weighty = 0.0;
		c.gridwidth = RELATIVE;
		panel.add(facesReflecting, c);
		c.weightx = 1.0;
		c.gridwidth = REMAINDER;
		panel.add(faceReflectionSlider, c);
		
		c.weightx = 0.0;
		c.gridwidth = RELATIVE;
		panel.add(transparency, c);
		c.weightx = 1.0;
		c.gridwidth = REMAINDER;
		panel.add(transparencySlider, c);
		
		c.weightx = 1.0;
		c.gridwidth = REMAINDER;	
		panel.add(reflectScene, c);
		reflectScene.setEnabled(false);
		reflectScene.setToolTipText("Coming soon...");
		
		textureInspector.setMaximalTextureScale(MAXIMAL_TEXTURE_SCALE);
		textureInspector.setLogarithmicRange(LOGARITHMIC_RANGE);
		textureInspector.setTextureUScale(DEFAULT_TEXTURE_SCALE );
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.gridwidth = REMAINDER;
		textureShrinker.setIcon(ImageHook.getIcon("photo.png"));
		textureShrinker.setShrinked(true);
		textureShrinker.setLayout(new GridLayout());
		textureShrinker.add(textureInspector);
		panel.add(textureShrinker, c);
		
		closeButton.addActionListener(this);
		visibleCheckBox.addActionListener(this);
		facesReflecting.addActionListener(this);
		transparency.addActionListener(this);
		faceColorButton.addActionListener(this);
		reflectScene.addActionListener(this);
		faceReflectionSlider.addChangeListener(this);
		transparencySlider.addChangeListener(this);
		faceColorChooser.getColorPanel().addChangeListener(this);
	}
	
	public void actionPerformed(ActionEvent e) {
		Object s = e.getSource();
		if (closeButton == s) {
			switchTo(panel);
		} else
		if (visibleCheckBox == s) {
			updateVisible();
		} else 
		if (facesReflecting == s) {
			updateFacesReflecting();
		} else 
		if (transparency == s) {
			updateTransparencyEnabled();
		} else 
		if (faceColorButton == s) {
			switchTo(colorPanel);
		} else
		if (reflectScene == s) {
			updateReflectSceneContent();
		}
	}
	
	public void stateChanged(ChangeEvent e) {
		Object s = e.getSource();
		if (faceReflectionSlider == s) {
			updateFaceReflection();
		} else 
		if (transparencySlider == s) {
			updateTransparency();
		} else 
		if (faceColorChooser.getColorPanel() == s) {
			updateFaceColor();
		}
	}
	
	void updateAll() {
		updateVisible();
		updateFacesReflecting();
		updateTransparencyEnabled();
		updateReflectSceneContent();
		updateFaceReflection();
		updateTransparency();
		updateFaceColor();
	}

	public void colorPickerModeChanged(int mode) {
		faceColorChooser.setMode(mode);		
	}

	
	public TextureInspector getTextureInspector() {
		return textureInspector;
	}
	
	public JPanel getPanel() {
		return panel;
	}

	public boolean isFacesReflecting() {
		return facesReflecting.isSelected();
	}

	public void setFacesReflecting(boolean b) {
		facesReflecting.setSelected(b);
	}
	
	public void setReflectSceneContent(boolean b) {
		reflectScene.setSelected(b);
	}
	
	
	private void updateFacesReflecting() {
		if (isFacesReflecting()) {
			updateFaceReflection();
		} else {
			appearance.setAttribute(
					"polygonShader.reflectionMap:blendColor",
					new Color(1f, 1f, 1f, 0f)
			);
		}
	}
	
	public double getFaceReflection() {
		return .01 * faceReflectionSlider.getValue();
	}

	public void setFaceReflection(double d) {
		faceReflectionSlider.setValue((int)(100*d));
	}

	public boolean isReflectSceneContent() {
		return reflectScene.isSelected();
	}
	
	
	private void updateFaceReflection() {
		if (isFacesReflecting()) {
			appearance.setAttribute(
					"polygonShader.reflectionMap:blendColor",
					new Color(1f, 1f, 1f, (float) getFaceReflection())
			);
		}
	}

	public Color getFaceColor() {
		return faceColorChooser.getColor();
	}

	public void setFaceColor(Color c) {
		faceColorChooser.setColor(c);
	}
	
	private void updateFaceColor() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR,
					getFaceColor()
			);
		}
		faceColorButton.setColor(getFaceColor());
	}

	public double getTransparency() {
		return .01 * transparencySlider.getValue();
	}

	public void setTransparency(double d) {
		transparencySlider.setValue((int)(100 * d));
	}

	private void updateTransparency() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.TRANSPARENCY,
					getTransparency()
			);
		}
	}
	
	public boolean isTransparencyEnabled() {
		return transparency.isSelected();
	}

	public void setTransparencyEnabled(boolean b) {
		transparency.setSelected(b);
	}
	
	private void updateTransparencyEnabled() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.TRANSPARENCY_ENABLED,
					isTransparencyEnabled()
			);
		}
	}
	
	private void updateReflectSceneContent() {
		boolean reflect = reflectScene.isSelected();
		if (reflect) {
			terrain.setAppearance(mirrorAppearance);
			
		} else {
			terrain.setAppearance(appearance);
		}
	}

	private void switchTo(JComponent content) {
		shrinkPanel.removeAll();
		shrinkPanel.add(content);
		shrinkPanel.getContentPanel().revalidate();
		shrinkPanel.getContentPanel().repaint();
		shrinkPanel.revalidate();
		shrinkPanel.repaint();
	}

	public SceneGraphComponent getSceneGraphComponent() {
		return terrain;
	}

	public boolean isVisible() {
		return visibleCheckBox.isSelected();
	}

	public void setVisible(boolean b) {
		visibleCheckBox.setSelected(b);
		updateVisible();
	}
	
	private void updateVisible() {
		terrain.setVisible(isVisible());
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		
		// scene
		Scene scene = c.getPlugin(Scene.class);
		scene.getBackdropComponent().addChild(terrain);
		scene.setAutomaticClippingPlanes(false);
		
		viewPreferences = c.getPlugin(ViewPreferences.class);
		viewPreferences.addColorPickerChangedListener(this);
		faceColorChooser.getColorPanel().setMode(viewPreferences.getColorPickerMode());
		
		
		textureInspector.setAppearance(appearance);
		updateFaceColor();
		updateFacesReflecting();
		updateFaceReflection();
		updateTransparencyEnabled();
		updateTransparency();
		VRPanel vp = c.getPlugin(VRPanel.class);
		vp.addComponent(getClass(), shrinkPanel, 1.0, "VR");

	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		Scene scene = c.getPlugin(Scene.class);
		scene.getBackdropComponent().removeChild(terrain);
		viewPreferences.removeColorPickerChangedListener(this);
		VRPanel vp = c.getPlugin(VRPanel.class);
		vp.removeAll(getClass());
	}
	
	public Appearance getAppearance() {
		return appearance;
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Terrain";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("vr/world.png");
		return info; 
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setFaceColor(c.getProperty(getClass(), "faceColor", DEFAULT_TERRAIN_COLOR));
		setTransparency(c.getProperty(getClass(), "transparency", DEFAULT_TRANSPARENCY));
		setTransparencyEnabled(c.getProperty(getClass(), "transparencyEnabled", DEFAULT_TRANSPARENCY_ENABLED));
		setFacesReflecting(c.getProperty(getClass(), "facesReflecting", DEFAULT_FACES_REFLECTING));
		setFaceReflection(c.getProperty(getClass(), "faceReflection", DEFAULT_FACE_REFLECTION));
		setReflectSceneContent(c.getProperty(getClass(), "reflectSceneContent", false));
		textureInspector.setTextures(c.getProperty(getClass(), "textures", textures));
		textureInspector.setTexture(c.getProperty(getClass(), "texture", DEFAULT_TEXTURE));
		textureInspector.setTextureUScale(c.getProperty(getClass(), "textureScale",DEFAULT_TEXTURE_SCALE));
		setVisible(c.getProperty(getClass(), "visible", true));
		updateAll();
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "textures", textureInspector.getTextures());
		c.storeProperty(getClass(), "texture", textureInspector.getActiveTexture());
		c.storeProperty(getClass(), "textureScale", textureInspector.getTextureUScale());
		c.storeProperty(getClass(), "visible", isVisible());
		c.storeProperty(getClass(), "faceColor", getFaceColor());
		c.storeProperty(getClass(), "transparency", getTransparency());
		c.storeProperty(getClass(), "transparencyEnabled", isTransparencyEnabled());
		c.storeProperty(getClass(), "facesReflecting", isFacesReflecting());
		c.storeProperty(getClass(), "faceReflection", getFaceReflection());
		c.storeProperty(getClass(), "reflectSceneContent", isReflectSceneContent());
		c.storeProperty(getClass(), "visible", isVisible());
		super.storeStates(c);
	}
	
	
	private static IndexedFaceSet bigMesh(int discretization, double cameraHeight, double size) {
		int n = discretization;
		QuadMeshFactory factory = new QuadMeshFactory();
		factory.setULineCount(n);
		factory.setVLineCount(n);
		factory.setGenerateEdgesFromFaces(true);
		factory.setGenerateTextureCoordinates(false);
		double totalAngle = Math.atan(size/cameraHeight);
		double dt = 2 * totalAngle/(n-1);
		double[] normal = new double[]{0,0,-1};
		double[][] normals = new double[n*n][];
		Arrays.fill(normals, normal);
		
		double[][][] coords = new double[n][n][3];

		for (int i=0; i<n; i++) {
			double y = cameraHeight * Math.tan(-totalAngle + i * dt);
			for (int j=0; j<n; j++) {
				coords[i][j][0] = cameraHeight * Math.tan(-totalAngle + j * dt);
				coords[i][j][1] = y;
			}
		}
		
		double[][][] texCoords = new double[n][n][2];
		for (int i=0; i<n; i++) {
			for (int j=0; j<n; j++) {
				texCoords[i][j][0] = coords[i][j][0];
				texCoords[i][j][1] = coords[i][j][1];
			}
		}
		
		factory.setVertexCoordinates(coords);
		factory.setVertexNormals(normals);
		factory.setVertexTextureCoordinates(texCoords);
		factory.update();
		
		return factory.getIndexedFaceSet();
	}

}
