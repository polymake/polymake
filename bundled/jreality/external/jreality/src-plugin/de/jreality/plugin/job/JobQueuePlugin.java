package de.jreality.plugin.job;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;

public class JobQueuePlugin extends Plugin implements JobProcessorListener {

	private Logger
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
		processorThread.processJob(job);
		runningJob = job;
	}

	protected void finalizeJob(Job job) {
		assert runningJob == job;
		synchronized (Q) {
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
		processorThread.addJobProcessorListener(this);
	}
	
	@Override
	public void processStarted(Job job) {
	}
	@Override
	public void processFinished(Job job) {
		finalizeJob(job);
	}
	@Override
	public void processFailed(Exception e, Job job) {
		finalizeJob(job);
	}
	
	public void addJobListener(JobListener l) {
		jobListeners.add(l);
	}
	public void removeJobListener(JobListener l) {
		jobListeners.remove(l);
	}
	
}
