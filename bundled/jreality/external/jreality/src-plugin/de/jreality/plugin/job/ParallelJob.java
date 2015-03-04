package de.jreality.plugin.job;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class ParallelJob extends AbstractJob implements JobListener {

	private ExecutorService
		executorService = Executors.newCachedThreadPool();
	
	private Collection<AbstractJob> jobs = null;
	
	private Map<Job, Double> jobProgressMap = Collections.synchronizedMap(new LinkedHashMap<Job,Double>());
	
	private long timeout = 3600000;
	
	private String jobName = "Parallel Job";

	public ParallelJob(Collection<AbstractJob> jobs, String name) {
		this.jobs  = jobs;
		this.jobName = name;
	}
	
	public ParallelJob(Collection<AbstractJob> jobs, String name, long timeout) {
		this.jobs  = jobs;
		this.jobName = name;
		this.timeout = timeout;
	}
	
	@Override
	protected void executeJob() throws Exception {
		for(final Job j : jobs) {
			j.addJobListener(this);
			Runnable r = new Runnable() {
				@Override
				public void run() {
					try {
						j.execute();
					} catch (Exception e) {
						fireJobFailed(e);
					} 
				}
			};
			executorService.execute(r);
		}
		executorService.shutdown();
		executorService.awaitTermination(timeout, TimeUnit.MILLISECONDS);
	}

	@Override
	public void jobStarted(Job job) {
	}

	@Override
	public void jobProgress(Job job, double progress) {
		jobProgressMap.put(job, progress);
		fireJobProgress(accumulateProgress());
	}

	@Override
	public void jobFinished(Job job) {
		jobProgress(job, 1.0);
	}

	@Override
	public void jobFailed(Job job, Exception e) {
		fireJobFailed(e);
	}

	@Override
	public void jobCancelled(Job job) {
	}

	private double accumulateProgress() {
		synchronized (jobProgressMap) {
			double progress = 0.0;
			for(Double d : jobProgressMap.values()) {
				progress += d;
			}
			return progress/jobs.size();
		}
	}

	@Override
	public String getJobName() {
		return jobName;
	}
	
}
