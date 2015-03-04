package de.jreality.plugin.basic;

import de.jreality.plugin.icon.ImageHook;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.StatusFlavor;

public class StatusBar extends Plugin implements StatusFlavor {

	private StatusChangedListener
		statusChangedListener = null;

	
	public void setStatus(String status) {
		if (statusChangedListener != null) {
			statusChangedListener.statusChanged(status);
		}
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Status Bar", "jReality Group");
		info.icon = ImageHook.getIcon("textfield.png");
		return info;
	}

	@Override
	public void setStatusListener(StatusChangedListener scl) {
		statusChangedListener = scl;
	}

}
