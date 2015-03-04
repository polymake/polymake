package de.jreality.plugin.job;

import java.util.logging.Logger;

public class BlockerJob extends AbstractJob {

	private String 
		jobName = "Blocker Job";
	private Object 
		blockMutex = new Object(),
		jobMutex = new Object();
	private volatile boolean
		running = false;
		
	private Logger
		log = Logger.getLogger(BlockerJob.class.getName());
	
	public BlockerJob(String name) {
		this.jobName = name;
	}

	@Override
	public String getJobName() {
		return jobName;
	}

	@Override
	protected void executeJob() throws Exception {
		synchronized (blockMutex) {
			running = true;
			blockMutex.notify();
		}
		synchronized (jobMutex) {
			jobMutex.wait();
		}
	}
	
	public BlockerJob block() {
		synchronized (blockMutex) {
			try {
				if (!running) {
					blockMutex.wait();
				}
			} catch (InterruptedException e) {
				log.warning("job blocker was interrupted: " + e);
			}
		}
		return this;
	}
	
	public BlockerJob unblock() {
		synchronized (jobMutex) {
			jobMutex.notify();
		}
		return this;
	}
	
	protected static class NonBlockingJob extends BlockerJob {

		public NonBlockingJob(String name) {
			super(name);
		}
		@Override
		public void execute() throws Exception {
		}
		@Override
		public BlockerJob block() {
			return this;
		}
		@Override
		public BlockerJob unblock() {
			return this;
		}
	}

}
