package de.tuberlin.polymake.common.jreality;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.swing.AbstractAction;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JSpinner;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.ScrollPaneConstants;
import javax.swing.SpinnerNumberModel;
import javax.swing.SwingConstants;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;

import de.jreality.geometry.CoordinateSystemFactory;
import de.jreality.geometry.SphereUtility;
import de.jreality.plugin.basic.View;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.LayoutFactory;
import de.jreality.ui.SimpleAppearanceInspector;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

public class VisualizationPlugin extends ShrinkPanelPlugin implements ActionListener {
		
	private SpinnerNumberModel 
		levelModel = new SpinnerNumberModel(3, 0, 10, 1);
	
	private JSpinner 
		levelSpinner = new JSpinner(levelModel);
	
	private JButton 
	    refineButton = new JButton("Refine"),
	    rescanButton = new JButton("Rescan"),
	    splitButton = new JButton("Split");
	
	private GeometryModel
		geometryModel = null; 
	
	private JTable
		geometryTable = null;
	
	private ShrinkPanel
		fanShrinkPanel = new ShrinkPanel("Fan refinement"),
		appearanceShrinkPanel = new ShrinkPanel("Appearance"),
		arrowsShrinkPanel = new ShrinkPanel("Arrows"),
		auxShrinkPanel = new ShrinkPanel("Auxiliary"); 
	
	private JCheckBox 
		showSphere = new JCheckBox("Show sphere"),
		axesBox = new JCheckBox("Show axes"),
		boxBox = new JCheckBox("Show bounding box"),
		labelsBox = new JCheckBox("Show labels"),
		gridBox = new JCheckBox("Show grid"),
		unitSphereBox = new JCheckBox("Show unit sphere");
	
	private CoordinateSystemFactory 
		coordSystem = null;
	
	private SceneGraphComponent
		root = null,
		unitSphereComponent = new SceneGraphComponent("Unit Sphere"),
		auxiliaryGeometryComponent = new SceneGraphComponent("Auxiliary Geometry");
	
	private BallAndStickPanel
		basPanel = new BallAndStickPanel();
	
	private SimpleAppearanceInspector
		appearanceInspector = new SimpleAppearanceInspector();
	
	private Map<SceneGraphComponent, SceneGraphComponent>
		componentArrowsMap = new HashMap<SceneGraphComponent, SceneGraphComponent>();

