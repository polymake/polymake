package de.jreality.junitutils;

import java.util.HashMap;
import java.util.List;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;

/** Implements a Controller that only supports property saving and restoring. Its intended as a parameter 
 * to the methods {@link Plugin#storeStates(Controller)} and {@link Plugin#restoreStates(Controller)}.
 * 
 * @author G. Paul Peters, 18.06.2010
 *
 */
public class PropertyVault implements Controller {
	
	HashMap<Class<?>,HashMap<String,Object>> properties=new HashMap<Class<?>, HashMap<String,Object>>();

	@SuppressWarnings("unchecked")
	public <T> T deleteProperty(Class<?> context, String key) {
		if (!properties.containsKey(context)) return null;
		if (!properties.get(context).containsKey(key)) return null;
		return (T) properties.get(context).remove(key);
	}

	public <T extends Plugin> T getPlugin(Class<T> clazz) {
		return null;
	}

	public <T> List<T> getPlugins(Class<T> pClass) {
		return null;
	}

	@SuppressWarnings("unchecked")
	public <T> T getProperty(Class<?> context, String key, T defaultValue) {
		if (!properties.containsKey(context)) return defaultValue;
		if (!properties.get(context).containsKey(key)) return defaultValue;
		return (T) properties.get(context).get(key);
	}

	public boolean isActive(Plugin p) {
		return false;
	}

	public Object storeProperty(Class<?> context, String key, Object property) {
		if (!properties.containsKey(context)) 
			properties.put(context, new HashMap<String, Object>());
		properties.get(context).put(key, property);
		return null;
	}

}
