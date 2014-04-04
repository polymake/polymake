package de.jreality.plugin.job;

public interface Job {

	public String getJobName();
	public void execute() throws Exception;
	public void addJobListener(JobListener l);
	public void removeJobListener(JobListener l);
	public void removeAllJobListeners();
	
}