	public VisualizationPlugin(SceneGraphComponent root) {
		this.root = root;
		geometryModel = new GeometryModel(new LinkedList<SceneGraphComponent>(root.getChildComponents()));
		geometryTable = new JTable(geometryModel);

		geometryTable.setDefaultRenderer(SceneGraphComponent.class, new GeometryCellRenderer());
		geometryTable.getColumnModel().getColumn(0).setMaxWidth(20);
		geometryTable.getSelectionModel().addListSelectionListener(geometryModel);
		geometryTable.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		
		shrinkPanel.setLayout(new GridBagLayout());
		
		GridBagConstraints gbc2 = LayoutFactory.createRightConstraint();
		shrinkPanel.setPreferredSize(new Dimension(150,300));
		shrinkPanel.add(new JLabel("Geometries"),gbc2);
		JScrollPane listPane = new JScrollPane(geometryTable, ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		listPane.setPreferredSize(new Dimension(150,100));
		shrinkPanel.add(listPane,gbc2);
		gbc2.gridwidth = 1;
		shrinkPanel.add(splitButton, gbc2);
		gbc2.gridwidth = GridBagConstraints.REMAINDER;
		shrinkPanel.add(rescanButton,gbc2);
		
//		appearanceShrinkPanel.setLayout(new GridLayout(3,1));
		appearanceShrinkPanel.setPreferredSize(new Dimension(150,150));
		appearanceShrinkPanel.add(appearanceInspector, gbc2);
		appearanceInspector.setPreferredSize(new Dimension(100,100));
		appearanceInspector.setMaximalRadius(10.0);
		appearanceShrinkPanel.setShrinked(false);
		JPanel clearPanel = new JPanel();
		clearPanel.setPreferredSize(new Dimension(100,32));
		clearPanel.setLayout(new GridBagLayout());
		gbc2.gridwidth = 1;
		clearPanel.add(new JLabel("Clear"),gbc2);
		JButton clearAppearanceButton = new JButton(new RemoveAppearanceAction());
		clearAppearanceButton.setToolTipText("Clear colors and sizes in Appearance");
		clearPanel.add(clearAppearanceButton,gbc2);
		JButton clearAttributesButton = new JButton(new RemoveGeometryAttributesAction());
		clearAttributesButton.setToolTipText("Clear individual colors/sizes in Attributes");
		gbc2.gridwidth = GridBagConstraints.REMAINDER;
		clearPanel.add(clearAttributesButton,gbc2);
		appearanceShrinkPanel.add(new JSeparator(SwingConstants.HORIZONTAL),gbc2);
		appearanceShrinkPanel.add(clearPanel, gbc2);
		shrinkPanel.add(appearanceShrinkPanel, gbc2);
		
		
		arrowsShrinkPanel.add(basPanel);
		arrowsShrinkPanel.setShrinked(true);
		shrinkPanel.add(arrowsShrinkPanel,gbc2);
		
		axesBox.addActionListener(this);
		auxShrinkPanel.add(axesBox,gbc2);
		boxBox.addActionListener(this);
		auxShrinkPanel.add(boxBox,gbc2);
		labelsBox.addActionListener(this);
		auxShrinkPanel.add(labelsBox,gbc2);
		gridBox.addActionListener(this);
		auxShrinkPanel.add(gridBox,gbc2);
		unitSphereBox.addActionListener(this);
		auxShrinkPanel.add(unitSphereBox,gbc2);
		auxShrinkPanel.setShrinked(true);
		
		shrinkPanel.add(auxShrinkPanel,gbc2);
		shrinkPanel.setShrinked(true);
		
		fanShrinkPanel.setLayout(new GridBagLayout());
		refineButton.addActionListener(this);
		
		fanShrinkPanel.add(levelSpinner,gbc2);
		fanShrinkPanel.add(showSphere,gbc2);
		fanShrinkPanel.add(refineButton,gbc2);
		fanShrinkPanel.setShrinked(true);
		shrinkPanel.add(fanShrinkPanel,gbc2);
		shrinkPanel.revalidate();
		
		initUnitSphereComponent();
		auxiliaryGeometryComponent.addChild(unitSphereComponent);
		root.addChild(auxiliaryGeometryComponent);
		
		coordSystem = new CoordinateSystemFactory(root);
		coordSystem.showBoxArrows(true);
		coordSystem.beautify(true);
		coordSystem.setColor(Color.DARK_GRAY);
		Appearance coordAppearance = new Appearance();
		coordAppearance.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		coordAppearance.setAttribute(CommonAttributes.EDGE_DRAW, true);
		coordAppearance.setAttribute(CommonAttributes.FACE_DRAW, true);
		coordAppearance.setAttribute(CommonAttributes.POLYGON_SHADER + "." + CommonAttributes.SMOOTH_SHADING, true);
		
		coordSystem.getCoordinateSystem().setAppearance(coordAppearance);

		rescanButton.addActionListener(this);
		splitButton.addActionListener(this);
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Visualization","polymake");
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	private class GeometryModel extends DefaultTableModel implements ListSelectionListener{

		private static final long serialVersionUID = -2823363787108362699L;

		private String[] 
				columnNames = {"", "Name"};
		
		private List<SceneGraphComponent>
			geometryComponents = null;
		
		private SceneGraphComponent
			selectedComponent = null;
		
		public GeometryModel(List<SceneGraphComponent> cps) {
			geometryComponents = cps;
		}
		
		public List<SceneGraphComponent> getElements() {
			return geometryComponents;
		}
		
		@Override
		public void valueChanged(ListSelectionEvent e) {
		        int i = geometryTable.getSelectedRow();

			if(e.getValueIsAdjusting()) { return; }
			SceneGraphComponent sgc = geometryComponents.get(i);
			if(sgc == null || sgc == selectedComponent) {
				return;
			} else {
				selectedComponent = sgc;
			}
			SceneGraphComponent arrowsComponent = componentArrowsMap.get(selectedComponent);
			boolean showArrows = false;
			if(arrowsComponent != null) {
				showArrows = arrowsComponent.isVisible();
				selectedComponent.removeChild(arrowsComponent);
			}
			updateBASPanel(showArrows);
			SceneGraphComponent basc = basPanel.getSceneGraphComponent();
			basc.setVisible(showArrows);
			componentArrowsMap.put(selectedComponent,basc);
			if(sgc != basc) {
				sgc.addChild(basc);
			}
			
			Appearance newApp = selectedComponent.getAppearance();
			if(newApp == null) {
				newApp = new Appearance();
				selectedComponent.setAppearance(newApp);
			}
			
			appearanceInspector.setAppearance(newApp);
			
		}


		private void updateBASPanel(boolean showArrows) {
			de.jreality.scene.Geometry geometry = selectedComponent.getGeometry();
			if(geometry instanceof IndexedLineSet) {
				IndexedLineSet lineset = (IndexedLineSet)geometry;
				basPanel.setLineSet(lineset);
				basPanel.setShowArrows(showArrows);
			}
		}

		@Override
		public int getRowCount() {
			if(geometryComponents == null) {
				return 0;
			}
			return geometryComponents.size();
		}

		@Override
		public int getColumnCount() {
			return 2;
		}

		@Override
		public String getColumnName(int columnIndex) {
			return columnNames[columnIndex];
		}

		@Override
		public Class<?> getColumnClass(int columnIndex) {
			switch (columnIndex) {
			case 0:
				return Boolean.class;
			case 1:
				return SceneGraphComponent.class;
			default:
				return String.class;
			}
		}

		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			if(columnIndex == 0) {
				return true;
			} else {
				return false;
			}
		}

		@Override
		public Object getValueAt(int rowIndex, int columnIndex) {
			if(columnIndex == 1) {
				return geometryComponents.get(rowIndex);
			} else {
				return geometryComponents.get(rowIndex).isVisible();
			}
		}

		@Override
		public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
			if(columnIndex == 0) {
				geometryComponents.get(rowIndex).setVisible((Boolean)aValue);
			}
		}

		public SceneGraphComponent getSelectedComponent() {
			return selectedComponent;
		}
	    
	        public void clear() {
		    geometryComponents.clear();
		}
	   
	    public void addAll(Collection<SceneGraphComponent> c) {
		geometryComponents.addAll(c);
	    }

	}
	
