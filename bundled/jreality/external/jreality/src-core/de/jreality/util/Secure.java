package de.jreality.util;

import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.logging.Level;

/**
 * A class for managing access to system resources, especially for the case 
 * when the application is running in a secure environment and must be 
 * careful how it accesses system properties.
 * @author gunn
 *
 */
public class Secure {
	
    public static String getProperty(final String name, final String def) {
    	String result = null;
        try{
            result = AccessController.doPrivileged(new PrivilegedAction<String>() {           
                public String run() {
                    return System.getProperty(name, def);
                }
            });
        } catch (Throwable t) {
            result = def;
        }
        return result;
    }

    public static String getProperty(final String name) {
        return  doPrivileged(new PrivilegedAction<String>() {
    		public String run() {
    			return System.getProperty(name);
    		}
    	});
    }

	public static String setProperty(final String name, final String value) {
    	return doPrivileged(new PrivilegedAction<String>() {
    		public String run() {
    			return System.setProperty(name, value);
    		}
    	});
	}
	
	public static <T> T doPrivileged(PrivilegedAction<T> action) {
		T ret=null;
		try {
			ret = AccessController.doPrivileged(action);
		} catch (Throwable e) {
			LoggingSystem.getLogger(Secure.class).log(Level.INFO, "security problem:", e);
		}
		return ret;
	}

}
