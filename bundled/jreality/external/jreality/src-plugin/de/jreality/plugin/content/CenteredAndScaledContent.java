package de.jreality.plugin.content;

import java.awt.GridLayout;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.MainPanel;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.ui.JSliderVR;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class CenteredAndScaledContent extends Content implements ChangeListener, TransformationListener {

	private double 
		size = 5.0;
	
	// things to remember for changing size
	private double[] 
	    center = new double[3];
	private double 
		objectSize = 1.0;
	private Matrix 
		lastMatrix = new Matrix();
	
	private JPanel
		panel = new JPanel();
	private JSliderVR 
		sizeSlider = new JSliderVR(1, 5001);

	public CenteredAndScaledContent() {
		panel.setBorder(BorderFactory.createTitledBorder("Scaled Content"));
		panel.setLayout(new GridLayout());
		panel.add(sizeSlider);
		sizeSlider.addChangeListener(this);
	}
	
	
	public synchronized void stateChanged(ChangeEvent e) {
		size = sizeSlider.getValue() / 100.0;
		updateMatrix();
	}
	
	public void transformationMatrixChanged(TransformationEvent ev) {
		FactoredMatrix fm = new FactoredMatrix(ev.getTransformation());
		double[] stretch = fm.getStretch();
		sizeSlider.removeChangeListener(this);
		sizeSlider.setValue((int)(stretch[0] * 100 * objectSize));
		sizeSlider.addChangeListener(this);
	}
	
	
	@Override
	public void setContent(SceneGraphNode node) {
		SceneGraphComponent root = getContentRoot();
		boolean fire = getContentNode() != node;
		if (getContentNode() != null) {
			SceneGraphUtility.removeChildNode(root, getContentNode());
		}
		setContentNode(node);
		if (getContentNode() != null) {
			root.setGeometry(null);
			SceneGraphUtility.addChildNode(root, getContentNode());
		}
		if (getContentNode() != null) {
			SceneGraphComponent cmp = null;
			if (node instanceof SceneGraphComponent) {
				cmp = (SceneGraphComponent) node;
			} else {
				cmp = new SceneGraphComponent("Wrapper");
				SceneGraphUtility.addChildNode(cmp, node);
			}
			Rectangle3D bds = BoundingBoxUtility.calculateBoundingBox(cmp);
			double[] ext = bds.getExtent();
			objectSize = Math.max(Math.max(ext[0], ext[1]), ext[2]);
			center = bds.getCenter();
		} else {
			center[0] = 0.0; 
			center[1] = 0.0; 
			center[2] = 0.0;
			objectSize = 1.0;
		}
		updateMatrix();
		if (fire) {
			ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ContentChanged);
			cce.node = node;
			fireContentChanged(cce);
		}
	}
	
	private void updateMatrix() {
		SceneGraphComponent root = getContentRoot();
		if (root == null) {
			return;
		}
		MatrixBuilder mb = MatrixBuilder.euclidean();
		mb.scale(size/objectSize);
		mb.translate(-center[0], -center[1], -center[2]);
		Matrix newMatrix = mb.getMatrix();
		/*
		Matrix toolModification = new Matrix(lastMatrix);
		toolModification.invert();
		toolModification.multiplyOnRight(new Matrix(contentCmp.getTransformation()));
		newMatrix.multiplyOnRight(toolModification);
		 */
		lastMatrix.assignFrom(newMatrix);
		newMatrix.assignTo(root);
	}

	public double getSize() {
		return size;
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Centered and Scaled Content", "jReality Group");
		return info;
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		MainPanel msp = c.getPlugin(MainPanel.class);
		msp.addComponent(getClass(), panel, 0.0, "Content");
		getContentRoot().getTransformation().addTransformationListener(this);
	}
	
	
	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "scale", size);
	}

	
	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		size = c.getProperty(getClass(), "scale", size);
	}
	
	
	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		MainPanel msp = c.getPlugin(MainPanel.class);
		msp.removeAll(getClass());
		SceneGraphComponent root = getContentRoot();
		if (root == null || getContentNode() == null) {
			return;
		}
		SceneGraphUtility.removeChildNode(root, getContentNode());
	}
	
}
