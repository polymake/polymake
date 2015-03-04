package de.jreality.plugin.job;

import static java.util.Collections.synchronizedList;

import java.util.LinkedList;
import java.util.List;

public class JobProcessorThread extends Thread {
	
	private Job
		nextJob = null;
	private List<JobProcessorListener>
		listeners = synchronizedList(new LinkedList<JobProcessorListener>());
	
	public JobProcessorThread() {
		super("jReality Job Processor");
	}

	protected void processJob(Job job) {
		synchronized (this) {
			nextJob = job;
			notifyAll();
		}
	}
	
	@Override
	public synchronized void run() {
		while (true) {
			try {
				if (nextJob == null) {
					wait();
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			Job executedJob = nextJob;
			fireProcessStarted(executedJob);
			try {
				executedJob.execute();
			} catch (Exception e) {
				fireProcessFailed(e, executedJob);
			} catch (Throwable t) {
				fireProcessFailed(new Exception("Error in job execution", t), nextJob);
			} finally {
				nextJob = null;
				fireProcessFinished(executedJob);
			}
		}
	}
	
	protected void addJobProcessorListener(JobProcessorListener l) {
		listeners.add(l);
	}
	protected void removeJobProcessorListener(JobProcessorListener l) {
		listeners.remove(l);
	}
	
	
	protected void fireProcessStarted(final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				l.processStarted(job);						
			}
		}
	}

	protected void fireProcessFinished(final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				l.processFinished(job);
			}
		}
	}
	
	protected void fireProcessFailed(final Exception e, final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				l.processFailed(e, job);
			}
		}
	}

	
}
