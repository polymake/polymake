package de.jreality.plugin.scripting;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import javax.swing.JPanel;

import de.jtem.jrworkspace.plugin.Controller;

public abstract class AbstractPythonGUI<T> implements PythonGUI<T> {

	protected long 
		id = -1;
	private Class<? extends PythonGUIPlugin<?>>
		pluginClass = null;
	private String
		variableName = "a",
		variableDisplay = "A Variable";
	private boolean
		instant = false;
	private List<PythonGUIListener>
		listeners = Collections.synchronizedList(new LinkedList<PythonGUIListener>());
	
	
	public AbstractPythonGUI(long id, Class<? extends PythonGUIPlugin<?>> pluginClass) {
		this.id = id;
		this.pluginClass = pluginClass;
	}

	@Override
	public long getId() {
		return id;
	}
	@Override
	public Class<? extends PythonGUIPlugin<?>> getPluginClass() {
		return pluginClass;
	}
	
	@Override
	public String getVariableName() {
		return variableName;
	}
	@Override
	public void setVariableName(String name) {
		this.variableName = name;
	}
	@Override
	public String getVariableDisplay() {
		return variableDisplay;
	}
	@Override
	public void setVariableDisplay(String display) {
		this.variableDisplay = display;
	}

	@Override
	public boolean isInstant() {
		return instant;
	}
	@Override
	public void setInstant(boolean instant) {
		this.instant = instant;
	}
	
	@Override
	public JPanel getFrontendGUI() {
		return null;
	}
	@Override
	public JPanel getBackendGUI() {
		return null;
	}

	
	@Override
	public void addGUIListener(PythonGUIListener l) {
		listeners.add(l);
	}
	@Override
	public void removeGUIListener(PythonGUIListener l) {
		listeners.remove(l);
	}
	protected void fireValueChanged() {
		synchronized (listeners) {
			for (PythonGUIListener l : listeners) {
				l.valueChanged(this);
			}
		}
	}
	
	@Override
	public void storeProperties(Controller c) {
		c.storeProperty(getClass(), "variableName" + getId(), getVariableName());
		c.storeProperty(getClass(), "variableDisplay" + getId(), getVariableDisplay());
		c.storeProperty(getClass(), "variableValue" + getId(), getVariableValue());
		c.storeProperty(getClass(), "instant" + getId(), isInstant());
	}
	@Override
	public void restoreProperties(Controller c) {
		String varName = c.getProperty(getClass(), "variableName" + getId(), "a");
		String varDisplay = c.getProperty(getClass(), "variableDisplay" + getId(), "A Variable");
		T varValue = c.getProperty(getClass(), "variableValue" + getId(), getVariableValue());
		boolean instant = c.getProperty(getClass(), "instant" + getId(), false);
		setVariableName(varName);
		setVariableDisplay(varDisplay);
		setVariableValue(varValue);
		setInstant(instant);
	}
	@Override
	public void deleteProperties(Controller c) {
		c.deleteProperty(getClass(), "variableName" + getId());
		c.deleteProperty(getClass(), "variableDisplay" + getId());
		c.deleteProperty(getClass(), "variableValue" + getId());
		c.deleteProperty(getClass(), "instant" + getId());
	}

}
