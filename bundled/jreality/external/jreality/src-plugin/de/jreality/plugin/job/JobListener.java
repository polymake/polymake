package de.jreality.plugin.job;

public interface JobListener {

	public void jobStarted(Job job);
	public void jobProgress(Job job, double progress);
	public void jobFinished(Job job);
	public void jobFailed(Job job, Exception e);
	public void jobCancelled(Job job);
	
}
