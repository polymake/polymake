package de.jreality.ui;

import java.awt.Color;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.PropertiesMenu;
import de.jreality.scene.Appearance;
import de.jreality.ui.ColorChooseJButton.ColorChangedEvent;
import de.jreality.ui.ColorChooseJButton.ColorChangedListener;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel.MinSizeGridBagLayout;

public class LabelsInspector extends JPanel implements ActionListener, ChangeListener, ColorChangedListener {

	private static final long 
		serialVersionUID = 1L;
	private Appearance	
		app = new Appearance();
	private String 
		shaderPrefix = "polygonShader";
	
	private JCheckBox
		visibleChecker = new JCheckBox("Visible");
	private ColorChooseJButton
		fontColorButton = new ColorChooseJButton();
	private JSliderVR
		offsetXSlider = new JSliderVR(-100, 100, 1), 
		offsetYSlider = new JSliderVR(-100, 100, 1), 
		offsetZSlider = new JSliderVR(-100, 100, 1), 
		sizeSlider = new JSliderVR(1, 100, 30),
		resolutionSlider = new JSliderVR(1, 200, 48);
	private SpinnerNumberModel
		resModel = new SpinnerNumberModel(48, 1, 100, 1);
	private JSpinner
		resolutionSpinner = new JSpinner(resModel);
	private Font
		labelFont = new Font("Sans Serif", Font.PLAIN, 48);
	
	public LabelsInspector(String shaderPrefix) {
		this.shaderPrefix = shaderPrefix;
		visibleChecker.addActionListener(this);
		sizeSlider.addChangeListener(this);
		resolutionSlider.addChangeListener(this);
		fontColorButton.addColorChangedListener(this);
		resolutionSpinner.addChangeListener(this);
		offsetXSlider.addChangeListener(this);
		offsetYSlider.addChangeListener(this);
		offsetZSlider.addChangeListener(this);
		
		setLayout(new MinSizeGridBagLayout());
		GridBagConstraints c1 = new GridBagConstraints();
		GridBagConstraints c2 = new GridBagConstraints();
		c1.weightx = 0.0;
		c1.gridwidth = GridBagConstraints.RELATIVE;
		c1.fill = GridBagConstraints.BOTH;
		c1.insets = new Insets(2,2,2,2);
		c2.weightx = 1.0;
		c2.gridwidth = GridBagConstraints.REMAINDER;
		c2.fill = GridBagConstraints.BOTH;
		c2.insets = new Insets(2,2,2,2);
		add(visibleChecker, c1);
		add(fontColorButton, c2);
		add(new JLabel("Size"), c1);
		add(sizeSlider, c2);
		add(new JLabel("Offset X"), c1);
		add(offsetXSlider, c2);
		add(new JLabel("Offset Y"), c1);
		add(offsetYSlider, c2);
		add(new JLabel("Offset Z"), c1);
		add(offsetZSlider, c2);
		add(new JLabel("Resolution"), c1);
//		add(resolutionSlider, c2);
		add(resolutionSpinner, c2);
	}
	
	public void setEditAppearance(boolean b) {
		if(b) {
			updateAppearance();
		} else {
			resetToInherited();
		}
	}
	
	private void updateGUI() {
		try {
			boolean isShowLabels = (Boolean) app.getAttribute(shaderPrefix + ".textShader.showLabels");
			visibleChecker.setSelected(isShowLabels);
		} catch(ClassCastException e){
			visibleChecker.setSelected(false);
		}
		try {
			Color labelsColor = (Color) app.getAttribute(shaderPrefix + ".textShader.diffuseColor");
			fontColorButton.setColor(labelsColor);
		} catch (ClassCastException e) {
			// no color set in appearance
		}
		try {
//			labelFont = new Font("arial", Font.PLAIN, fontSize);
			Font font = (Font) app.getAttribute(shaderPrefix + ".textShader.font");
			int resolution = font.getSize();
			double labelSize = (Double) app.getAttribute(shaderPrefix + ".textShader.scale");
			resModel.setValue(resolution);
			sizeSlider.setValue((int)(labelSize*resolution*100));
		} catch (ClassCastException e) {
			// no LabeSize specified
		}
		try {
			double[] off = (double[]) app.getAttribute(shaderPrefix + ".textShader.offset");
			offsetXSlider.setValue((int)(100.0*off[0]));
			offsetYSlider.setValue((int)(100.0*off[1]));
			offsetZSlider.setValue((int)(100.0*off[2]));
		} catch (ClassCastException e) {
			// no offset specified
		}
		updateAppearance();
	}

