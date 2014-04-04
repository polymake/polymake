package de.jreality.plugin.job;

import java.awt.EventQueue;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

public abstract class AbstractJob implements Job {

	protected List<JobListener>
		listeners = Collections.synchronizedList(new LinkedList<JobListener>());
	
	@Override
	public void addJobListener(JobListener l) {
		listeners.add(l);
	}
	@Override
	public void removeJobListener(JobListener l) {
		listeners.remove(l);
	}
	@Override
	public void removeAllJobListeners() {
		listeners.clear();
	}

	@Override
	public void execute() throws Exception {
		fireJobStarted();
		try {
			executeJob();
			fireJobFinished();
		} catch (Exception e) {
			fireJobFailed(e);
		} catch (Throwable t) {
			fireJobFailed(new Exception("Error in job execution", t));
		}
	}
	
	protected abstract void executeJob() throws Exception;
	
	protected void fireJobStarted() {
		synchronized (listeners) {
			for (final JobListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.jobStarted(AbstractJob.this);						
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}
	
	protected void fireJobProgress(final double progress) {
		synchronized (listeners) {
			for (final JobListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.jobProgress(AbstractJob.this, progress);			
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}

	protected void fireJobFailed(final Exception e) {
		synchronized (listeners) {
			for (final JobListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.jobFailed(AbstractJob.this, e);			
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}
	
	protected void fireJobFinished() {
		synchronized (listeners) {
			for (final JobListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.jobFinished(AbstractJob.this);			
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}
	
	@Override
	public String toString() {
		return getJobName();
	}
	
}
