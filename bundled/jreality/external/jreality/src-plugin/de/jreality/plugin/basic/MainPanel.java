package de.jreality.plugin.basic;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.scene.ShrinkPanelAggregator;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;

public class MainPanel extends ShrinkPanelAggregator {

	public MainPanel() {
		super();
		// need to obtain panel title from plugin info,
		// otherwise inconsistant with in-scene title:
		//shrinkPanel.setTitle("Main Tools");
		shrinkPanel.setShrinked(true);
	}
	
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Main Tools", "jReality Group");
		info.icon = ImageHook.getIcon("wrench_orange.png");
		return info;
	}
	
	@Override
	public String getHelpTitle() {
		return "Main Tools";
	}
	
}