	private void resetToInherited() {
		app.setAttribute(shaderPrefix + ".textShader.showLabels", Appearance.INHERITED);
		app.setAttribute(shaderPrefix + ".textShader.diffuseColor", Appearance.INHERITED);
		app.setAttribute(shaderPrefix + ".textShader.scale", Appearance.INHERITED);
		app.setAttribute(shaderPrefix + ".textShader.font", Appearance.INHERITED);
		app.setAttribute(shaderPrefix + ".textShader.offset", Appearance.INHERITED);
	}

	public void actionPerformed(ActionEvent e) {
		if (visibleChecker == e.getSource()) {
			updateShowLabels();
		}
	}
	
	public void stateChanged(ChangeEvent e) {
		updateLabelResolution();
		updateLabelSize();
		updateLabelOffset();
	}
	
	public void colorChanged(ColorChangedEvent cce) {
		updateLabelColor();
	}
	
	public void updateStates() {
		visibleChecker.setSelected(isShowLabels());
	}
	
	
	public double getLabelSize() {
		return sizeSlider.getValue() / 100.0;
	}
	
	public void setLabelSize(double size) {
		sizeSlider.setValue((int)(size * 100));
		updateLabelSize();
		updateLabelResolution();
	}
	
	public void updateLabelSize() {
		if (app != null) {
			double resolution = getLabelResolution() / 100.0;
			app.setAttribute(shaderPrefix + ".textShader.scale", getLabelSize() / resolution / 100.0);
		}
	}
	
	public double[] getLabelOffset() {
		double[] result = new double[3];
		result[0] = offsetXSlider.getValue() / 100.0;
		result[1] = offsetYSlider.getValue() / 100.0;
		result[2] = offsetZSlider.getValue() / 100.0;
		return result;
	}
	
	public void setLabelOffset(double[] off) {
		offsetXSlider.setValue((int)(off[0] * 100));
		offsetYSlider.setValue((int)(off[1] * 100));
		offsetZSlider.setValue((int)(off[2] * 100));
		updateLabelOffset();
	}
	
	public void updateLabelOffset() {
		if (app != null) {
			double[] off = getLabelOffset();
			app.setAttribute(shaderPrefix + ".textShader.offset", off);
		}
	}
	
	public int getLabelResolution() {
//		return resolutionSlider.getValue();
		return resModel.getNumber().intValue();
	}
	
	public void setLabelResolution(int res) {
//		resolutionSlider.setValue(res);
		resModel.setValue(res);
		updateLabelSize();
		updateLabelResolution();
	}
	
	public void updateLabelResolution() {
		if (app != null) {
			int fontSize = (int)(getLabelResolution());
			labelFont = new Font("arial", Font.PLAIN, fontSize);
			app.setAttribute(shaderPrefix + ".textShader.font", labelFont);
		}
	}
	
	
	public void updateShowLabels() {
		if (app != null) {
			app.setAttribute(shaderPrefix + ".textShader.showLabels", isShowLabels());
		}
		updateStates();
	}
	
	public boolean isShowLabels() {
		return visibleChecker.isSelected();
	}

	public void setShowLabels(boolean show) {
		visibleChecker.setSelected(show);
		updateShowLabels();
	}
	
	public void updateLabelColor() {
		if (app != null) {
			app.setAttribute(shaderPrefix + ".textShader.diffuseColor", fontColorButton.getColor());
		}
		updateStates();
	}
	
	public Color getFontColor() {
		return fontColorButton.getColor();
	}
	
	public void setFontColor(Color color) {
		fontColorButton.setColor(color);
		updateLabelColor();
	}
	
	
	public void setNoUpdate(Appearance app) {
		this.app = app;
		updateGUI();
	}
	
	public void setAppearance(Appearance app) {
		this.app = app;
		updateAppearance();
	}


	private void updateAppearance() {
		updateShowLabels();
		updateLabelColor();
		updateLabelSize();
		updateLabelResolution();
		updateLabelOffset();
	}
	
	public void setShaderPrefix(String shaderPrefix) {
		this.shaderPrefix = shaderPrefix;
	}
	
	
	
	
	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.setPropertiesFile("LabelInspectorTest.jrw");
		v.addBasicUI();
		v.addContentUI();
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.registerPlugin(new PropertiesMenu());
		
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexCount(4);
		ifsf.setVertexCoordinates(new double[][] {{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}});
		ifsf.setFaceCount(1);
		ifsf.setFaceIndices(new int[][] {{0,1,2,3}});
		ifsf.setGenerateFaceNormals(true);
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateVertexLabels(true);
		ifsf.setGenerateEdgeLabels(true);
		ifsf.setGenerateFaceLabels(true);
		ifsf.update();
		
		v.setContent(ifsf.getGeometry());
		v.startup();
	}
	
	
}
