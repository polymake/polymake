package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.prefs.Preferences;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.MatrixBuilder;
import de.jreality.scene.tool.Tool;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.AxisTranslationTool;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.HeadTransformationTool;
import de.jreality.tools.RotateTool;

public class ToolPluginVR extends AbstractPluginVR {
	
	private static final boolean DEFAULT_PICK_FACES = true;
	private static final boolean DEFAULT_PICK_EDGES = false;
	private static final boolean DEFAULT_PICK_VERTICES = false;

	// defaults for tool panel
	private static final boolean DEFAULT_ROTATION_ENABLED = false;
	private static final boolean DEFAULT_DRAG_ENABLED = false;
	private static final boolean DEFAULT_SNAP_TO_GRID = false;
	private static final boolean DEFAULT_INVERT_MOUSE = false;
	private static final double DEFAULT_SPEED = 4;
	private static final double DEFAULT_GRAVITY = 9.81;

	// tool tab
	private JPanel toolPanel;
	private JCheckBox rotate;
	private JCheckBox drag;
	private JCheckBox snapToGrid;
	private JCheckBox pickFaces;
	private JCheckBox pickEdges;
	private JCheckBox pickVertices;
	private JSlider gravity;
	private JSlider gain;
	private JCheckBox invertMouse;

	private Tool rotateTool = new RotateTool(), dragTool = new DraggingTool(), snapDragTool = new AxisTranslationTool();
	

	public ToolPluginVR() {
		super("tool");
		makeToolTab();
	}
	
	@Override
	public void setViewerVR(ViewerVR vvr) {
		super.setViewerVR(vvr);
	}

	@Override
	public void contentChanged() {
		setToolEnabled(rotateTool, false);
		setToolEnabled(dragTool, false);
		setToolEnabled(snapDragTool, false);
		setToolEnabled(rotateTool, rotate.isSelected());
		setToolEnabled(dragTool, drag.isSelected() && !snapToGrid.isSelected());
		setToolEnabled(snapDragTool, drag.isSelected() && snapToGrid.isSelected());
	}
	
	@Override
	public JPanel getPanel() {
		return toolPanel;
	}
	
