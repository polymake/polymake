package de.jreality.tools;

public interface AnimatorTask {
	/**
	 * 
	 * @param time the time (long)
	 * @param dt milli seconds count since last call (int)
	 * @return true if the task should be scheduled again, false otherwise
	 */
  boolean run(double time, double dt);
}
