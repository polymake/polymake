package de.tuberlin.polymake.common.jreality;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.swing.AbstractAction;
import javax.swing.DefaultListCellRenderer;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JSpinner;
import javax.swing.ListModel;
import javax.swing.ListSelectionModel;
import javax.swing.ScrollPaneConstants;
import javax.swing.SpinnerNumberModel;
import javax.swing.SwingConstants;
import javax.swing.event.ListDataListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

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

public class VisualizationPlugin 
		extends ShrinkPanelPlugin 
		implements ListSelectionListener, ActionListener {
		
	private SpinnerNumberModel 
		levelModel = new SpinnerNumberModel(3, 0, 10, 1);
	
	private JSpinner 
		levelSpinner = new JSpinner(levelModel);
	
	private JButton 
		refineButton = new JButton("Refine");
	
	private GeometryModel
		geometryModel = null; 
	
	private JList
		geometryList = null;
	
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
		geometryList = new JList(geometryModel);
		geometryList.setPreferredSize(new Dimension(150, 40));

		geometryList.setCellRenderer(new GeometryCellRenderer());
		geometryList.getSelectionModel().addListSelectionListener(geometryModel);
		geometryList.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		
		shrinkPanel.setLayout(new GridBagLayout());
		
		GridBagConstraints gbc2 = LayoutFactory.createRightConstraint();
		
		shrinkPanel.add(new JLabel("Geometries"),gbc2);
		JScrollPane listPane = new JScrollPane(geometryList, ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		listPane.setMaximumSize(new Dimension(0,10));
		shrinkPanel.add(listPane,gbc2);
		geometryList.addListSelectionListener(this);
		shrinkPanel.add(new JSeparator(SwingConstants.HORIZONTAL),gbc2);
		
		appearanceShrinkPanel.setLayout(new GridLayout(2,1));
		appearanceShrinkPanel.add(appearanceInspector);
		appearanceInspector.setMaximalRadius(5.0);
		appearanceShrinkPanel.setShrinked(false);
		JPanel clearPanel = new JPanel();
		clearPanel.add(new JLabel("Clear"));
		JButton clearAppearanceButton = new JButton(new RemoveAppearanceAction());
		clearAppearanceButton.setToolTipText("Clear colors and sizes in Appearance");
		clearPanel.add(clearAppearanceButton);
		JButton clearAttributesButton = new JButton(new RemoveGeometryAttributesAction());
		clearAttributesButton.setToolTipText("Clear individual colors/sizes in Attributes");
		clearPanel.add(clearAttributesButton);
		appearanceShrinkPanel.add(clearPanel);
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
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Visualization","polymake");
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	private class GeometryModel implements ListModel, ListSelectionListener {

		private List<SceneGraphComponent>
			geometryComponents = null;
		
		private SceneGraphComponent
			selectedComponent = null;
		
		public GeometryModel(List<SceneGraphComponent> cps) {
			geometryComponents = cps;
		}
		
		@Override
		public int getSize() {
			return geometryComponents.size();
		}

		@Override
		public Object getElementAt(int index) {
			return geometryComponents.get(index);
		}

		public List<SceneGraphComponent> getElements() {
			return geometryComponents;
		}
		
		@Override
		public void addListDataListener(ListDataListener l) {
			// TODO Auto-generated method stub
		}

		@Override
		public void removeListDataListener(ListDataListener l) {
			// TODO Auto-generated method stub
		}

		@Override
		public void valueChanged(ListSelectionEvent e) {
			SceneGraphComponent sgc = (SceneGraphComponent)geometryList.getSelectedValue();
			if(sgc == selectedComponent) {
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
			sgc.addChild(basc);
			
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
	}
	
	private class GeometryCellRenderer extends DefaultListCellRenderer {

		private static final long serialVersionUID = 1L;

		public GeometryCellRenderer() {}

		@Override
		public Component getListCellRendererComponent(
				JList list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
			if(value instanceof SceneGraphComponent) {
				return super.getListCellRendererComponent(list, ((SceneGraphComponent)value).getName(), index, isSelected, cellHasFocus);
			} else {
				return super.getListCellRendererComponent(list, value, index, isSelected,cellHasFocus);
			}
		}
	}

	@Override
	public void valueChanged(ListSelectionEvent e) {
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		Object source = e.getSource();
		if(source == refineButton) { 
			for(int i = 0; i < geometryModel.getSize(); ++i) { 
				Utils.refineGeometry(root, geometryModel.getElements(), levelModel.getNumber().intValue(),showSphere.isSelected());
			}
		}
		coordSystem.showAxes(axesBox.isSelected());		
		coordSystem.showBox(boxBox.isSelected());
		coordSystem.showLabels(labelsBox.isSelected());
		coordSystem.showGrid(gridBox.isSelected());
		updateUnitSphereComponent();
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
			SceneGraphComponent sgc = (SceneGraphComponent)geometryList.getSelectedValue();
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
			SceneGraphComponent sgc = (SceneGraphComponent)geometryList.getSelectedValue();
			Utils.clearAppearance(sgc);
			Appearance emptyAppearance = sgc.getAppearance();
			if(emptyAppearance != null) {
				appearanceInspector.setAppearance(emptyAppearance);
			}
		}

	}

}
