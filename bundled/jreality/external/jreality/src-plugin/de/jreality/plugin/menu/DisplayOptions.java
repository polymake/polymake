package de.jreality.plugin.menu;

import java.awt.Component;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

import javax.swing.KeyStroke;

import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewPreferences;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.PickShowTool;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jreality.util.GuiUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.FrontendFlavor;
import de.jtem.jrworkspace.plugin.flavor.PerspectiveFlavor;

/**
 * 
 * Plugin for quickly setting and saving camera settings and cursor/picking
 * 
 * @author brinkman
 *
 */
public class DisplayOptions extends Plugin implements FrontendFlavor {

	private ViewPreferences
		viewPreferences = null;
	private AbstractJrToggleAction pickBox;
	
	private AbstractJrToggleAction  
		fullscreenToggle;
	private boolean 
		windowedHidesPanels = false;
	private FrontendListener
		frontendListener = null;
	private boolean
		windowHasToolbar = false,
		windowHasMenuBar = false,
		windowHasStatusBar = false;
	
	private View 
		view = null;
	
	private Scene
		scene = null;
	
	private PickShowTool 
		pickShowTool = new PickShowTool();
	
	@SuppressWarnings("serial")
	public DisplayOptions() {

		pickBox = new AbstractJrToggleAction("Hide mouse, show pick") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setPick(isSelected());
			}
		};
		pickBox.setIcon(ImageHook.getIcon("mouse.png"));
		
		fullscreenToggle = new AbstractJrToggleAction("Fullscreen") {
			@Override
			public void actionPerformed(ActionEvent e) {
				setFullscreen(isSelected());
			}
		};
		fullscreenToggle.setIcon(ImageHook.getIcon("arrow_out.png"));
		fullscreenToggle.setAcceleratorKey(KeyStroke.getKeyStroke(KeyEvent.VK_F, Toolkit.getDefaultToolkit().getMenuShortcutKeyMask()));
	}
	
	public void setFullscreen(boolean fs) {
		if (fs) {
			windowHasMenuBar = frontendListener.isShowMenuBar();
			windowHasToolbar = frontendListener.isShowToolBar();
			windowHasStatusBar = frontendListener.isShowStatusBar();
			windowedHidesPanels = view.isHidePanels();
			frontendListener.setShowMenuBar(false);
			frontendListener.setShowToolBar(false);
			frontendListener.setShowStatusBar(false);
			view.setHidePanels(true);
		} else {
			frontendListener.setShowMenuBar(windowHasMenuBar);
			frontendListener.setShowToolBar(windowHasToolbar);
			frontendListener.setShowStatusBar(windowHasStatusBar);
			view.setHidePanels(windowedHidesPanels);
		}
		frontendListener.setFullscreen(fs, viewPreferences.isExclusiveFullscreen());
		frontendListener.updateFrontendUI();
		view.getViewer().getViewingComponent().requestFocusInWindow();
	}
	
	
	private void setPick(boolean showPick) {
		Component frame = view.getViewer().getViewingComponent();
		if (showPick) {
			GuiUtility.hideCursor(frame);
		} else {
			GuiUtility.showCursor(frame);
		}
		
		SceneGraphComponent root = scene.getSceneRoot();
		if (showPick && !root.getTools().contains(pickShowTool)) {
			root.addTool(pickShowTool);
		}
		if (!showPick && root.getTools().contains(pickShowTool)) {
			root.removeTool(pickShowTool);
		}
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Display Options";
		info.vendorName = "Peter Brinkmann"; 
		info.icon = ImageHook.getIcon("monitor.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		view = c.getPlugin(View.class);
		scene = c.getPlugin(Scene.class);
		viewPreferences = c.getPlugin(ViewPreferences.class);
		ViewMenuBar viewMenuBar = c.getPlugin(ViewMenuBar.class);
		viewMenuBar.addMenuItem(getClass(), 1.0, pickBox.createMenuItem(), "Viewer");
		viewMenuBar.addMenuSeparator(getClass(), 1.5, "Viewer");
		viewMenuBar.addMenuItem(getClass(), 2.0, fullscreenToggle.createMenuItem(), "Viewer");
		
		ViewToolBar viewToolBar = c.getPlugin(ViewToolBar.class);
		viewToolBar.addTool(getClass(), 2.0, pickBox.createToolboxItem());
		viewToolBar.addSeparator(getClass(), 2.1);
		setPick(pickBox.isSelected());
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		c.getPlugin(ViewMenuBar.class).removeAll(getClass());
		c.getPlugin(ViewToolBar.class).removeAll(getClass());
	}

	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		pickBox.setSelected(c.getProperty(getClass(), "showPick", false));
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "showPick", pickBox.isSelected());
	}
	
	public Class<? extends PerspectiveFlavor> getPerspective() {
		return View.class;
	}
	
	public void setFrontendListener(FrontendListener l) {
		frontendListener = l;
	}
	
	public AbstractJrToggleAction getHideMouseToggle() {
		return pickBox;
	}
	
}
