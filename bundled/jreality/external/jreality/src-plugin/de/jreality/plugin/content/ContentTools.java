package de.jreality.plugin.content;

import java.awt.event.ActionEvent;

import javax.swing.JMenu;

import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.scene.tool.Tool;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.AxisTranslationTool;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.EncompassTool;
import de.jreality.tools.RotateTool;
import de.jreality.tools.SimpleDraggingTool;
import de.jreality.tools.SimpleRotateTool;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.aggregators.ToolBarAggregator;
import de.jtem.jrworkspace.plugin.flavor.PerspectiveFlavor;

/** A plugin that adds tools to the viewer: rotate, drag, encompass, snap to grid, and pick faces, edges,
 * and vertices to the scene; and corresponding enable/disable buttons to the tool bar and to the "Content" menu.
 *
 */
public class ContentTools extends Plugin {

	private static final boolean DEFAULT_PICK_FACES = true;
	private static final boolean DEFAULT_PICK_EDGES = true;
	private static final boolean DEFAULT_PICK_VERTICES = true;

	private static final boolean DEFAULT_ROTATE_ENABLED = true;
	private static final boolean DEFAULT_DRAG_ENABLED = true;
	private static final boolean DEFAULT_ENCOMPASS_ENABLED = true;
	private static final boolean DEFAULT_SNAP_TO_GRID_ENABLED = false;

	private RotateTool rotateTool;
	private SimpleRotateTool simpleRotateTool;
	private SimpleDraggingTool simpleDraggingTool;
	private DraggingTool draggingTool;
	private AxisTranslationTool snapDragTool;
	private EncompassTool encompassTool;
	
	private AbstractJrToggleAction 
		rotate,
		drag,
		snapToGrid,
		pickFaces,
		pickEdges,
		pickVertices,
		encompass;
	
	private Scene scene = null;
	private Content content = null;
	
