package de.jreality.plugin.job;

import java.awt.EventQueue;

public abstract class AbstractCancelableJob extends AbstractJob implements CancelableJob {

	protected boolean
		cancelRequested = false;
	
	@Override
	public void requestCancel() {
		this.cancelRequested = true;
	}
	public boolean isCancelRequested() {
		return cancelRequested;
	}
	
	protected void fireJobCancelled() {
		synchronized (listeners) {
			for (final JobListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.jobCancelled(AbstractCancelableJob.this);
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}

}
