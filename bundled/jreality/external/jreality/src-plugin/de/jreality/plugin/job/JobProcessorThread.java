package de.jreality.plugin.job;

import static java.util.Collections.synchronizedList;

import java.awt.EventQueue;
import java.util.LinkedList;
import java.util.List;

public class JobProcessorThread extends Thread {
	
	private Job
		activeJob = null;
	private List<JobProcessorListener>
		listeners = synchronizedList(new LinkedList<JobProcessorListener>());
	
	public JobProcessorThread() {
		super("jReality Job Processor");
	}

	public void processJob(Job job) {
		if (Thread.currentThread() == this) {
			throw new RuntimeException("Cannot invoke processJob() from the job processor thread");
		}
		synchronized (this) {
			activeJob = job;
			notifyAll();
		}
	}
	
	@Override
	public synchronized void run() {
		while (true) {
			try {
				wait();
			} catch (InterruptedException e) {
				e.printStackTrace();
			};
			fireProcessStarted(activeJob);
			try {
				activeJob.execute();
			} catch (Exception e) {
				fireProcessFailed(e, activeJob);
			} catch (Throwable t) {
				System.err.println("Job failed with Error " + t);
			}
			fireProcessFinished(activeJob);
		}
	}
	
	public void addJobProcessorListener(JobProcessorListener l) {
		listeners.add(l);
	}
	public void removeJobProcessorListener(JobProcessorListener l) {
		listeners.remove(l);
	}
	
	
	protected void fireProcessStarted(final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.processStarted(job);						
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}

	protected void fireProcessFinished(final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.processFinished(job);
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}
	
	protected void fireProcessFailed(final Exception e, final Job job) {
		synchronized (listeners) {
			for (final JobProcessorListener l : listeners) {
				Runnable r = new Runnable() {
					@Override
					public void run() {
						l.processFailed(e, job);
					}
				};
				EventQueue.invokeLater(r);
			}
		}
	}

	
}
