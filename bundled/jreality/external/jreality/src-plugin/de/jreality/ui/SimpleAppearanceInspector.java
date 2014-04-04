package de.jreality.ui;

import static de.jreality.scene.Appearance.INHERITED;
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

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.ColorChooseJButton.ColorChangedEvent;
import de.jreality.ui.ColorChooseJButton.ColorChangedListener;

/** The gui component of an {@link Appearance} plugin.
 *
 */
public class SimpleAppearanceInspector extends JPanel implements ActionListener, ChangeListener, ColorChangedListener {

	private static final long serialVersionUID = 1L;
	
	private enum LinesState {
		HIDE("Hide"), LINES("Lines"), TUBES("Tubes");

		private String displayName = "";
		
		private LinesState(String str) {
			displayName = str;
		}
		
		@Override
		public String toString() {
			return displayName;
		}
	}
	
	private enum VertexState {
		HIDE("Hide"), POINTS("Points"), SPHERES("Spheres");

		private String displayName = "";
		
		private VertexState(String str) {
			displayName = str;
		}
		
		@Override
		public String toString() {
			return displayName;
		}
	}
	
	private enum FaceState {
		HIDE("Hide"), FLAT("Flat"), SMOOTH("Smooth");

		private String displayName = "";
		
		private FaceState(String str) {
			displayName = str;
		}
		
		@Override
		public String toString() {
			return displayName;
		}
	}
	
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
		vertexLabelsPanel = new JPanel(),
		lineLabelsPanel = new JPanel(),
		faceLabelsPanel = new JPanel();

	private ColorChooseJButton
		lineColorButton = new ColorChooseJButton(true),
		pointColorButton = new ColorChooseJButton(true),
		faceColorButton = new ColorChooseJButton(true);
	
	private JSliderVR
		tubeRadiusSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		sphereRadiusSlider = new JSliderVR(HORIZONTAL, 0, 100, 0),
		transparencySlider = new JSliderVR(HORIZONTAL, 0, 100, 0);

	private JCheckBox 
		lines = new JCheckBox("Lines"),
		points = new JCheckBox("Points"),
		faces = new JCheckBox("Faces"),
		transparency = new JCheckBox("Transp.");
	
	private JButton
		linesButton = new JButton(LinesState.TUBES.toString()),
		lineInspectorButton = new JButton(ImageHook.getIcon("font.png")),
		closeLineInspectorButton = new JButton("Done"),
		vertexButton = new JButton(VertexState.SPHERES.toString()),
		vertexInspectorButton = new JButton(ImageHook.getIcon("font.png")),
		closeVertexInspectorButton = new JButton("Done"),
		facesButton = new JButton(FaceState.FLAT.toString()),
		faceInspectorButton = new JButton(ImageHook.getIcon("font.png")),
		closeFaceInspectorButton = new JButton("Done");
	
	private LabelsInspector
		faceFontInspector = new LabelsInspector("polygonShader"),
		lineFontInspector = new LabelsInspector("lineShader"),
		vertexFontInspector = new LabelsInspector("pointShader");
	
	private Appearance appearance = new Appearance();
	
	private GridBagConstraints
		c = new GridBagConstraints();
	
	private Insets
		insets = new Insets(1,0,1,0);
	
