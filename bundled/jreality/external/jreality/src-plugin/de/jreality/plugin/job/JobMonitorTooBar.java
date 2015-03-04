package de.jreality.plugin.job;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;

import javax.swing.JProgressBar;
import javax.swing.JToolBar;

import de.jreality.plugin.basic.View;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.flavor.PerspectiveFlavor;
import de.jtem.jrworkspace.plugin.flavor.ToolBarFlavor;

public class JobMonitorTooBar extends Plugin implements ToolBarFlavor {

	private JobQueuePlugin
		Q = null;
	private JToolBar
		mainPanel = new JToolBar();
	private JProgressBar
		progressBar = new JProgressBar();
	
	public JobMonitorTooBar() {
		mainPanel.setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.insets = new Insets(0, 0, 2, 2);
		mainPanel.add(progressBar, c);
		reset();
	}
	
	private void reset() {
		progressBar.setMinimumSize(new Dimension(150, 20));
		progressBar.setString("Job Progress Monitor");
		progressBar.setValue(0);
		progressBar.setIndeterminate(false);
		progressBar.setMaximum(1000);
		progressBar.setStringPainted(true);
	}
	
	private class JobAdapter implements JobListener {

		@Override
		public void jobStarted(Job job) {
			progressBar.setIndeterminate(true);
			progressBar.setValue(0);
			progressBar.setString(job.getJobName());
			progressBar.setStringPainted(true);
		}
		@Override
		public void jobProgress(Job job, double progress) {
			String status = job.getJobName() + ": " + (int)(100 * progress) + "%";
			progressBar.setString(status);
			progressBar.setValue((int)(1000 * progress));
			progressBar.setIndeterminate(false);
		}
		@Override
		public void jobFinished(Job job) {
			reset();
		}
		@Override
		public void jobFailed(Job job, Exception e) {
			reset();
		}
		@Override
		public void jobCancelled(Job job) {
			reset();
		}
		
	}
	
	@Override
	public Component getToolBarComponent() {
		return mainPanel;
	}

	@Override
	public Class<? extends PerspectiveFlavor> getPerspective() {
		return View.class;
	}

	@Override
	public double getToolBarPriority() {
		return 1.0;
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		Q = c.getPlugin(JobQueuePlugin.class);
		Q.addJobListener(new JobAdapter());
	}
	
	public void setFloatable(boolean floatable) {
		mainPanel.setFloatable(floatable);
	}
	
}
