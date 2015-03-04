package de.jreality.plugin.job;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;

public class JobQueuePlugin extends Plugin {

	private static Logger
		log = Logger.getLogger(JobQueuePlugin.class.getName());
	protected List<Job>
		Q = Collections.synchronizedList(new LinkedList<Job>());
	private JobProcessorThread
		processorThread = new JobProcessorThread();
	protected Job
		runningJob = null;
	public List<JobListener>
		jobListeners = Collections.synchronizedList(new LinkedList<JobListener>());

	public void queueJob(Job job) {
		synchronized (Q) {
			log.fine("enqueuing job " + job);
			Q.add(job);			
		}
		processQueue();
	}
	public void cancelJob(Job job) {
		if (job instanceof CancelableJob) {
			CancelableJob cancelableJob = (CancelableJob)job;
			cancelableJob.requestCancel();
		}
		if (job != runningJob) {
			Q.remove(job);
		} else {
			log.warning("cannot cancel job " + job.getJobName() + ": not cancelable");
		}
	}
	public BlockerJob block(String name) {
		if (Thread.currentThread() == processorThread) {
			log.fine("created non-blocking job");
			return new BlockerJob.NonBlockingJob(name);
		}
		BlockerJob job = new BlockerJob(name);
		log.fine("created blocking job " + job);
		queueJob(job);
		return job.block();
	}
	
	protected void processQueue() {
		synchronized (Q) {
			if (Q.isEmpty() || runningJob != null) {
				return;
			}
			Job job = Q.get(0);
			processJob(job);
		}
	}
	
	protected void processJob(Job job) {
		synchronized (jobListeners) {
			for (JobListener l : jobListeners) {
				job.addJobListener(l);
			}			
		}
		log.fine("processing job " + job);
		processorThread.processJob(job);
		runningJob = job;
	}

	protected void finalizeJob(Job job) {
		assert runningJob == job;
		synchronized (Q) {
			log.fine("finalizing job " + job);
			Q.remove(runningJob);
			synchronized (jobListeners) {
				for (JobListener l : jobListeners) {
					job.removeJobListener(l);
				}	
			}
			runningJob = null;
		}
		processQueue();				
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		processorThread.start();
		ProcessorListener proc = new ProcessorListener();
		processorThread.addJobProcessorListener(proc);
	}
	
	private class ProcessorListener implements JobProcessorListener {
	
		@Override
		public void processStarted(Job job) {
			Logger log = Logger.getLogger(job.getClass().getName());
			log.fine("job " + job + " started");
		}
		@Override
		public void processFinished(Job job) {
			Logger log = Logger.getLogger(job.getClass().getName());
			log.fine("job " + job + " finished");
			finalizeJob(job);
		}
		@Override
		public void processFailed(Exception e, Job job) {
			Logger log = Logger.getLogger(job.getClass().getName());
			log.log(Level.SEVERE, "Error in job execution: " + e, e);
		}
	
	}
	
	public void addJobListener(JobListener l) {
		jobListeners.add(l);
	}
	public void removeJobListener(JobListener l) {
		jobListeners.remove(l);
	}
	
}
