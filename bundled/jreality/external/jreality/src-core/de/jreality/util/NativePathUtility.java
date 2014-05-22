package de.jreality.util;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class NativePathUtility {

	/**
	 * Tweaks the native path for a HotSpot virtual machine
	 */
	public static void set(String pathPrefix) {
		String vmName = Secure.getProperty("java.vm.name");
		if (!vmName.contains("HotSpot") && !vmName.contains("OpenJDK")) {
			return;
		}
		String webstartVersion = Secure.getProperty("webstart.version");
		if (webstartVersion != null) { // is webstart?
			System.out.println("I'm webstart");
			return;
		}
		String applet = Secure.getProperty("java.version.applet");
		if (applet != null) { // is applet
			System.out.println("I'm an applet");
			return;
		}
		String sysOsName = Secure.getProperty("os.name").toLowerCase();
		String sysOsArch = Secure.getProperty("os.arch").toLowerCase();
		String jniPath = pathPrefix + File.separator;
		if (sysOsName.startsWith("windows")) {
			jniPath += "win";
			if (sysOsArch.contains("64")) {
				jniPath += "64";
			} else {
				jniPath += "32";
			}
		}
		if (sysOsName.startsWith("linux")) {
			jniPath += "linux";
			if (sysOsArch.contains("64")) {
				jniPath += "64";
			} else {
				jniPath += "32";
			}
		}
		if (sysOsName.startsWith("mac os x")) {
			jniPath += "macosx";
		}
		File jniDir = new File(jniPath);
		try {
			Class<?> clazz = ClassLoader.class;
			Field field = clazz.getDeclaredField("sys_paths");
			field.setAccessible(true);
			String[] jniPaths = (String[])field.get(clazz);
			List<String> pathList = new LinkedList<String>();
			pathList.addAll(Arrays.asList(jniPaths));
			pathList.add(jniDir.getAbsolutePath());
			jniPaths = pathList.toArray(jniPaths);
			field.set(null, jniPaths);
			field.setAccessible(false);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	
}
