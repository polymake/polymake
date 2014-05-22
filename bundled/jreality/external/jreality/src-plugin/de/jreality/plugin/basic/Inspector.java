package de.jreality.plugin.basic;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridLayout;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.Navigator;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class Inspector extends ShrinkPanelPlugin {
	
	private Navigator navigator;
	private View sceneViewPlugin;
	private boolean receiveSelections = true;
	private boolean propagateSelections = true;
	
	public Inspector() {
		setInitialPosition(SHRINKER_RIGHT);
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		sceneViewPlugin = c.getPlugin(View.class);
		navigator = new Navigator(sceneViewPlugin.getViewer());
		Component nav = navigator.getComponent();
		nav.setPreferredSize(new Dimension(10, 400));
		nav.setMinimumSize(new Dimension(10, 400));

		shrinkPanel.setFillSpace(true);
		shrinkPanel.setLayout(new GridLayout());
		shrinkPanel.add(nav);
		
		setPropagateSelections(propagateSelections);
		setReceiveSelections(receiveSelections);
		super.install(c);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		shrinkPanel.removeAll();
		super.uninstall(c);
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Navigator";
		info.vendorName = "Stefan Sechelmann";
		info.icon = ImageHook.getIcon("select01.png");
		return info; 
	}

	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setReceiveSelections(c.getProperty(getClass(), "receiveSelections", isReceiveSelections()));
		setPropagateSelections(c.getProperty(getClass(), "propagateSelections", isPropagateSelections()));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "receiveSelections", isReceiveSelections());
		c.storeProperty(getClass(), "propagateSelections", isPropagateSelections());
		super.storeStates(c);
	}
	
	public boolean isReceiveSelections() {
		return navigator != null ? navigator.isReceiveSelections() : receiveSelections;
	}
	
	public void setReceiveSelections(boolean b) {
		receiveSelections = b;
		if (navigator != null) {
			navigator.setReceiveSelections(b);
		}
	}
	
	public boolean isPropagateSelections() {
		return navigator != null ? navigator.isPropagateSelections() : propagateSelections;
	}
	
	public void setPropagateSelections(boolean b) {
		propagateSelections = b;
		if (navigator != null) {
			navigator.setPropagateSelections(b);
		}
	}
	
	
	@Override
	public String getHelpDocument() {
		return "Navigator.html";
	}
	
	@Override
	public String getHelpPath() {
		return "/de/jreality/plugin/help/";
	}
	
	@Override
	public Class<?> getHelpHandle() {
		return getClass();
	}
	
	
}