	public SimpleAppearanceInspector() {
		setLayout(new GridLayout(0,1));
		makePanel();
		makeLabelPanels();

		
		add(mainPanel);
		
		// lines
		lines.addActionListener(this);
		linesButton.addActionListener(this);
		lineInspectorButton.addActionListener(this);
		lineInspectorButton.setPreferredSize(new Dimension(20,20));
		closeLineInspectorButton.addActionListener(this);
		lineColorButton.addColorChangedListener(this);
		lineInspectorButton.addActionListener(this);
//		showLines.addActionListener(this);
		tubeRadiusSlider.addChangeListener(this);
		tubeRadiusSlider.setPreferredSize(new Dimension(0,20));
		
		// points
		points.addActionListener(this);
		vertexButton.addActionListener(this);
		vertexInspectorButton.addActionListener(this);
		vertexInspectorButton.setPreferredSize(new Dimension(20,20));
		pointColorButton.addColorChangedListener(this);
		closeVertexInspectorButton.addActionListener(this);
		sphereRadiusSlider.addChangeListener(this);
		sphereRadiusSlider.setPreferredSize(new Dimension(0,20));
		
		// faces
		faces.addActionListener(this);
		facesButton.addActionListener(this);
		faceInspectorButton.addActionListener(this);
		faceInspectorButton.setPreferredSize(new Dimension(20,20));
		faceColorButton.addColorChangedListener(this);
		closeFaceInspectorButton.addActionListener(this);
		transparencySlider.addChangeListener(this);
		transparencySlider.setPreferredSize(new Dimension(0,10));
		transparency.addActionListener(this);

		vertexFontInspector.setFontColor(Color.black);
		lineFontInspector.setFontColor(Color.black);
		faceFontInspector.setFontColor(Color.black);
	}
	
	
	private void makeLabelPanels() {
		makeLabelPanel(vertexLabelsPanel, vertexFontInspector, closeVertexInspectorButton);
		makeLabelPanel(lineLabelsPanel, lineFontInspector, closeLineInspectorButton);
		makeLabelPanel(faceLabelsPanel, faceFontInspector, closeFaceInspectorButton);
		
	}


	private void makeLabelPanel(JPanel panel, JPanel inspector, JButton returnButton) {
		panel.setLayout(new GridBagLayout());
		GridBagConstraints c1 = new GridBagConstraints();
		c1.weightx = 1.0;
		c1.weighty = 1.0;
		c1.insets = new Insets(1, 0, 1, 0);
		c1.gridwidth = REMAINDER;
		c1.anchor = GridBagConstraints.EAST;
		c1.fill = BOTH;
		panel.add(inspector, c1);
		c1.fill = VERTICAL;
		c1.weighty = 0.0;
		panel.add(returnButton, c1);
	}


