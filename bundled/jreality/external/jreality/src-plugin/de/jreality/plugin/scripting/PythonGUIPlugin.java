package de.jreality.plugin.scripting;

import de.jtem.jrworkspace.plugin.Plugin;

public abstract class PythonGUIPlugin<T> extends Plugin {

	public abstract PythonGUI<T> getGUI(long id);
	public abstract void setGUIId(long oldId, long newId);

}
