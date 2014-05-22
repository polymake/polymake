package de.jreality.plugin.scripting;

import javax.swing.JPanel;

import de.jtem.jrworkspace.plugin.Controller;

public interface PythonGUI<T> {
	
	public long getId();
	public Class<? extends PythonGUIPlugin<?>> getPluginClass();
	
	public String getVariableName();
	public void setVariableName(String name);
	public String getVariableDisplay();
	public void setVariableDisplay(String display);
	public T getVariableValue();
	public void setVariableValue(T val);
	public boolean isInstant();
	public void setInstant(boolean instant);
	
	public JPanel getFrontendGUI();
	public JPanel getBackendGUI();
	
	public void addGUIListener(PythonGUIListener l);
	public void removeGUIListener(PythonGUIListener l);
	
	public void storeProperties(Controller c);
	public void restoreProperties(Controller c);
	public void deleteProperties(Controller c);

}
