package de.jreality.plugin.content;

import static de.jreality.geometry.BoundingBoxUtility.calculateBoundingBox;
import static de.jreality.geometry.BoundingBoxUtility.removeZeroExtends;

import java.awt.Dimension;
import java.awt.GridLayout;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.MainPanel;
import de.jreality.plugin.basic.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.ui.JSliderVR;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class TerrainAlignedContent extends Content {

	public static final double
		DEFAULT_CONTENT_SIZE = 20D,
		DEFAULT_VERTICAL_OFFSET = 1D;

	SceneGraphComponent transformationComponent;
	SceneGraphComponent scalingComponent = new SceneGraphComponent("scaling");

	private double contentSize = DEFAULT_CONTENT_SIZE;
	private double verticalOffset = DEFAULT_VERTICAL_OFFSET;

	private Rectangle3D bounds;

	// private Matrix lastMatrix=new Matrix();

	private JPanel panel = new JPanel();
	final JSliderVR sizeSlider = new JSliderVR(1, 5001);
	final JSliderVR offsetSlider = new JSliderVR(-250, 5000 - 250);

	// private JPanel guiPanel;

	public void alignContent() {
		try {
			bounds = calculateBoundingBox(wrap(content));
		} catch (Exception e) {
			return;
		}
		removeZeroExtends(bounds);
		double scale = 1;
		double[] e = bounds.getExtent();
		double[] center = bounds.getCenter();
		double objectSize = Math.max(Math.max(e[0], e[1]), e[2]);
		scale = contentSize / objectSize;
		Matrix matrix = MatrixBuilder.euclidean().scale(
				scale
			)
			.getMatrix();

		matrix.assignTo(scalingComponent);

		bounds = bounds.transformByMatrix(
				bounds,
				matrix.getArray()
			);
		center = bounds.getCenter();

		FactoredMatrix factoredMatrix = new FactoredMatrix(transformationComponent.getTransformation());
		double angle = factoredMatrix.getRotationAngle();
		double[] axis = factoredMatrix.getRotationAxis();

		Matrix m = MatrixBuilder.euclidean()
			.translate(0, verticalOffset, 0)
			.rotate(angle, axis)
			.translate(-center[0], -bounds.getMinY(), -center[2]
			)
			.getMatrix();
		m.assignTo(transformationComponent);
		bounds = bounds.transformByMatrix(
				bounds,
				m.getArray()
			);
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		Scene scene = c.getPlugin(Scene.class);
		setContentSize(contentSize);
		setVerticalOffset(verticalOffset);
		transformationComponent = scene.getContentComponent();
		scalingComponent.setTransformation(new Transformation("scaling trafo"));
		transformationComponent.addChild(scalingComponent);
		SceneGraphPath newEmptyPick = scene.getContentPath().pushNew(scalingComponent);
		scene.setEmptyPickPath(newEmptyPick);
		createGUI();
		MainPanel msp = c.getPlugin(MainPanel.class);
		msp.addComponent(getClass(), panel, 0.0, "Content");
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		MainPanel msp = c.getPlugin(MainPanel.class);
		msp.removeAll(getClass());
		transformationComponent.removeChild(scalingComponent);
		super.uninstall(c);
	}

	private SceneGraphComponent wrap(SceneGraphNode node) {
		if (node instanceof SceneGraphComponent) return (SceneGraphComponent) node;
		SceneGraphComponent wrap = new SceneGraphComponent("wrapper");
		SceneGraphUtility.addChildNode(wrap, node);
		return wrap;
	}

	public void contentChanged() {
		alignContent();
	}

	@Override
	public void setContent(SceneGraphNode node) {
		boolean fire = getContentNode() != node;
		if (getContentNode() != null) {
			SceneGraphUtility.removeChildNode(scalingComponent, getContentNode());
		}
		setContentNode(node);
		if (getContentNode() != null) {
			SceneGraphUtility.addChildNode(scalingComponent, getContentNode());
			alignContent();
		}
		if (fire) {
			ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ContentChanged);
			cce.node = node;
			fireContentChanged(cce);
		}
	}

	public double getContentSize() {
		return contentSize;
	}

	public void setContentSize(double contentSize) {
		this.contentSize = contentSize;
		sizeSlider.setValue((int) (contentSize * 100));
		alignContent();
	}

	public double getVerticalOffset() {
		return verticalOffset;
	}

	public void setVerticalOffset(double verticalOffset) {
		this.verticalOffset = verticalOffset;
		offsetSlider.setValue((int) (verticalOffset * 100));
		alignContent();
	}

	private void createGUI() {
		panel.setBorder(BorderFactory.createTitledBorder("Terrain Content"));
		sizeSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setContentSize(sizeSlider.getValue() / 100.);
			}
		});
		offsetSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setVerticalOffset(offsetSlider.getValue() / 100.);
			}
		});
		panel.setLayout(new GridLayout(2, 1));
		sizeSlider.setMinimumSize(new Dimension(250, 35));
		sizeSlider.setBorder(BorderFactory.createTitledBorder(BorderFactory.createEmptyBorder(), "Size"));
		offsetSlider.setMinimumSize(new Dimension(250, 35));
		offsetSlider.setBorder(BorderFactory.createTitledBorder(BorderFactory.createEmptyBorder(), "Offset"));
		panel.add(sizeSlider);
		panel.add(offsetSlider);
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Terrain Aligned Content", "jReality Group");
		return info;
	}

	@Override
	public void restoreStates(Controller c) throws Exception {
		setContentSize(c.getProperty(getClass(), "size", DEFAULT_CONTENT_SIZE));
		setVerticalOffset(c.getProperty(getClass(), "offset", DEFAULT_VERTICAL_OFFSET));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "size", getContentSize());
		c.storeProperty(getClass(), "offset", getVerticalOffset());
		super.storeStates(c);
	}

}
