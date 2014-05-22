package de.jreality.plugin.job;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.View;
import de.jreality.ui.JRealitySplashScreen;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.simplecontroller.widget.SplashScreen;

public class JobsTestPlugin extends ShrinkPanelPlugin implements ActionListener {

	private JButton
		jobButton1 = new JButton("Queue Job"),
		jobButton2 = new JButton("Queue Cancelable Job"),
		jobButton3 = new JButton("Queue No Progress Job");
	private JobQueuePlugin
		Q = null;
	
	public JobsTestPlugin() {
		shrinkPanel.setTitle("Job Test Plugin");
		shrinkPanel.setLayout(new GridLayout(3, 1, 1, 1));
		shrinkPanel.add(jobButton1);
		shrinkPanel.add(jobButton2);
		shrinkPanel.add(jobButton3);
		jobButton2.addActionListener(this);
		jobButton1.addActionListener(this);
		jobButton3.addActionListener(this);
	}
	
	public static class CancelableTestJob extends AbstractCancelableJob {
		
		private static int
			count = 1;
		private int
			jobIndex = count++;
		
		@Override
		public String getJobName() {
			return "Cancelable Job " + jobIndex;
		}
		
		@Override
		public void executeJob() throws Exception {
			for (int i = 0; i < 100; i++) {
				if (isCancelRequested()) {
					fireJobCancelled();
					return;
				}
				Thread.sleep(50);
				double progress = (i + 1) / 100.0;
				fireJobProgress(progress);
			}
		}
		
	}
	
	public static class TestJob extends AbstractJob {
		
		private static int
			count = 1;
		private int
			jobIndex = count++;
		
		@Override
		public String getJobName() {
			return "Test Job " + jobIndex;
		}
		
		@Override
		public void executeJob() throws Exception {
			for (int i = 0; i < 100; i++) {
				Thread.sleep(20);
				double progress = (i + 1) / 100.0;
				fireJobProgress(progress);
			}
		}
		
	}
	
	public static class TestJobWithoutProgress extends AbstractJob {
		
		private static int
			count = 1;
		private int
			jobIndex = count++;
		
		@Override
		public String getJobName() {
			return "No Progress Job " + jobIndex;
		}
		
		@Override
		public void executeJob() throws Exception {
			Thread.sleep(3000);
		}
		
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		if (jobButton2 == e.getSource()) {
			Job job = new CancelableTestJob();
			Q.queueJob(job);
		}
		if (jobButton1 == e.getSource()) {
			Job job = new TestJob();
			Q.queueJob(job);
		}
		if (jobButton3 == e.getSource()) {
			Job job = new TestJobWithoutProgress();
			Q.queueJob(job);
		}
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		Q = c.getPlugin(JobQueuePlugin.class);
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
	public static void main(String[] args) {
		SplashScreen splash = new JRealitySplashScreen();
		splash.setVisible(true);
		JRViewer v = new JRViewer();
		v.setShowPanelSlots(true, false, false, false);
		v.getController().setPropertyEngineEnabled(false);
		v.addBasicUI();
		v.addContentUI();
		v.registerPlugin(JobMonitorPlugin.class);
		v.registerPlugin(JobsTestPlugin.class);
		v.setSplashScreen(splash);
		v.startup();
		splash.setVisible(false);
	}
	
	
}
