package de.jreality.plugin.experimental;

import java.awt.event.ActionEvent;
import java.beans.XMLDecoder;
import java.beans.XMLEncoder;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.List;

import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.util.Secure;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;

@Experimental
public class LoadSaveSettings extends Plugin {
	
	String filename;
	private Controller c;

	@SuppressWarnings("serial")
	AbstractJrAction loadAction = new AbstractJrAction("Load settings") {
		@Override
		public void actionPerformed(ActionEvent e) {
			try {
				restoreCurrentStates(new FileInputStream(filename));
			} catch (IOException e1) {
				e1.printStackTrace();
			}
		}
	};
	
	@SuppressWarnings("serial")
	AbstractJrAction saveAction = new AbstractJrAction("Save settings") {
		@Override
		public void actionPerformed(ActionEvent e) {
			try {
				storeCurrentStates(new FileOutputStream(filename));
			} catch (IOException e1) {
				e1.printStackTrace();
			}
		};
	};
	
	
	public LoadSaveSettings() {
		loadAction.setIcon(ImageHook.getIcon("disk.png"));
		saveAction.setIcon(ImageHook.getIcon("disk.png"));
		filename = Secure.getProperty("user.home", "")+File.separatorChar+".jrDefSettings.xml";
		System.out.println("def settings file = "+filename);
	}
	
	protected void storeCurrentStates(OutputStream os) throws IOException {
		
		final XMLEncoder enc = new XMLEncoder(os);
		
		final HashMap<String, Object> map = new HashMap<String, Object>();
		Controller faceC = new Controller() {
			public <T> T deleteProperty(Class<?> context, String key) {
				throw new UnsupportedOperationException();
			}
			public <T extends Plugin> T getPlugin(Class<T> clazz) {
				throw new UnsupportedOperationException();
			}
			public <T> List<T> getPlugins(Class<T> class1) {
				throw new UnsupportedOperationException();
			}
			public <T> T getProperty(Class<?> context, String key, T defaultValue) {
				throw new UnsupportedOperationException();
			}
			public boolean isActive(Plugin p) {
				throw new UnsupportedOperationException();
			}
			public Object storeProperty(Class<?> context, String key, Object property) {
				return map.put(key, property);
			}
		};
		for (Object p : c.getPlugins(Object.class)) {
			try {
				((Plugin)p).storeStates(faceC);
				if (!map.isEmpty()) {
					enc.writeObject(p.getClass());
					HashMap<String, Object> cm = new HashMap<String, Object>();
					cm.putAll(map);
					enc.writeObject(cm);
					map.clear();
				}
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		enc.writeObject(null); // signal file end
		enc.close();
		os.close();
	}
	
	@SuppressWarnings("unchecked")
	protected void restoreCurrentStates(InputStream is) throws IOException {
		
		final XMLDecoder dec = new XMLDecoder(is);
		
		final HashMap<Class<?>, HashMap<String, Object>> properyMap = new HashMap<Class<?>, HashMap<String,Object>>();
		while (true) {
			try {
				Class<?> pluginClazz = (Class<?>) dec.readObject();
				if (pluginClazz == null) break;
				HashMap<String, Object> propMap = (HashMap<String, Object>) dec.readObject();
				properyMap.put(pluginClazz, propMap);
			} catch (Exception e) {
				e.printStackTrace();
				break;
			}
		}
		
		Controller faceC = new Controller() {
			public <T> T deleteProperty(Class<?> context, String key) {
				throw new UnsupportedOperationException();
			}
			public <T extends Plugin> T getPlugin(Class<T> clazz) {
				throw new UnsupportedOperationException();
			}
			public <T> List<T> getPlugins(Class<T> class1) {
				throw new UnsupportedOperationException();
			}
			public <T> T getProperty(Class<?> context, String key, T defaultValue) {
				T ret = defaultValue;
				HashMap<String, Object> hashMap = properyMap.get(context);
				if (hashMap != null) {
					T foo = (T) hashMap.get(key);
					if (foo != null) ret = foo;
				}
				return ret;
			}
			public boolean isActive(Plugin p) {
				throw new UnsupportedOperationException();
			}
			public Object storeProperty(Class<?> context, String key, Object property) {
				throw new UnsupportedOperationException();
			}
		};
		
		for (Object p : c.getPlugins(Object.class)) {
			try {
				((Plugin)p).restoreStates(faceC);
			} catch (Exception c) {
				c.printStackTrace();
			}
		}
	}

	@Override
	public void install(Controller c) throws Exception {
		this.c = c;
		c.getPlugin(ViewMenuBar.class).addMenuItem(getClass(), 5, loadAction, "Settings");
		c.getPlugin(ViewMenuBar.class).addMenuItem(getClass(), 6, saveAction, "Settings");
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		c.getPlugin(ViewMenuBar.class).removeAll(getClass());
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Save and load the current settings", "jreality Group");
	}

}