	private void makePanel() {
		mainPanel.setLayout(new GridBagLayout());
		c.fill = BOTH;
		c.insets = insets;
		c.weighty = 0.0;
		c.anchor = WEST;
		
		// lines
		
		c.gridwidth = 4;
		c.weightx = 0.0;
		mainPanel.add(lines, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		mainPanel.add(linesButton, c);
		c.weightx = 0.0;
		mainPanel.add(lineColorButton, c);
		c.gridwidth = REMAINDER;
		mainPanel.add(lineInspectorButton,c);
		c.weightx = 1.0;
		mainPanel.add(tubeRadiusSlider, c);

		// points
		c.gridwidth = 4;
		c.weightx = 0.0;
		mainPanel.add(points, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		mainPanel.add(vertexButton, c);
		c.weightx = 0.0;
		mainPanel.add(pointColorButton, c);
		c.gridwidth = REMAINDER;
		c.weightx = 0.0;
//		vertexInspectorButton.setSize(32, 32);
		mainPanel.add(vertexInspectorButton,c);
		c.weightx = 1.0;
		mainPanel.add(sphereRadiusSlider, c);
		
		// faces
		c.gridwidth = 4;
		c.weightx = 0.0;
		mainPanel.add(faces, c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		mainPanel.add(facesButton, c);
		c.weightx = 0.0;
		mainPanel.add(faceColorButton, c);
		c.gridwidth = REMAINDER;
		c.weightx = 0.0;
		mainPanel.add(faceInspectorButton,c);
		c.gridwidth = 1;
		c.weightx = 0.0;
		mainPanel.add(transparency, c);
		c.gridwidth = REMAINDER;
		c.weightx = 1.0;
		mainPanel.add(transparencySlider, c);

		updateEnabledStates();
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		Object s = e.getSource();
		// lines
		if (lines == s) {
			if(isEditLines()) {
				updateLines();
			} else {
				resetLinesToInherited();
			}
			
		} else 
		if (linesButton == s) {
			if(linesButton.getText().equals("Hide")) {
				linesButton.setText("Lines");
			} else if(linesButton.getText().equals("Lines")) {
				linesButton.setText("Tubes");
			} else if(linesButton.getText().equals("Tubes")) {
				linesButton.setText("Hide");
			}
			updateLines();
		} else if (lineInspectorButton == s) {
			switchTo(lineLabelsPanel);
		} else
		// points
		if (points == s) {
			if(isEditPoints()) {
				updatePoints();
			} else {
				resetPointsToInherited();
			}
		} else if (vertexButton == s) {
			if(vertexButton.getText().equals(VertexState.HIDE.toString())) {
				vertexButton.setText(VertexState.POINTS.toString());
			} else if(vertexButton.getText().equals(VertexState.POINTS.toString())) {
				vertexButton.setText(VertexState.SPHERES.toString());
			} else if(vertexButton.getText().equals(VertexState.SPHERES.toString())) {
				vertexButton.setText(VertexState.HIDE.toString());
			}
			updatePoints();
		} else if (vertexInspectorButton == s) {
			switchTo(vertexLabelsPanel);
		} else		
		// faces
		if (faces == s) {
			if(isEditFaces()) {
				updateFaces();
			} else {
				resetFacesToInherited();
			}
		} else if (facesButton == s) {
			if(facesButton.getText().equals(FaceState.HIDE.toString())) {
				facesButton.setText(FaceState.FLAT.toString());
			} else if(facesButton.getText().equals(FaceState.FLAT.toString())) {
				facesButton.setText(FaceState.SMOOTH.toString());
			} else if(facesButton.getText().equals(FaceState.SMOOTH.toString())) {
				facesButton.setText(FaceState.HIDE.toString());
			}
			updateFaces();
		} else if (transparency == s) {
			updateTransparencyEnabled();
		} else
		if (lineColorButton == s) {
			updateLineColor();
		} else if (pointColorButton == s) {
			updatePointColor();
		} else if (faceColorButton == s) {
			updateFaceColor();
		} else if (faceInspectorButton == s) {
			switchTo(faceLabelsPanel);
		} else 
		// for done buttons.
		{
			switchTo(mainPanel);
		}
		updateEnabledStates();
	}
	
	@Override
	public void stateChanged(ChangeEvent e) {
		Object s = e.getSource();
		// lines
		if (tubeRadiusSlider == s) {
			updateTubeRadius();
		} else

		// points
		if (sphereRadiusSlider == s) {
			updateSphereRadius();
		} else
		
		// faces
		if (transparencySlider == s) {
			updateTransparency();
		}
	}

	
	public void updateEnabledStates() {
		lineColorButton.setEnabled(isEditLines());
		linesButton.setEnabled(isEditLines());
		tubeRadiusSlider.setEnabled(isEditLines());
		
		pointColorButton.setEnabled(isEditPoints());
		vertexButton.setEnabled(isEditPoints());
		sphereRadiusSlider.setEnabled(isEditPoints());
		
		faceColorButton.setEnabled(isEditFaces());
		transparency.setEnabled(isEditFaces());
		transparencySlider.setEnabled(isEditFaces() && isTransparencyEnabled());
		facesButton.setEnabled(isEditFaces());
		
		vertexInspectorButton.setEnabled(isEditPoints());
		lineInspectorButton.setEnabled(isEditLines());
		faceInspectorButton.setEnabled(isEditFaces());
	}
	
	public Appearance getAppearance() {
		return appearance;
	}

	public void setAppearance(Appearance appearance) {
		this.appearance = appearance;
		vertexFontInspector.setNoUpdate(appearance);
		lineFontInspector.setNoUpdate(appearance);
		faceFontInspector.setNoUpdate(appearance);
		updateGUI();
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
	
	public boolean isEditPoints() {
		return points.isSelected();
	}
	public boolean isShowPoints() {
		return !vertexButton.getText().equals(VertexState.HIDE.toString());
	}
	public void setShowPoints(VertexState state) {
		vertexButton.setText(state.toString());
		updatePoints();
	}
	private void updatePoints() {
		updateShowPoints();
		updateSpheres();
		updatePointColor();
		updateSphereRadius();
		vertexFontInspector.setEditAppearance(true);
		updateEnabledStates();
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
		return vertexButton.getText().equals(VertexState.SPHERES.toString());
	}
	public void setSpheres(boolean b) {
		if(isShowPoints()) {
			if(b) {
				vertexButton.setText(VertexState.SPHERES.toString());
			} else {
				vertexButton.setText(VertexState.POINTS.toString());
			}
		}
		updatePoints();
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
		return pointColorButton.getColor();
	}
	public void setPointColor(Color c) {
		pointColorButton.setColor(c);
		updatePointColor();
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
	private void resetPointsToInherited() {
		if(appearance != null) {
			appearance.setAttribute(
					CommonAttributes.VERTEX_DRAW,
					INHERITED
			);
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.SPHERES_DRAW,
					INHERITED
			);
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.POINT_RADIUS,
					INHERITED
			);
			// 64 pixels is the maximum size for point sprites
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.POINT_SIZE,
					INHERITED
			);
			appearance.setAttribute(CommonAttributes.LINE_SHADER + "." + 
					CommonAttributes.DEPTH_FUDGE_FACTOR, INHERITED);
			appearance.setAttribute(
					CommonAttributes.POINT_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR,
					INHERITED
			);
			vertexFontInspector.setEditAppearance(false);
		}
	}
	
	private void updatePointsGUI() {
		boolean pointsDraw = false;
		try {
			pointsDraw = (Boolean) appearance.getAttribute(CommonAttributes.VERTEX_DRAW);
			if(!pointsDraw) {
				vertexButton.setText(FaceState.HIDE.toString());
			}
		} catch(ClassCastException e) {
			// points draw not set
		}
		boolean spheresDraw = true;
		try {
			 spheresDraw = (Boolean) appearance.getAttribute(
					CommonAttributes.POINT_SHADER + "." + CommonAttributes.SPHERES_DRAW);
			if(pointsDraw) {
				if(spheresDraw) {
					vertexButton.setText(VertexState.SPHERES.toString());
				} else {
					vertexButton.setText(VertexState.POINTS.toString());
				}
			}
		} catch(ClassCastException e) {
			// spheresDraw not set
		}
		try {
			Color pointColor = (Color) appearance.getAttribute(
				CommonAttributes.POINT_SHADER + "." + CommonAttributes.DIFFUSE_COLOR);
			pointColorButton.setColor(pointColor);
		} catch(ClassCastException ex) {
			// no color set in appearance
		}
		try {
			if(spheresDraw) {
				double r = (Double)	appearance.getAttribute(
										CommonAttributes.POINT_SHADER + "." +
										CommonAttributes.POINT_RADIUS);
				setPointRadius(convertTubeRadius(getLogarithmicRange(), getMaximalRadius(), r));
			} else {
				double r = (Double) appearance.getAttribute(
										CommonAttributes.POINT_SHADER + "."	+
										CommonAttributes.POINT_RADIUS);
				setPointRadius(convertTubeRadius(getLogarithmicRange(), getMaximalRadius(), r)/10);
			}
		} catch(ClassCastException ex) {
			// no color set in appearance
		}
	}
	//-----------------------------lines---------------------
	public boolean isEditLines() {
		return lines.isSelected();
	}
	
	public boolean isShowLines() {
		return !linesButton.getText().equals(LinesState.HIDE.toString());
	}
	public void setShowLines(LinesState state) {
		linesButton.setText(state.toString());
		updateLines();
	}
	private void updateLines() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.EDGE_DRAW,
					isShowLines()
			);
			boolean tubes = isTubes();
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "." +
					CommonAttributes.TUBES_DRAW,
					tubes
			);
		}
		updateTubeRadius();
		updateLineColor();
		lineFontInspector.setEditAppearance(true);
		updateEnabledStates();
		
	}
	private void resetLinesToInherited() {
		appearance.setAttribute(
				CommonAttributes.EDGE_DRAW,
				INHERITED
		);
		appearance.setAttribute(
				CommonAttributes.LINE_SHADER + "." +
				CommonAttributes.TUBES_DRAW,
				INHERITED
		);
		
		appearance.setAttribute(
				CommonAttributes.LINE_SHADER + "."	+
				CommonAttributes.LINE_WIDTH,
				INHERITED
		);
		appearance.setAttribute(
				CommonAttributes.LINE_SHADER + "."	+
				CommonAttributes.TUBE_RADIUS,
				INHERITED
		);
		
		appearance.setAttribute(
				CommonAttributes.LINE_SHADER + "." +
				CommonAttributes.DIFFUSE_COLOR ,
				INHERITED
		);
		lineFontInspector.setEditAppearance(false);
	}
	
	public boolean isTubes() {
		return linesButton.getText().equals(LinesState.TUBES.toString());
	}
	public void setTubes(boolean b) {
		if(isShowLines()) {
			if(b) {
				linesButton.setText(LinesState.TUBES.toString());
			} else {
				linesButton.setText(LinesState.LINES.toString());
			}
		}
		updateLines();
	}

	public double getTubeRadius() {
		return .01 * tubeRadiusSlider.getValue();
	}
	public void setTubeRadius(double d) {
		tubeRadiusSlider.setValue((int) (d * 100));
	}
	
	private double convertTubeRadius(double L, double M, Double r) {
		return Math.log(r * L / M)/Math.log(L);
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
		return lineColorButton.getColor();
	}
	public void setLineColor(Color c) {
		lineColorButton.setColor(c);
		updateLineColor();
	}
	private void updateLineColor() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.LINE_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR ,
					getLineColor()
			);
		}
	}
	
	private void updateLinesGUI() {
		boolean linesDraw = false;
		try {
			linesDraw = (Boolean) appearance.getAttribute(CommonAttributes.EDGE_DRAW);
			if(!linesDraw) {
				linesButton.setText(LinesState.HIDE.toString());
			}
		} catch(ClassCastException e) {
			// lines draw not set
		}
		boolean tubesDraw = false;
		try {
			tubesDraw = (Boolean) appearance.getAttribute(
					CommonAttributes.LINE_SHADER + "." + CommonAttributes.TUBES_DRAW);
			if(linesDraw) {
				if(tubesDraw) {
					linesButton.setText(LinesState.TUBES.toString());
				} else {
					linesButton.setText(LinesState.LINES.toString());
				}
			}
		} catch(ClassCastException e) {
			// tubes draw not set
		}
		try {
			Color lineColor = (Color) appearance.getAttribute(
				CommonAttributes.LINE_SHADER + "." + CommonAttributes.DIFFUSE_COLOR);
			lineColorButton.setColor(lineColor);
		} catch(ClassCastException ex) {
			// no color set in appearance
		}
		try {
			if(tubesDraw) {
				double r = (Double)	appearance.getAttribute(
										CommonAttributes.LINE_SHADER + "." +
										CommonAttributes.TUBE_RADIUS);
				setTubeRadius(convertTubeRadius(getLogarithmicRange(), getMaximalRadius(), r));
			} else {
				double r = (Double) appearance.getAttribute(
										CommonAttributes.LINE_SHADER + "."	+
										CommonAttributes.LINE_WIDTH);
				setTubeRadius(convertTubeRadius(getLogarithmicRange(), getMaximalRadius(), r)/10);
			}
		} catch(ClassCastException ex) {
			// no color set in appearance
		}
	}
	
