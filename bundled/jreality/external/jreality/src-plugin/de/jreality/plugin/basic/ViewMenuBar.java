package de.jreality.plugin.basic;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenu;
import javax.swing.JMenuItem;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.actions.file.Quit;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.aggregators.MenuAggregator;
import de.jtem.jrworkspace.plugin.flavor.PerspectiveFlavor;
import de.jtem.jrworkspace.plugin.flavor.ShutdownFlavor;

public class ViewMenuBar extends MenuAggregator implements ShutdownFlavor {

	private View viewerPlugin = null;
	private ShutdownListener shutdownListener;
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Viewer Menu";
		info.vendorName = "jReality Group";
		info.icon = ImageHook.getIcon("menu.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		viewerPlugin = c.getPlugin(View.class);
		removeAll(getClass());
		
		// File menu
		JMenu fileMenu =  new JMenu("File");
		fileMenu.setMnemonic('f');
		addMenu(getClass(), 0.0, fileMenu);
	
		addMenuSeparator(getClass(), 99, "File");
		Quit quitMenuAction = new Quit("Quit");
		quitMenuAction.setActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				shutdownListener.shutdown();
			}
		});
		addMenuItem(getClass(), 100, quitMenuAction, "File");
		
		// Viewer menu
		JMenu viewerMenu = viewerPlugin.createViewerMenu();
		viewerMenu.setMnemonic('v');
		addMenu(getClass(), 1.0, viewerMenu);
		addMenuSeparator(getClass(), 0.0, "Viewer");
		
		// window menu
		JMenu windowMenu = new JMenu("Window");
		windowMenu.setMnemonic('w');
		addMenu(getClass(), 100.0, windowMenu);
		addMenuItem(getClass(), 1.0, viewerPlugin.getPanelsMenu(), "Window");
		addMenuSeparator(getClass(), 1.5, "Window");
		JMenu slotsMenu = viewerPlugin.getContaintersMenu();
		double priority = 20.0;
		addMenuSeparator(getClass(), 19.5, "Window");
		for (Component item : slotsMenu.getMenuComponents()) {
			addMenuItem(getClass(), priority++, (JMenuItem)item, "Window");
		}
	}
	

	@Override
	public void uninstall(Controller c) throws Exception {

	}
	
	public Class<? extends PerspectiveFlavor> getPerspective() {
		return View.class;
	}

	public void setShutdownListener(ShutdownListener shutdownListener) {
		this.shutdownListener = shutdownListener;
	}
	
	

}