	@SuppressWarnings("serial")
	public ContentTools() {
		
		rotate = new AbstractJrToggleAction("Rotate") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setRotationEnabled(isSelected());
			}
		};
		rotate.setIcon(ImageHook.getIcon("arrow_rotate_clockwise.png"));
		
		drag = new AbstractJrToggleAction("Drag") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setDragEnabled(isSelected());
			}	
		};
		drag.setIcon(ImageHook.getIcon("arrow_inout.png"));
		
		snapToGrid = new AbstractJrToggleAction("Snap to Grid") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setSnapToGrid(isSelected());
			}	
		};
		snapToGrid.setIcon(ImageHook.getIcon("brick.png"));
		
		pickFaces = new AbstractJrToggleAction("Pick Faces") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setPickFaces(isSelected());
			}
		};
		pickFaces.setIcon(ImageHook.getIcon("shape_square.png"));
		
		pickEdges = new AbstractJrToggleAction("Pick Edges") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setPickEdges(isSelected());
			}
		};
		pickEdges.setIcon(ImageHook.getIcon("shape_edges.png"));
		
		pickVertices = new AbstractJrToggleAction("Pick Vertices") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setPickVertices(isSelected());
			}
		};
		pickVertices.setIcon(ImageHook.getIcon("shape_handles.png"));
		
		encompass = new AbstractJrToggleAction("Encompass") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setEncompassEnabled(isSelected());
			}
		};
		encompass.setIcon(ImageHook.getIcon("arrow_out.png"));
		
		encompassTool = new EncompassTool();
		rotateTool = new RotateTool();
		rotateTool.setFixOrigin(false);
		rotateTool.setMoveChildren(false);
		rotateTool.setUpdateCenter(false);
		rotateTool.setAnimTimeMin(250.0);
		rotateTool.setAnimTimeMax(750.0);
		
		simpleRotateTool = new SimpleRotateTool();
		simpleRotateTool.setFixOrigin(false);
		simpleRotateTool.setMoveChildren(false);
		simpleRotateTool.setUpdateCenter(false);
		
		draggingTool = new DraggingTool();
		draggingTool.setMoveChildren(false);
		
		simpleDraggingTool = new SimpleDraggingTool();
		simpleDraggingTool.setMoveChildren(false);

		snapDragTool = new AxisTranslationTool();

		setRotationEnabled(DEFAULT_ROTATE_ENABLED);
		setDragEnabled(DEFAULT_DRAG_ENABLED);
		setEncompassEnabled(DEFAULT_ENCOMPASS_ENABLED);
		setSnapToGrid(DEFAULT_SNAP_TO_GRID_ENABLED);
		setPickEdges(DEFAULT_PICK_EDGES);
		setPickFaces(DEFAULT_PICK_FACES);
		setPickVertices(DEFAULT_PICK_VERTICES);
	}

	public void install() {
		setEncompassEnabled(isEncompassEnabled());
		setRotationEnabled(isRotationEnabled());
		setDragEnabled(isDragEnabled());
		setPickEdges(isPickEdges());
		setPickFaces(isPickFaces());
		setPickVertices(isPickVertices());
	}

	public void setSnapToGrid(boolean b) {
		snapToGrid.setSelected(b);
		setToolEnabled(draggingTool, drag.isSelected() && !snapToGrid.isSelected());
		setToolEnabled(snapDragTool, drag.isSelected() && snapToGrid.isSelected());
	}
	public boolean isSnapToGrid() {
		return snapToGrid.isSelected();
	}

	public void setPickVertices(boolean b) {
		pickVertices.setSelected(b);
		setPickable(CommonAttributes.POINT_SHADER, b);
	}
	public void setPickEdges(boolean b) {
		pickEdges.setSelected(b);
		setPickable(CommonAttributes.LINE_SHADER, b);
	}
	public void setPickFaces(boolean b) {
		pickFaces.setSelected(b);
		setPickable(CommonAttributes.POLYGON_SHADER, b);
	}
	private void setPickable(String shader, boolean b) {
		if (scene != null) {
			Appearance contentAppearance = scene.getContentAppearance();
			if (contentAppearance != null) {
				contentAppearance.setAttribute(
						shader+"."+CommonAttributes.PICKABLE,
						b
				);
			}
		}
	}

	public boolean isDragEnabled() {
		return drag.isSelected();
	}
	public void setDragEnabled(boolean b) {
		drag.setSelected(b);
		boolean success = setToolEnabled(draggingTool, drag.isSelected() && !snapToGrid.isSelected());
		boolean success2 = setToolEnabled(snapDragTool, drag.isSelected() &&  snapToGrid.isSelected());
		drag.setSelected(success || success2);
	}

	public boolean isRotationEnabled() {
		return rotate.isSelected();
	}
	public void setRotationEnabled(boolean b) {
		rotate.setSelected(b);
		rotate.setSelected(setToolEnabled(rotateTool, b));
	}
	public boolean isRotateAnimationEnabled() {
		return rotateTool.isAnimationEnabled();
	}
	public void setRotateAnimationEnabled(boolean enable) {
		rotateTool.setAnimationEnabled(enable);
	}

	private boolean setToolEnabled(Tool tool, boolean b) {
		if (content != null) {
			if (b) return content.addContentTool(tool);
			else content.removeContentTool(tool);
			return false;
		}
		return b;
	}

	public boolean isPickFaces() {
		return pickFaces.isSelected();
	}
	public boolean isPickEdges() {
		return pickEdges.isSelected();
	}
	public boolean isPickVertices() {
		return pickVertices.isSelected();
	}
	
	public boolean isEncompassEnabled() {
		return encompass.isSelected();
	}
	public void setEncompassEnabled(boolean b) {
		encompass.setSelected(b);
		setToolEnabled(encompassTool, b);
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		scene = c.getPlugin(Scene.class);
		content = JRViewerUtility.getContentPlugin(c);
		encompassTool.setAutomaticClippingPlanes(scene != null ? scene.isAutomaticClippingPlanes() : true);
		install();
		ViewMenuBar viewMenuBar = c.getPlugin(ViewMenuBar.class);
		installMenu(viewMenuBar);
		ViewToolBar tb = c.getPlugin(ViewToolBar.class);
		installToolbox(tb);
		
		setToolEnabled(simpleRotateTool, true);
		setToolEnabled(simpleDraggingTool, true);
	}

	private void installToolbox(ToolBarAggregator viewToolbar) {
		viewToolbar.addTool(getClass(), 1.1, drag.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.2, rotate.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.3, snapToGrid.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addSeparator(getClass(), 1.4);//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.5, pickVertices.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.6, pickEdges.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.7, pickFaces.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addSeparator(getClass(), 1.8);//, "Tools", "Content");
		viewToolbar.addTool(getClass(), 1.9, encompass.createToolboxItem());//, "Tools", "Content");
		viewToolbar.addSeparator(getClass(), 2.0);//, "Tools", "Content");
	}

	private void installMenu(ViewMenuBar viewMenuBar) {
		viewMenuBar.addMenu(getClass(), 10.0, new JMenu("Content"));
		JMenu toolsMenu = new JMenu("Tools");
		toolsMenu.setIcon(ImageHook.getIcon("toolsblau.png"));
		JMenu pickingMenu = new JMenu("Picking");
		pickingMenu.setIcon(ImageHook.getIcon("cursor.png"));
		viewMenuBar.addMenu(getClass(), 1.0, toolsMenu, "Content");
		viewMenuBar.addMenu(getClass(), 2.0, pickingMenu, "Content");
		viewMenuBar.addMenuItem(getClass(), 1.1, drag.createMenuItem(), "Content", "Tools");
		viewMenuBar.addMenuItem(getClass(), 1.2, rotate.createMenuItem(), "Content", "Tools");
		viewMenuBar.addMenuItem(getClass(), 1.3, snapToGrid.createMenuItem(), "Content", "Tools");
		viewMenuBar.addMenuItem(getClass(), 1.5, pickVertices.createMenuItem(), "Content", "Picking");
		viewMenuBar.addMenuItem(getClass(), 1.6, pickEdges.createMenuItem(), "Content", "Picking");
		viewMenuBar.addMenuItem(getClass(), 1.7, pickFaces.createMenuItem(), "Content", "Picking");
		viewMenuBar.addMenuSeparator(getClass(), 1.8, "Content", "Tools");
		viewMenuBar.addMenuItem(getClass(), 1.9, encompass.createMenuItem(), "Content", "Tools");
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		setToolEnabled(draggingTool, false);
		setToolEnabled(rotateTool, false);
		setToolEnabled(snapDragTool, false);
		c.getPlugin(ViewMenuBar.class).removeAll(getClass());
		c.getPlugin(ViewToolBar.class).removeAll(getClass());
		super.uninstall(c);
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Tools";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("toolsblau.png");
		return info; 
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setEncompassEnabled(c.getProperty(getClass(), "encompassEnabled", isEncompassEnabled()));
		setRotationEnabled(c.getProperty(getClass(), "rotationEnabled", isRotationEnabled()));
		setDragEnabled(c.getProperty(getClass(), "dragEnabled", isDragEnabled()));
		setSnapToGrid(c.getProperty(getClass(), "snapTogrid", isSnapToGrid()));
		setPickVertices(c.getProperty(getClass(), "pickVertices", isPickVertices()));
		setPickEdges(c.getProperty(getClass(), "pickEdges", isPickEdges()));
		setPickFaces(c.getProperty(getClass(), "pickFaces", isPickFaces()));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "encompassEnabled", isEncompassEnabled());
		c.storeProperty(getClass(), "rotationEnabled", isRotationEnabled());
		c.storeProperty(getClass(), "dragEnabled", isDragEnabled());
		c.storeProperty(getClass(), "snapTogrid", isSnapToGrid());
		c.storeProperty(getClass(), "pickVertices", isPickVertices());
		c.storeProperty(getClass(), "pickEdges", isPickEdges());
		c.storeProperty(getClass(), "pickFaces", isPickFaces());
		super.storeStates(c);
	}
	
	public Class<? extends PerspectiveFlavor> getPerspective() {
		return View.class;
	}

}

