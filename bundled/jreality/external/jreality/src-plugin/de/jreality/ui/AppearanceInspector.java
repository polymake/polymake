package de.jreality.ui;

import static de.jreality.shader.CommonAttributes.Z_BUFFER_ENABLED;
import static java.awt.GridBagConstraints.BOTH;
import static java.awt.GridBagConstraints.REMAINDER;
import static java.awt.GridBagConstraints.VERTICAL;
import static java.awt.GridBagConstraints.WEST;
import static javax.swing.SwingConstants.HORIZONTAL;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Map;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import com.bric.swing.ColorPicker;

import de.jreality.math.Matrix;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.scene.Scene;
import de.jreality.shader.CommonAttributes;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

/** The gui component of the {@link ContentAppearance} plugin.
 *
 */
@SuppressWarnings("serial")
public class AppearanceInspector extends JPanel implements ActionListener, ChangeListener {

	/*
	 * maximal radius of tubes or points compared to content size
	 */
	private double 
		maximalRadius = 1.5,
		objectScale = 1.0;
	/*
	 * ratio of maximal versus minimal value for logarithmic sliders
	 */
	private int 
		logarithmicRange = 200;
	
	private JPanel
		mainPanel = new JPanel(),
		lineColorPanel = new JPanel(),
		pointColorPanel = new JPanel(),
		faceColorPanel = new JPanel();
	private ColorChooseJButton
		lineColorButton = new ColorChooseJButton(false),
		pointColorButton = new ColorChooseJButton(false),
		faceColorButton = new ColorChooseJButton(false);
	private ColorPicker
		lineColorChooser = new ColorPicker(false, false),
		pointColorChooser = new ColorPicker(false, false),
		faceColorChooser = new ColorPicker(false, false);
	private LabelsInspector
		faceFontInspector = new LabelsInspector("polygonShader"),
		lineFontInspector = new LabelsInspector("lineShader"),
		pointFontInspector = new LabelsInspector("pointShader");
	private JSliderVR
		tubeRadiusSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		sphereRadiusSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		linesReflectionSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		pointsReflectionSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		facesReflectionSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		transparencySlider = new JSliderVR(HORIZONTAL, 0, 100, 0);
	private JCheckBox 
		showLines = new JCheckBox("Visible"),
		showPoints = new JCheckBox("Visible"),
		showFaces = new JCheckBox("Visible"),
		transparency = new JCheckBox("Transp."),
		pointsReflecting = new JCheckBox("Reflection"),
		linesReflecting = new JCheckBox("Reflection"),
		facesReflecting = new JCheckBox("Reflection"),
		facesFlat = new JCheckBox("Flat Shading"),
		tubes = new JCheckBox ("Tubes"),
		spheres = new JCheckBox("Spheres");
	private JButton 
		closeLineColorsButton = new JButton("<-- Back"),
		closePointColorsButton = new JButton("<-- Back"),
		closeFaceColorsButton = new JButton("<-- Back");
	private Appearance appearance = new Appearance();
	private TextureInspector 
		textureInspector = new TextureInspector(54);
	private ShrinkPanel
		texturePanel = new ShrinkPanel("Texture");
	private GridBagConstraints
		c = new GridBagConstraints();
	private Insets
		insets = new Insets(1,0,1,0);

	
	public AppearanceInspector() {
		setLayout(new GridLayout());
		makePanel();
		add(mainPanel);
		
		// lines
		lineColorChooser.getColorPanel().addChangeListener(this);
		lineColorButton.addActionListener(this);
		closeLineColorsButton.addActionListener(this);
		showLines.addActionListener(this);
		linesReflecting.addActionListener(this);
		linesReflectionSlider.addChangeListener(this);
		tubeRadiusSlider.addChangeListener(this);
		tubes.addActionListener(this);
		
		// points
		pointColorChooser.getColorPanel().addChangeListener(this);
		pointColorButton.addActionListener(this);
		closePointColorsButton.addActionListener(this);
		showPoints.addActionListener(this);
		pointsReflecting.addActionListener(this);
		pointsReflectionSlider.addChangeListener(this);
		sphereRadiusSlider.addChangeListener(this);
		spheres.addActionListener(this);
		
		// faces
		faceColorChooser.getColorPanel().addChangeListener(this);
		faceColorButton.addActionListener(this);
		closeFaceColorsButton.addActionListener(this);
		showFaces.addActionListener(this);
		facesReflecting.addActionListener(this);
		facesReflectionSlider.addChangeListener(this);
		transparencySlider.addChangeListener(this);
		transparency.addActionListener(this);
		facesFlat.addActionListener(this);
		
		showFaces.setSelected(true);
		
		lineColorChooser.setMode(0);
		pointColorChooser.setMode(0);
		faceColorChooser.setMode(0);
	}
	
	
	private void makePanel() {
		mainPanel.setLayout(new ShrinkPanel.MinSizeGridBagLayout());
		c.fill = BOTH;
		c.insets = insets;
		c.weighty = 0.0;
		c.anchor = WEST;
		
		// lines
		ShrinkPanel linesPanel = new ShrinkPanel("Lines");
		linesPanel.setIcon(ImageHook.getIcon("shape_edges.png"));
		linesPanel.setShrinked(true);
//		linesPanel.setBorder(createTitledBorder("Lines"));
		linesPanel.setLayout(new GridBagLayout());
		
		c.gridwidth = 1;
		c.weightx = 0.0;
		linesPanel.add(showLines, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		linesPanel.add(lineColorButton, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		linesPanel.add(tubes, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		linesPanel.add(tubeRadiusSlider, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		linesPanel.add(linesReflecting, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		linesPanel.add(linesReflectionSlider, c);
		ShrinkPanel lineLabelShrinker = new ShrinkPanel("Labels");
		lineLabelShrinker.setIcon(ImageHook.getIcon("font.png"));
		lineLabelShrinker.setShrinked(true);
		lineLabelShrinker.setLayout(new GridLayout());
		lineLabelShrinker.add(lineFontInspector);
		linesPanel.add(lineLabelShrinker, c);

		// points
		ShrinkPanel pointsPanel = new ShrinkPanel("Points");
		pointsPanel.setIcon(ImageHook.getIcon("shape_handles.png"));
		pointsPanel.setShrinked(true);
//		pointsPanel.setBorder(createTitledBorder("Points"));
		pointsPanel.setLayout(new GridBagLayout());
		c.gridwidth = 1;
		c.weightx = 0.0;
		pointsPanel.add(showPoints, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		pointsPanel.add(pointColorButton, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		pointsPanel.add(spheres, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		pointsPanel.add(sphereRadiusSlider, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		pointsPanel.add(pointsReflecting, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		pointsPanel.add(pointsReflectionSlider, c);
		ShrinkPanel pointLabelShrinker = new ShrinkPanel("Labels");
		pointLabelShrinker.setIcon(ImageHook.getIcon("font.png"));
		pointLabelShrinker.setShrinked(true);
		pointLabelShrinker.setLayout(new GridLayout());
		pointLabelShrinker.add(pointFontInspector);
		pointsPanel.add(pointLabelShrinker, c);
		
		// faces
		ShrinkPanel facesPanel = new ShrinkPanel("Faces");
		facesPanel.setIcon(ImageHook.getIcon("shape_square.png"));
		facesPanel.setShrinked(true);
//		facesPanel.setBorder(createTitledBorder("Faces"));
		facesPanel.setLayout(new GridBagLayout());
		c.gridwidth = 1;
		c.weightx = 0.0;
		facesPanel.add(showFaces, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		facesPanel.add(faceColorButton, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		facesPanel.add(facesReflecting, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		facesPanel.add(facesReflectionSlider, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		facesPanel.add(transparency, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		facesPanel.add(transparencySlider, c);
		facesPanel.add(facesFlat, c);
		ShrinkPanel faceLabelShrinker = new ShrinkPanel("Labels");
		faceLabelShrinker.setIcon(ImageHook.getIcon("font.png"));
		faceLabelShrinker.setShrinked(true);
		faceLabelShrinker.setLayout(new GridLayout());
		faceLabelShrinker.add(faceFontInspector);
		facesPanel.add(faceLabelShrinker, c);
		
		texturePanel.setIcon(ImageHook.getIcon("photo.png"));
		texturePanel.setShrinked(true);
		texturePanel.setLayout(new GridLayout());
		texturePanel.add(textureInspector);
		
		c.fill = BOTH;
		c.gridwidth = REMAINDER;
		c.weighty = 1.0;
		c.weightx = 1.0;
		mainPanel.add(facesPanel, c);
		mainPanel.add(texturePanel, c);
		mainPanel.add(linesPanel, c);
		mainPanel.add(pointsPanel, c);
		
		
		// line color panel
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.insets = insets;
		c.gridwidth = REMAINDER;
		c.anchor = WEST;
		lineColorPanel.setLayout(new GridBagLayout());
		c.fill = BOTH;
		lineColorChooser.setPreferredSize(new Dimension(220, 230));
		lineColorPanel.add(lineColorChooser, c);
		c.fill = VERTICAL;
		c.weighty = 0.0;
		lineColorPanel.add(closeLineColorsButton, c);
		// point color panel
		pointColorPanel.setLayout(new GridBagLayout());
		c.fill = BOTH;
		pointColorChooser.setPreferredSize(new Dimension(220, 230));
		c.weighty = 1.0;
		pointColorPanel.add(pointColorChooser, c);
		c.fill = VERTICAL;
		c.weighty = 0.0;
		pointColorPanel.add(closePointColorsButton, c);
		// face color panel
		faceColorPanel.setLayout(new GridBagLayout());
		c.fill = BOTH;
		faceColorChooser.setPreferredSize(new Dimension(220, 230));
		c.weighty = 1.0;
		faceColorPanel.add(faceColorChooser, c);
		c.fill = VERTICAL;
		c.weighty = 0.0;
		faceColorPanel.add(closeFaceColorsButton, c);
	}

	
	@Override
	public void actionPerformed(ActionEvent e) {
		Object s = e.getSource();
		// lines
		if (showLines == s) {
			updateShowLines();
		} else if (linesReflecting == s) {
			updateLinesReflecting();
		} else if (tubes == s) {
			updateTubes();
		} else
		
		// points
		if (showPoints == s) {
			updateShowPoints();
		} else if (pointsReflecting == s) {
			updatePointsReflecting();
		} else if (spheres == s) {
			updateSpheres();
		} else
		
		// faces
		if (showFaces == s) {
			updateShowFaces();
		} else if (facesReflecting == s) {
			updateFacesReflecting();
		} else if (facesFlat == s) {
			updateFacesFlat();
		} else if (transparency == s) {
			updateTransparencyEnabled();
		} else
		
		// colors
		if (lineColorButton == s) {
			switchTo(lineColorPanel);
		} else if (pointColorButton == s) {
			switchTo(pointColorPanel);
		} else if (faceColorButton == s) {
			switchTo(faceColorPanel);
		} else
		
		// default back to main panel
		{
			switchTo(mainPanel);
		}
	}
	
	public void updateAll() {
		
		updateEnabledStates();
		
		// lines
			updateShowLines();
			updateLinesReflecting();
			updateTubes();
		
		// points
			updateShowPoints();
			updatePointsReflecting();
			updateSpheres();
		
		// faces
			updateShowFaces();
			updateFacesReflecting();
			updateFacesFlat();
			updateTransparencyEnabled();
	}
	
	@Override
	public void stateChanged(ChangeEvent e) {
		Object s = e.getSource();
		// lines
		if (lineColorChooser.getColorPanel() == s) {
			updateLineColor();
		} else if (linesReflectionSlider == s) {
			updateLineReflection();
		} else if (tubeRadiusSlider == s) {
			updateTubeRadius();
		} else
		
		// points
		if (pointColorChooser.getColorPanel() == s) {
			updatePointColor();
		} else if (pointsReflectionSlider == s) {
			updatePointReflection();
		} else if (sphereRadiusSlider == s) {
			updateSphereRadius();
		} else
		
		// faces
		if (faceColorChooser.getColorPanel() == s) {
			updateFaceColor();
		} else if (facesReflectionSlider == s) {
			updateFaceReflection();
		} else if (transparencySlider == s) {
			updateTransparency();
		}
	}

	
	public Map<String, String> getTextures() {
		return textureInspector.getTextures();
	}
	
	public void setTextures(Map<String, String> textures) {
		textureInspector.setTextures(textures);
	}
	
	
	public void updateEnabledStates() {
		lineColorButton.setEnabled(isShowLines());
		linesReflecting.setEnabled(isShowLines() && isTubes());
		linesReflectionSlider.setEnabled(isShowLines() && isLinesReflecting() && isTubes());
		tubes.setEnabled(isShowLines());
		tubeRadiusSlider.setEnabled(isShowLines());
		
		pointColorButton.setEnabled(isShowPoints());
		pointsReflecting.setEnabled(isShowPoints() && isSpheres());
		pointsReflectionSlider.setEnabled(isShowPoints() && isPointsReflecting() && isSpheres());
		spheres.setEnabled(isShowPoints());
		sphereRadiusSlider.setEnabled(isShowPoints());
		
		faceColorButton.setEnabled(isShowFaces());
		facesReflecting.setEnabled(isShowFaces());
		facesReflectionSlider.setEnabled(isShowFaces() && isFacesReflecting());
		transparency.setEnabled(isShowFaces());
		transparencySlider.setEnabled(isShowFaces() && isTransparencyEnabled());
		facesFlat.setEnabled(isShowFaces());
	}
	
	
	public void setColorPickerMode(int mode) {
		lineColorChooser.setMode(mode);
		pointColorChooser.setMode(mode);
		faceColorChooser.setMode(mode);
	}
	
	
	public Appearance getAppearance() {
		return appearance;
	}

	public void setAppearance(final Appearance appearance) {
		this.appearance = appearance;
		Scene.executeWriter(appearance, new Runnable() {
			
			@Override
			public void run() {
				textureInspector.setAppearance(appearance);
				pointFontInspector.setAppearance(appearance);
				lineFontInspector.setAppearance(appearance);
				faceFontInspector.setAppearance(appearance);
				updateShowPoints();
				updateShowLines();
				updateShowFaces();
				updatePointColor();
				updateLineColor();
				updateFaceColor();
				updatePointsReflecting();
				updateLinesReflecting();
				updateFacesReflecting();
				updatePointReflection();
				updateLineReflection();
				updateFaceReflection();
				updateSpheres();
				updateTubes();
				updateSphereRadius();
				updateTubeRadius();
				updateTransparencyEnabled();
				updateTransparency();
				updateFacesFlat();
			}
		});
	}

	public TextureInspector getTextureInspector() {
		return textureInspector;
	}
	
	public double getMaximalRadius() {
		return maximalRadius;
	}

	public double getObjectScale() {
		return objectScale;
	}

	public void setObjectScale(double d) {
		objectScale=d;
		updateSphereRadius();
		updateTubeRadius();
	}
	
	public void setMaximalRadius(double maximalRadius) {
		this.maximalRadius = maximalRadius;
		updateSphereRadius();
		updateTubeRadius();
	}

	public int getLogarithmicRange() {
		return logarithmicRange;
	}

	public void setLogarithmicRange(int logarithmicRange) {
		this.logarithmicRange = logarithmicRange;
		updateSphereRadius();
		updateTubeRadius();
	}
	
	public boolean isShowPoints() {
		return showPoints.isSelected();
	}
	public void setShowPoints(boolean selected) {
		showPoints.setSelected(selected);
		updateShowPoints();
	}
	private void updateShowPoints() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.VERTEX_DRAW,
					isShowPoints()
			);
		}
		updateEnabledStates();
	}
	
	public boolean isSpheres() {
		return spheres.isSelected();
	}
	public void setSpheres(boolean b) {
		spheres.setSelected(b);
	}
	private void updateSpheres() {
		boolean spheres = isSpheres();
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.SPHERES_DRAW,
					spheres
			);
		}
		updateEnabledStates();
	}
	
	public boolean isPointsReflecting() {
		return pointsReflecting.isSelected();
	}
	public void setPointsReflecting(boolean b) {
		pointsReflecting.setSelected(b);
	}
	private void updatePointsReflecting() {
		if (isPointsReflecting()) {
			updatePointReflection();
		} else {
			if (appearance != null) {
				appearance.setAttribute(
						"pointShader.reflectionMap:blendColor",
						new Color(1f, 1f, 1f, 0f)
				);
			}
		}
		updateEnabledStates();
	}
	
	public double getPointReflection() {
		return .01 * pointsReflectionSlider.getValue();
	}
	public void setPointReflection(double d) {
		pointsReflectionSlider.setValue((int)(100*d));
	}
	private void updatePointReflection() {
		if (appearance != null) {
			appearance.setAttribute(
					"pointShader.reflectionMap:blendColor",
					new Color(1f, 1f, 1f, (float) getPointReflection())
			);
		}
	}
	
	public double getPointRadius() {
		return .01 * sphereRadiusSlider.getValue();
	}
	public void setPointRadius(double d) {
		sphereRadiusSlider.setValue((int) (d * 100));
	}
	private void updateSphereRadius() {
		double r =
			Math.exp(Math.log(logarithmicRange) * getPointRadius()) /
			logarithmicRange * maximalRadius;
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.POINT_RADIUS,
					r
			);
			// 64 pixels is the maximum size for point sprites
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.POINT_SIZE,
					getPointRadius() * 64
			);
			appearance.setAttribute(CommonAttributes.LINE_SHADER + "." + 
					CommonAttributes.DEPTH_FUDGE_FACTOR, 1.0);
		}
	}
	
	public Color getPointColor() {
		int[] rgb = pointColorChooser.getRGB();
		return new Color(rgb[0], rgb[1], rgb[2]);
	}
	public void setPointColor(Color c) {
		pointColorChooser.setRGB(c.getRed(), c.getGreen(), c.getBlue());
	}
	private void updatePointColor() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR,
					getPointColor()
			);
		}
		pointColorButton.setColor(getPointColor());
	}
	
	public boolean isShowLines() {
		return showLines.isSelected();
	}
	public void setShowLines(boolean selected) {
		showLines.setSelected(selected);
		updateShowLines();
	}
	private void updateShowLines() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.EDGE_DRAW,
					isShowLines()
			);
		}
		updateEnabledStates();
	}
	
	public boolean isTubes() {
		return tubes.isSelected();
	}
	public void setTubes(boolean b) {
		tubes.setSelected(b);
	}
	private void updateTubes() {
		if (appearance != null) {
			boolean tubes = isTubes();
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "." +
					CommonAttributes.TUBES_DRAW,
					tubes
			);
		}
		updateEnabledStates();
	}

