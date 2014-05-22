package de.jreality.plugin.job;

public interface JobProcessorListener {

	public void processStarted(Job job);
	public void processFinished(Job job);
	public void processFailed(Exception e, Job job);
	
}