//----------------------faces----------------------------
	public boolean isEditFaces() {
		return faces.isSelected();
	}
	public boolean isShowFaces() {
		return !facesButton.getText().equals(FaceState.HIDE.toString());
	}

	public void setShowFaces(FaceState state) {
		facesButton.setText(state.toString());
		updateFaces();
	}
	
	private void updateFaces() {
		updateShowFaces();
		updateFacesFlat();
		updateFaceColor();
		updateTransparencyEnabled();
		updateTransparency();
		faceFontInspector.setEditAppearance(true);
	}
	
	private void updateShowFaces() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.FACE_DRAW,
					isShowFaces()
			);
		}
	}
	
	public boolean isFacesFlat() {
		return facesButton.getText().equals(FaceState.FLAT.toString());
	}
	public void setFacesFlat(boolean b) {
		if(isShowFaces()) {
			if(b) {
				facesButton.setText(FaceState.FLAT.toString());
			} else {
				facesButton.setText(FaceState.SMOOTH.toString());
			}
		}
		updateFaces();
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
		updateTransparency();
	}
	private void updateTransparencyEnabled() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.TRANSPARENCY_ENABLED,
					isTransparencyEnabled()
			);
			appearance.setAttribute(Z_BUFFER_ENABLED, isTransparencyEnabled());
		} 
		updateTransparency();
		updateEnabledStates();
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
		return faceColorButton.getColor();
	}
	public void setFaceColor(Color c) {
		appearance.setAttribute(
				CommonAttributes.POLYGON_SHADER + "." +
				CommonAttributes.DIFFUSE_COLOR,c);
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
	private void resetFacesToInherited() {
		if (appearance != null) {
			appearance.setAttribute(
					CommonAttributes.FACE_DRAW,
					INHERITED
			);
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.SMOOTH_SHADING,
					INHERITED
			);
			appearance.setAttribute(
					CommonAttributes.TRANSPARENCY_ENABLED,
					INHERITED
			);
			appearance.setAttribute(Z_BUFFER_ENABLED, INHERITED);
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.TRANSPARENCY,
					INHERITED
			);
			appearance.setAttribute(
					CommonAttributes.POLYGON_SHADER + "." +
					CommonAttributes.DIFFUSE_COLOR,
					INHERITED
			);
		}
		faceFontInspector.setEditAppearance(false);
	}
	
	private void updateFacesGUI() {
		if (appearance != null) {
			boolean faceDraw = false;
			try {
				faceDraw = (Boolean) appearance.getAttribute(CommonAttributes.FACE_DRAW);
				if(!faceDraw) {
					facesButton.setText(FaceState.HIDE.toString());
				}
			} catch (ClassCastException e) {
				// faceDraw not set
			}
			try {
				boolean smooth = (Boolean) appearance.getAttribute(
											CommonAttributes.POLYGON_SHADER + "." +
											CommonAttributes.SMOOTH_SHADING);
				if(faceDraw) {
					if(smooth) {
						facesButton.setText(FaceState.SMOOTH.toString());
					} else {	
						facesButton.setText(FaceState.FLAT.toString());
					}
				}
			} catch(ClassCastException e) {
				// smooth shading not set.
			}
			try {
				boolean transparencyEnabled = (Boolean)appearance.getAttribute(
												CommonAttributes.TRANSPARENCY_ENABLED);
				transparency.setSelected(transparencyEnabled);
				double t = (Double) appearance.getAttribute(CommonAttributes.POLYGON_SHADER + "." +
								CommonAttributes.TRANSPARENCY);
				setTransparency(t);
			} catch(ClassCastException e) {
				// transparencyEnabled not set
			}
			try {
				Color faceColor = (Color) appearance.getAttribute(
						CommonAttributes.POLYGON_SHADER + "." +	CommonAttributes.DIFFUSE_COLOR);
				faceColorButton.setColor(faceColor);
			} catch(ClassCastException ex) {
				// no color set in appearance
			}
		}
	}
	
	private void updateGUI() {
		updatePointsGUI();
		updateLinesGUI();
		updateFacesGUI();
	}
	
	@Override
	public void colorChanged(ColorChangedEvent cce) {
		Object s = cce.getSource();
		if(s == lineColorButton) {
			setLineColor(cce.getColor());
		} else if(s == pointColorButton) {
			setPointColor(cce.getColor());
		} else if(s == faceColorButton) {
			setFaceColor(cce.getColor());
		}
	}
	
	private void switchTo(JComponent content) {
		removeAll();
		setLayout(new GridLayout());
		add(content);
		revalidate();
		repaint();
	}
	
	@Override
	public void updateUI() {
		super.updateUI();
		if (isShowing()) {
			SwingUtilities.updateComponentTreeUI(vertexLabelsPanel);
		}
	}
}
