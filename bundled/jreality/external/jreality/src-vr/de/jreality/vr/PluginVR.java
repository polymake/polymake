package de.jreality.vr;

import java.util.prefs.Preferences;

import javax.swing.JPanel;

public interface PluginVR {
	
	/**
	 * this method is called only once after initialization
	 * @param vvr
	 */
	void setViewerVR(ViewerVR vvr);
	ViewerVR getViewerVR();
	
	String getName();
	JPanel getPanel();

	void contentChanged();
	void storePreferences(Preferences prefs);
	void restoreDefaults();
	void restorePreferences(Preferences prefs);
	void environmentChanged();
	void terrainChanged();
	
}