	public double getLineReflection() {
		return .01 * linesReflectionSlider.getValue();
	}
	public void setLineReflection(double d) {
		linesReflectionSlider.setValue((int)(100*d));
	}
	private void updateLineReflection() {
		if (appearance != null) {
			appearance.setAttribute(
					"lineShader.reflectionMap:blendColor",
					new Color(1f, 1f, 1f, (float) getLineReflection())
			);
		}
	}
	
	public double getTubeRadius() {
		return .01 * tubeRadiusSlider.getValue();
	}
	public void setTubeRadius(double d) {
		tubeRadiusSlider.setValue((int) (d * 100));
	}
	private void updateTubeRadius() {
		double r = Math.exp(Math.log(logarithmicRange) * getTubeRadius())
		/ logarithmicRange * maximalRadius;
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "."	+
					CommonAttributes.LINE_WIDTH,
					getTubeRadius() * 10
			);
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "."	+
					CommonAttributes.TUBE_RADIUS,
					r
			);
		}
	}
	
	public Color getLineColor() {
		int[] rgb = lineColorChooser.getRGB();
		return new Color(rgb[0], rgb[1], rgb[2]);
	}
	public void setLineColor(Color c) {
		lineColorChooser.setRGB(c.getRed(), c.getGreen(), c.getBlue());
	}
	private void updateLineColor() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR ,
					getLineColor()
			);
		}
		lineColorButton.setColor(getLineColor());
	}
	
	public boolean isLinesReflecting() {
		return linesReflecting.isSelected();
	}
	public void setLinesReflecting(boolean b) {
		linesReflecting.setSelected(b);
	}
	private void updateLinesReflecting() {
		if (isLinesReflecting()) {
			updateLineReflection();
		} else {
			if (appearance != null) {
				appearance.setAttribute(
						"lineShader.reflectionMap:blendColor",
						new Color(1f, 1f, 1f, 0f)
				);
			}
		}
		updateEnabledStates();
	}

	public boolean isShowFaces() {
		return showFaces.isSelected();
	}
	public void setShowFaces(boolean selected) {
		showFaces.setSelected(selected);
		if (appearance != null) {
			appearance.setAttribute("showFaces", selected);
		}
	}
	private void updateShowFaces() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.FACE_DRAW,
					isShowFaces()
			);
		}
		updateEnabledStates();
	}
	
	public boolean isFacesFlat() {
		return facesFlat.isSelected();
	}
	public void setFacesFlat(boolean b) {
		facesFlat.setSelected(b);
	}
	private void updateFacesFlat() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.SMOOTH_SHADING,
					!isFacesFlat()
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
			appearance.setAttribute(Z_BUFFER_ENABLED, true);
		}
		updateTransparency();
		updateEnabledStates();
	}
	
	public boolean isFacesReflecting() {
		return facesReflecting.isSelected();
	}
	public void setFacesReflecting(boolean b) {
		facesReflecting.setSelected(b);
	}
	private void updateFacesReflecting() {
		if (isFacesReflecting()) {
			updateFaceReflection();
		} else {
			if (appearance != null) {
				appearance.setAttribute(
						"polygonShader.reflectionMap:blendColor",
						new Color(1f, 1f, 1f, 0f)
				);
			}
		}
		updateEnabledStates();
	}
	
	public double getFaceReflection() {
		return .01 * facesReflectionSlider.getValue();
	}
	public void setFaceReflection(double d) {
		facesReflectionSlider.setValue((int)(100*d));
	}
	private void updateFaceReflection() {
		if (appearance != null) {
			appearance.setAttribute(
					"polygonShader.reflectionMap:blendColor",
					new Color(1f, 1f, 1f, (float) getFaceReflection())
			);
		}
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
					// TODO this is a workaround for a jogl backend bug
					isTransparencyEnabled() ? getTransparency() : 0.0
			);
		}
	}

	public Color getFaceColor() {
		int[] rgb = faceColorChooser.getRGB();
		return new Color(rgb[0], rgb[1], rgb[2]);
	}
	public void setFaceColor(Color c) {
		faceColorChooser.setRGB(c.getRed(), c.getGreen(), c.getBlue());
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
	
	public String getTexture() {
		return textureInspector.getActiveTexture();
	}
	public void setTexture(String texture) {
		textureInspector.setTexture(texture);
	}

	public double getTextureScaleU() {
		return textureInspector.getTextureUScale();
	}
	public void setTextureScaleU(double scale) {
		textureInspector.setTextureUScale(scale);
	}
	public double getTextureScaleV() {
		return textureInspector.getTextureVScale();
	}
	public void setTextureScaleV(double scale) {
		textureInspector.setTextureVScale(scale);
	}
	
	public boolean isTextureScaleLock() {
		return textureInspector.isTextureScaleLock();
	}
	public void setTextureScaleLock(boolean lock) {
		textureInspector.setTextureScaleLock(lock);
	}
	public double getTextureTranslationU() {
		return textureInspector.getTextureUTranslation();
	}
	public void setTextureTranslationU(double u) {
		textureInspector.setTextureUTranslation(u);
	}
	
	public double getTextureTranslationV() {
		return textureInspector.getTextureVTranslation();
	}
	public void setTextureTranslationV(double v) {
		textureInspector.setTextureVTranslation(v);
	}
	
	public double getTextureRotationAngle() {
		return textureInspector.getTextureRotation();
	}
	public void setTextureRotationAngle(double a) {
		textureInspector.setTextureRotation(a);
	}
	
	public double getTextureShearAngle() {
		return textureInspector.getTextureShear();
	}
	public void setTextureShearAngle(double a) {
		textureInspector.setTextureShear(a);
	}
	public Matrix getTextureMatrix() {
		return textureInspector.getTextureMatrix();
	}
	
	public void setShowPointLabels(boolean show) {
		pointFontInspector.setShowLabels(show);
	}
	
	public void setShowLineLabels(boolean show) {
		lineFontInspector.setShowLabels(show);
	}
	
	public void setShowFaceLabels(boolean show) {
		faceFontInspector.setShowLabels(show);
	}
	
	public boolean isShowPointLabels() {
		return pointFontInspector.isShowLabels();
	}

	public boolean isShowLineLabels() {
		return lineFontInspector.isShowLabels();
	}
	
	public boolean isShowFaceLabels() {
		return faceFontInspector.isShowLabels();
	}
	
	public void setPointLabelColor(Color color) {
		pointFontInspector.setFontColor(color);
	}
	
	public void setLineLabelColor(Color color) {
		lineFontInspector.setFontColor(color);
	}
	
	public void setFaceLabelColor(Color color) {
		faceFontInspector.setFontColor(color);
	}
	
	public Color getPointLabelColor() {
		return pointFontInspector.getFontColor();
	}
	
	public Color getLineLabelColor() {
		return lineFontInspector.getFontColor();
	}
	
	public Color getFaceLabelColor() {
		return faceFontInspector.getFontColor();
	}
	
	public double getPointLabelSize() {
		return pointFontInspector.getLabelSize();
	}
	
	public double getLineLabelSize() {
		return lineFontInspector.getLabelSize();
	}
	
	public double getFaceLabelSize() {
		return faceFontInspector.getLabelSize();
	}
	
	public void setPointLabelSize(double size) {
		pointFontInspector.setLabelSize(size);
	}
	
	public void setLineLabelSize(double size) {
		lineFontInspector.setLabelSize(size);
	}
	
	public void setFaceLabelSize(double size) {
		faceFontInspector.setLabelSize(size);
	}
	
	public double[] getPointLabelOffset() {
		return pointFontInspector.getLabelOffset();
	}
	
	public double[] getLineLabelOffset() {
		return lineFontInspector.getLabelOffset();
	}
	
	public double[] getFaceLabelOffset() {
		return faceFontInspector.getLabelOffset();
	}
	
	public void setPointLabelOffset(double[] size) {
		pointFontInspector.setLabelOffset(size);
	}
	
	public void setLineLabelOffset(double[] size) {
		lineFontInspector.setLabelOffset(size);
	}
	
	public void setFaceLabelOffset(double[] size) {
		faceFontInspector.setLabelOffset(size);
	}	
	
	public int getPointLabelResolution() {
		return pointFontInspector.getLabelResolution();
	}
	
	public int getLineLabelResolution() {
		return lineFontInspector.getLabelResolution();
	}
	
	public int getFaceLabelResolution() {
		return faceFontInspector.getLabelResolution();
	}
	
	public void setPointLabelResolution(int res) {
		pointFontInspector.setLabelResolution(res);
	}
	
	public void setLineLabelResolution(int res) {
		lineFontInspector.setLabelResolution(res);
	}
	
	public void setFaceLabelResolution(int res) {
		faceFontInspector.setLabelResolution(res);
	}
	
	private void switchTo(JComponent content) {
		removeAll();
		add(content);
		revalidate();
		repaint();
	}

	@Override
	public void updateUI() {
		super.updateUI();
		if (isShowing()) {
			SwingUtilities.updateComponentTreeUI(texturePanel);
			SwingUtilities.updateComponentTreeUI(lineColorPanel);
			SwingUtilities.updateComponentTreeUI(faceColorPanel);
			SwingUtilities.updateComponentTreeUI(pointColorPanel);
		}
	}
	
}
