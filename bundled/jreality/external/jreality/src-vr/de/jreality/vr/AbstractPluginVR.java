package de.jreality.vr;

import java.util.prefs.Preferences;

import javax.swing.JPanel;

public class AbstractPluginVR implements PluginVR {

	private String name;
	private ViewerVR vvr;
	
	public AbstractPluginVR(String pluginName) {
		this.name=pluginName;
	}
	
	public void setViewerVR(ViewerVR vvr) {
		this.vvr=vvr;
	}

	public ViewerVR getViewerVR() {
		return vvr;
	}
	
	public void contentChanged() {
	}

	public void environmentChanged() {
	}

	public void terrainChanged() {
	}

	public String getName() {
		return name;
	}

	public JPanel getPanel() {
		return null;
	}

	public void storePreferences(Preferences prefs) {
	}

	public void restoreDefaults() {
	}

	public void restorePreferences(Preferences prefs) {
	}

}
