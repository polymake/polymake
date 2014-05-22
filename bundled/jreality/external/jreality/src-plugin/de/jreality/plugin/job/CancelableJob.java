package de.jreality.plugin.job;

public interface CancelableJob extends Job {

	public void requestCancel();
	
}