	private void makeToolTab() {
		toolPanel = new JPanel(new BorderLayout());
		toolPanel.setBorder(new EmptyBorder(5, 5, 5, 5));
		Box toolBox = new Box(BoxLayout.Y_AXIS);
		Box toolButtonBox = new Box(BoxLayout.X_AXIS);
		toolButtonBox.setBorder(new EmptyBorder(5, 0, 5, 5));
		rotate = new JCheckBox("rotate");
		rotate.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setRotationEnabled(rotate.isSelected());
			}
		});
		toolButtonBox.add(rotate);
		drag = new JCheckBox("drag");
		drag.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setDragEnabled(drag.isSelected());
			}
		});
		toolButtonBox.add(drag);
		toolButtonBox.add(Box.createHorizontalGlue());
		snapToGrid = new JCheckBox("snap");
		snapToGrid.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setSnapToGrid(snapToGrid.isSelected());
			}
		});
		toolButtonBox.add(snapToGrid);
		toolButtonBox.add(Box.createHorizontalGlue());
		toolBox.add(toolButtonBox);

		
		Box pickButtonBox = new Box(BoxLayout.X_AXIS);
		pickButtonBox.setBorder(new EmptyBorder(5, 5, 5, 5));
		pickButtonBox.add(new JLabel("pick: "));
		pickFaces = new JCheckBox("faces");
		pickFaces.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setPickFaces(pickFaces.isSelected());
			}
		});
		pickButtonBox.add(pickFaces);
		
		pickEdges = new JCheckBox("edges");
		pickEdges.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setPickEdges(pickEdges.isSelected());
			}
		});
		pickButtonBox.add(pickEdges);
		
		pickVertices = new JCheckBox("vertices");
		pickVertices.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setPickVertices(pickVertices.isSelected());
			}
		});
		pickButtonBox.add(pickVertices);
		pickButtonBox.add(Box.createHorizontalGlue());
		
		toolBox.add(pickButtonBox);
		
		Box invertBox = new Box(BoxLayout.X_AXIS);
		invertBox.setBorder(new EmptyBorder(5, 0, 5, 5));
		invertMouse = new JCheckBox("invert mouse");
		invertMouse.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				setInvertMouse(invertMouse.isSelected());
			}
		});
		invertBox.add(invertMouse);
		invertBox.add(Box.createHorizontalGlue());
		toolBox.add(invertBox);
		
		Box gainBox = new Box(BoxLayout.X_AXIS);
		gainBox.setBorder(new EmptyBorder(10,5,10,5));
		JLabel gainLabel = new JLabel("navigation speed");
		gainBox.add(gainLabel);
		gain = new JSlider(0, 3000, (int) (100*DEFAULT_SPEED));
		gain.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setNavigationSpeed(getNavigationSpeed());
			}
		});
		gain.setPreferredSize(new Dimension(70,20));
		gain.setBorder(new EmptyBorder(0,5,0,0));
		gainBox.add(gain);
		toolBox.add(gainBox);
		
		Box gravityBox = new Box(BoxLayout.X_AXIS);
		gravityBox.setBorder(new EmptyBorder(10,5,10,5));
		JLabel gravityLabel = new JLabel("gravity");
		gravityBox.add(gravityLabel);
		gravity = new JSlider(0, 2000, (int) (100*DEFAULT_GRAVITY));
		gravity.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setGravity(getGravity());
			}
		});
		gravity.setPreferredSize(new Dimension(70,20));
		gravity.setBorder(new EmptyBorder(0,5,0,0));
		gravityBox.add(gravity);
		toolBox.add(gravityBox);
		
		toolPanel.add(BorderLayout.CENTER, toolBox);
		
		JPanel buttonPanel = new JPanel(new FlowLayout());
		JButton resetButton = new JButton("reset");
		resetButton.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e) {
				if (getViewerVR().getCurrentContent() != null) {
					MatrixBuilder.euclidean().assignTo(getViewerVR().getCurrentContent());
					getViewerVR().alignContent();
				}
			}
		});
		buttonPanel.add(resetButton);
		toolPanel.add(BorderLayout.SOUTH, buttonPanel);
	}

	protected void setSnapToGrid(boolean b) {
		snapToGrid.setSelected(b);
		setToolEnabled(dragTool, drag.isSelected() && !snapToGrid.isSelected());
		setToolEnabled(snapDragTool, drag.isSelected() && snapToGrid.isSelected());
	}

	private boolean isSnapTogrid() {
		return snapToGrid.isSelected();
	}
	
	protected void setNavigationSpeed(double navigationSpeed) {
		int speed = (int)(100*navigationSpeed);
		gain.setValue(speed);
		getViewerVR().getShipNavigationTool().setGain(navigationSpeed);
	}

	protected double getNavigationSpeed() {
		double speed = 0.01*gain.getValue();
		return speed;
	}

	protected void setGravity(double g) {
		int grav = (int)(100*g);
		gravity.setValue(grav);
		getViewerVR().getShipNavigationTool().setGravity(g);
	}

	protected double getGravity() {
		double g = 0.01*gravity.getValue();
		return g;
	}

	public void setInvertMouse(boolean b) {
		invertMouse.setSelected(b);
		HeadTransformationTool headTransformationTool = getViewerVR().getHeadTransformationTool();
		if (headTransformationTool != null) headTransformationTool.setInvert(b);
	}
	
	public boolean isInvertMouse() {
		return invertMouse.isSelected();
	}

	public void setPickVertices(boolean b) {
		getViewerVR().getContentAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE, b);
		pickVertices.setSelected(b);
	}

	public void setPickEdges(boolean b) {
		getViewerVR().getContentAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.PICKABLE, b);
		pickEdges.setSelected(b);
	}

	public void setPickFaces(boolean b) {
		getViewerVR().getContentAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.PICKABLE, b);
		pickFaces.setSelected(b);
	}
	
	public boolean isDragEnabled() {
		return drag.isSelected();
	}
	
	public void setDragEnabled(boolean b) {
		drag.setSelected(b);
		setToolEnabled(dragTool, drag.isSelected() && !snapToGrid.isSelected());
		setToolEnabled(snapDragTool, drag.isSelected() && snapToGrid.isSelected());
	}

	public boolean isRotationEnabled() {
		return rotate.isSelected();
	}
	
	public void setRotationEnabled(boolean b) {
		setToolEnabled(rotateTool, b);
		rotate.setSelected(b);
	}

	private void setToolEnabled(Tool rotateTool, boolean b) {
		if (getViewerVR().getCurrentContent() == null)
			return;
		if (!b && getViewerVR().getCurrentContent() != null
				&& getViewerVR().getCurrentContent().getTools().contains(rotateTool)) {
			getViewerVR().getCurrentContent().removeTool(rotateTool);
		} else {
			if (b && !getViewerVR().getCurrentContent().getTools().contains(rotateTool))
				getViewerVR().getCurrentContent().addTool(rotateTool);
		}
	}
	
	private boolean isPickFaces() {
		Object v = getViewerVR().getContentAppearance().getAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.PICKABLE);
		return (v instanceof Boolean) ? (Boolean) v : DEFAULT_PICK_FACES;
	}

	private boolean isPickEdges() {
		Object v = getViewerVR().getContentAppearance().getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.PICKABLE);
		return (v instanceof Boolean) ? (Boolean) v : DEFAULT_PICK_EDGES;
	}

	private boolean isPickVertices() {
		Object v = getViewerVR().getContentAppearance().getAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.PICKABLE);
		return (v instanceof Boolean) ? (Boolean) v : DEFAULT_PICK_VERTICES;
	}
	
	@Override
	public void restoreDefaults() {
		// tool panel
		setRotationEnabled(DEFAULT_ROTATION_ENABLED);
		setDragEnabled(DEFAULT_DRAG_ENABLED);
		setPickVertices(DEFAULT_PICK_VERTICES);
		setPickEdges(DEFAULT_PICK_EDGES);
		setPickFaces(DEFAULT_PICK_FACES);
		setInvertMouse(DEFAULT_INVERT_MOUSE);
		setGravity(DEFAULT_GRAVITY);
		setNavigationSpeed(DEFAULT_SPEED);
		setSnapToGrid(DEFAULT_SNAP_TO_GRID);
	}
	
	@Override
	public void storePreferences(Preferences prefs) {
		// tool panel
		prefs.putBoolean("rotationEnabled", isRotationEnabled());
		prefs.putBoolean("dragEnabled", isDragEnabled());
		prefs.putBoolean("pickVertices", isPickVertices());
		prefs.putBoolean("pickEdges", isPickEdges());
		prefs.putBoolean("pickFaces", isPickFaces());
		prefs.putBoolean("invertMouse", isInvertMouse());
		prefs.putDouble("gravity", getGravity());
		prefs.putDouble("navSpeed", getNavigationSpeed());
		prefs.putBoolean("snapToGrid", isSnapTogrid());
	}
	
	@Override
	public void restorePreferences(Preferences prefs) {
		// tool panel
		setRotationEnabled(prefs.getBoolean("rotationEnabled", DEFAULT_ROTATION_ENABLED));
		setDragEnabled(prefs.getBoolean("dragEnabled", DEFAULT_DRAG_ENABLED));
		setPickVertices(prefs.getBoolean("pickVertices", DEFAULT_PICK_VERTICES));
		setPickEdges(prefs.getBoolean("pickEdges", DEFAULT_PICK_EDGES));
		setPickFaces(prefs.getBoolean("pickFaces", DEFAULT_PICK_FACES));
		setInvertMouse(prefs.getBoolean("invertMouse", DEFAULT_INVERT_MOUSE));
		setGravity(prefs.getDouble("gravity", DEFAULT_GRAVITY));
		setNavigationSpeed(prefs.getDouble("navSpeed", DEFAULT_SPEED));
		setSnapToGrid(prefs.getBoolean("snapToGrid", DEFAULT_SNAP_TO_GRID));
	}

}
