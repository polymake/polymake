package de.jreality.plugin.scripting;

import static de.jtem.jrworkspace.logging.LoggingSystem.LOGGER;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.thoughtworks.xstream.XStream;
import com.thoughtworks.xstream.converters.reflection.PureJavaReflectionProvider;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;

public class PythonIOController implements Controller {

	protected XStream 
		propertyxStream = new XStream(new PureJavaReflectionProvider());
	private Map<String, Object>
		storageMap = new HashMap<String, Object>();
	private Controller 
		pluginController = null;
	
	public PythonIOController(Controller pluginController) {
		this.pluginController = pluginController;
	}
	
	public void writeProperties(File f) {
		try {
			LOGGER.finer("try to write properties to file: " + f);
			String xml = propertyxStream.toXML(storageMap);
			FileWriter writer = new FileWriter(f);
			writer.write(xml);
			writer.flush();
			writer.close();
		} catch (IOException e) {
			LOGGER.info("writing properties failed to file \"" + f + "\":" + e.getMessage());
		}
	}
	@SuppressWarnings("unchecked")
	public void readProperties(File f) {
		try {
			FileReader r = new FileReader(f);
			storageMap = storageMap.getClass().cast(propertyxStream.fromXML(r));
			r.close();
		} catch (Exception e) {
			LOGGER.info("error while loading properties: " + e.getMessage());
		}
	}
	
	@Override
	public <T extends Plugin> T getPlugin(Class<T> clazz) {
		return pluginController.getPlugin(clazz);
	}

	@Override
	public <T> List<T> getPlugins(Class<T> pClass) {
		return null;
	}

	
	private String assembleContextKey(Class<?> context, String key) {
		String contextString = context.getName().replace('.', '_');
		String contextKey = contextString + "_" + key;
		return contextKey;
	}
	
	@Override
	public Object storeProperty(Class<?> context, String key, Object property) {
		String contextKey = assembleContextKey(context, key);
		return storageMap.put(contextKey, property);
	}

	@SuppressWarnings("unchecked")
	@Override
	public <T> T getProperty(Class<?> context, String key, T defaultValue) {
		String contextKey = assembleContextKey(context, key);
		if (storageMap.containsKey(contextKey)) {
			return (T)storageMap.get(contextKey);
		}
		return defaultValue;
	}

	@Override
	public <T> T deleteProperty(Class<?> context, String key) {
		return null;
	}

	@Override
	public boolean isActive(Plugin p) {
		return false;
	}

}
