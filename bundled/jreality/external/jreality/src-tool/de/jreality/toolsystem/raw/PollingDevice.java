package de.jreality.toolsystem.raw;

/**
 * Polling devices implement this interface and do their
 * polling in the poll() method. Please write no raw
 * devices that start extra timers.
 * 
 * @author Steffen Weissmann
 *
 */
public interface PollingDevice {

	/**
	 * Perform polling for the device in this method.
	 * @param when 
	 */
	void poll(long when);
	
}
