package de.jreality.plugin.scene;

import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;

public class VRPanel extends ShrinkPanelAggregator {

	public VRPanel() {
		shrinkPanel.setTitle("VR Controls");
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("VR Panel", "jReality Group");
		info.icon = ImageHook.getIcon("controller.png");
		return info;
	}
	
	@Override
	public String getHelpTitle() {
		return "VR Controls";
	}

}