	private class GeometryCellRenderer extends DefaultTableCellRenderer {

		private static final long serialVersionUID = 1L;

		public GeometryCellRenderer() {}

		@Override
		public Component getTableCellRendererComponent(JTable table,
				Object value, boolean isSelected, boolean hasFocus, int row,
				int column) {
			if(value instanceof SceneGraphComponent) {
				return super.getTableCellRendererComponent(table, ((SceneGraphComponent)value).getName(), isSelected, hasFocus, row, column);
			} else {
				return super.getTableCellRendererComponent(table, value, isSelected, hasFocus,
					row, column);
			}
		}
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		Object source = e.getSource();
		if(source == refineButton) { 
			for(int i = 0; i < geometryModel.getRowCount(); ++i) { 
				Utils.refineGeometry(root, geometryModel.getElements(), levelModel.getNumber().intValue(),showSphere.isSelected());
			}
		} else if(source == rescanButton) {
		    rescanPolymakeRoot();
		} else if(source == splitButton) {
			SceneGraphComponent sgc = geometryModel.getSelectedComponent();
			if(sgc != null) {
				Utils.splitIndexedFaceSet(sgc);
			}
		}
		coordSystem.showAxes(axesBox.isSelected());		
		coordSystem.showBox(boxBox.isSelected());
		coordSystem.showLabels(labelsBox.isSelected());
		coordSystem.showGrid(gridBox.isSelected());
		updateUnitSphereComponent();
	}
	
	private void rescanPolymakeRoot() {
		List<SceneGraphComponent> geometries = collectGeometries(root);
		updateGeometryTable(geometries);
	}

	private void updateGeometryTable(List<SceneGraphComponent> geometries) {
		geometryModel.clear();
		geometryModel.addAll(geometries);
		geometryModel.fireTableDataChanged();
	}

	private List<SceneGraphComponent> collectGeometries(SceneGraphComponent r) {
		List<SceneGraphComponent> children = r.getChildComponents();
		List<SceneGraphComponent> geomNodes = filterGeometries(children);
		for(SceneGraphComponent child : children) {
			if(child.getName().equalsIgnoreCase("BAS")) {
				// do nothing.
				// geomNodes.add(child);
			} else if(child != auxiliaryGeometryComponent && child != coordSystem.getCoordinateSystem()) {
				geomNodes.addAll(collectGeometries(child));
			}
		}
		return geomNodes;
	}

	private List<SceneGraphComponent> filterGeometries(List<SceneGraphComponent> cps) {
		List<SceneGraphComponent> geometries = new LinkedList<SceneGraphComponent>();
		for(SceneGraphComponent sgc : cps) {
			if(sgc.getGeometry() != null) {
				geometries.add(sgc);
			}
		}
		return geometries;
	}

	private void initUnitSphereComponent() {
		unitSphereComponent.setGeometry(SphereUtility.tessellatedIcosahedronSphere(2));
		unitSphereComponent.setVisible(false);
		Appearance app = new Appearance();
		app.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		app.setAttribute(CommonAttributes.EDGE_DRAW, true);
		app.setAttribute(CommonAttributes.FACE_DRAW, false);
		app.setAttribute(CommonAttributes.POLYGON_SHADER + "." + CommonAttributes.SMOOTH_SHADING, true);
		app.setAttribute(CommonAttributes.LINE_SHADER + "." + CommonAttributes.TUBES_DRAW, true);
		app.setAttribute(CommonAttributes.LINE_SHADER + "." + CommonAttributes.TUBE_RADIUS, 0.02);
		unitSphereComponent.setAppearance(app);
	}
	
	private void updateUnitSphereComponent() {
		unitSphereComponent.setVisible(unitSphereBox.isSelected());
	}

	public void setSphericalRefineEnabled(boolean b) {
		fanShrinkPanel.setVisible(b);
	}
	

	private class RemoveGeometryAttributesAction extends AbstractAction {

		private static final long serialVersionUID = 1L;

		public RemoveGeometryAttributesAction() {
			putValue(NAME,"Attr.");
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			SceneGraphComponent sgc = geometryModel.getSelectedComponent();
			Utils.clearAttributes(sgc);
		}
	}
	
	private class RemoveAppearanceAction extends AbstractAction {

		private static final long serialVersionUID = 1L;

		public RemoveAppearanceAction() {
			putValue(NAME,"App.");
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			SceneGraphComponent sgc = geometryModel.getSelectedComponent();
			Utils.clearAppearance(sgc);
			Appearance emptyAppearance = sgc.getAppearance();
			if(emptyAppearance != null) {
				appearanceInspector.setAppearance(emptyAppearance);
			}
		}

	}

}
