package de.jreality.plugin.job;

import static javax.swing.ListSelectionModel.SINGLE_SELECTION;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.Map;

import javax.swing.AbstractCellEditor;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.Timer;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableCellEditor;

import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class JobMonitorPlugin extends ShrinkPanelPlugin {

	private JobQueuePlugin
		Q = null;
	private JobSynchronzerThread
		synchronzerThread = new JobSynchronzerThread();
	private QueueTableModel
		model = new QueueTableModel();
	private JTable
		queueTabel = new JTable();
	private JScrollPane
		queueScroller = new JScrollPane(queueTabel);
	private Map<Job, Double>
		progressMap = new HashMap<Job, Double>();
	private JobAdapter
		jobAdapter = new JobAdapter();
	private MonitorCellRenderer
		cellRenderer = new MonitorCellRenderer();
	private MonitorCellEditor
		cellEditor = new MonitorCellEditor();
	private Icon
		stopIcon = ImageHook.getIcon("control_stop_blue.png"),
		stopIconDisabled = ImageHook.getIcon("control_stop.png");
	private JobProgressBar
		jobProgressBar = new JobProgressBar();
	
	public JobMonitorPlugin() {
		shrinkPanel.setTitle("Job Monitor");
		shrinkPanel.setLayout(new BorderLayout());
		shrinkPanel.add(queueScroller, BorderLayout.CENTER);
		
		queueScroller.setPreferredSize(new Dimension(200, 100));
		queueTabel.getTableHeader().setPreferredSize(new Dimension(0, 0));
		queueTabel.setFillsViewportHeight(true);
		queueTabel.setDefaultRenderer(Component.class, cellRenderer);
		queueTabel.setDefaultEditor(CancelButton.class, cellEditor);
		queueTabel.getSelectionModel().setSelectionMode(SINGLE_SELECTION);
	}
	
	private class JobSynchronzerThread extends Thread {
		
		public JobSynchronzerThread() {
			super("jReality Job Monitor");
		}
		
		@Override
		public void run() {
			while (true) {
				updateJobTable();
				progressMap.keySet().retainAll(Q.Q);
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {}
			}
		}
		
	}
	
	private class JobAdapter implements JobListener {

		@Override
		public void jobStarted(Job job) {
			updateJobTable();
		}
		@Override
		public void jobProgress(Job job, double progress) {
			progressMap.put(job, progress);
			updateJobTable();
		}
		@Override
		public void jobFinished(Job job) {
			updateJobTable();
		}
		@Override
		public void jobFailed(Job job, Exception e) {
			updateJobTable();
		}
		@Override
		public void jobCancelled(Job job) {
			updateJobTable();
		}
		
	}
	
	
	private class CancelButton extends JButton implements ActionListener {
		
		private static final long serialVersionUID = 1L;
		private Job job = null;
		
		public CancelButton(Job job) {
			this.job = job;
			setIcon(stopIcon);
			setDisabledIcon(stopIconDisabled);
			addActionListener(this);
			setToolTipText("Cancel Job");
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			Q.cancelJob(job);
			queueTabel.getCellEditor().stopCellEditing();
		}
		
	}
	
	
	private class JobProgressBar extends JProgressBar {
		
		private static final long serialVersionUID = 1L;
		private Job job = null;
		
		public JobProgressBar() {
			super(0, 1000);
			setStringPainted(true);
            int delayMilliseconds = 60;
            Timer repaintTimer = new Timer(delayMilliseconds, new ActionListener(){
                @Override
                public void actionPerformed(ActionEvent ae) {
                	if (jobProgressBar.isIndeterminate()) {
                		queueTabel.repaint();
                	}
                }
            });
            repaintTimer.start();
		}
		
		public void setJob(Job job) {
			this.job = job;
		}
		
		@Override
		public void paint(Graphics g) {
			if (progressMap.containsKey(job)) {
				setIndeterminate(false);
				double progress = progressMap.get(job);
				setValue((int)(progress * 1000));
			} else {
				setIndeterminate(true);
			}
			setStringPainted(true);
			super.paint(g);
		}
		
	}
	
	
	
	private class QueueTableModel extends AbstractTableModel {

		private static final long serialVersionUID = 1L;

		@Override
		public int getColumnCount() {
			return 2;
		}

		@Override
		public int getRowCount() {
			return Q.Q.size();
		}

		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			if (columnIndex != 1) return false;
			if (rowIndex >= Q.Q.size()) return false;
			Job job = Q.Q.get(rowIndex);
			if (rowIndex == 0) {
				return job instanceof CancelableJob;
			} else {
				return true;
			}
		}
		
		@Override
		public Class<?> getColumnClass(int col) {
			switch (col) {
			case 0:
			default:
				return Component.class;
			case 1:
				return CancelButton.class;
			}

		}
		
		@Override
		public Object getValueAt(int row, int col) {
			if (row >= Q.Q.size()) return "-";
			Job job = Q.Q.get(row);
			switch (col) {
			case 0:
			default:
				String name = job.getJobName();
				if (progressMap.containsKey(job)) {
					double progress = progressMap.get(job);
					name += ": " + (int)(100 * progress) + "%";
				}
				if (row == 0) {
					jobProgressBar.setJob(job);
					jobProgressBar.setString(name);
					return jobProgressBar;
				} else {
					return new JLabel(name);
				}
			case 1:
				CancelButton cb = new CancelButton(job);
				return cb;
			}
		}
		
	}
	
	
	
	private class MonitorCellRenderer extends DefaultTableCellRenderer {

		private static final long serialVersionUID = 1L;
		private JButton renderButton = new JButton();
		
		public MonitorCellRenderer() {
			renderButton.setToolTipText("Cancel Job");
		}
		
		@Override
		public Component getTableCellRendererComponent(JTable table,
				Object value, boolean isSelected, boolean hasFocus, int row,
				int column) {
			if (value instanceof CancelButton) {
				CancelButton buttonValue = (CancelButton)value;
				renderButton.setIcon(buttonValue.getIcon());
				renderButton.setText(buttonValue.getText());
				renderButton.setDisabledIcon(stopIconDisabled);
				if (row == 0 && !(buttonValue.job instanceof CancelableJob)) {
					renderButton.setEnabled(false);
				} else {
					renderButton.setEnabled(true);
				}
				return renderButton;
			} else if (value instanceof Component) { 
				return (Component)value;
			} else {
				return super.getTableCellRendererComponent(table, "", isSelected, hasFocus, row, column);
			}
		}
		
		@Override
		public void updateUI() {
			super.updateUI();
			if (renderButton != null) {
				renderButton.updateUI();
			}
		}
		
	}
	
	
	private class MonitorCellEditor extends AbstractCellEditor implements TableCellEditor {

		private static final long 	
			serialVersionUID = 1L;
		private JLabel
			defaultEditor = new JLabel("-");
		private Object 
			activeValue = null;
		
		@Override
		public Component getTableCellEditorComponent(JTable table,
				Object value, boolean isSelected, int row, int column) {
			this.activeValue = value;
			if (value instanceof Component) {
				return (Component)value;
			}
			return defaultEditor;
		}
		@Override
		public Object getCellEditorValue() {
			return activeValue;
		}
		
	}
	
	
	public void updateJobTable() {
		queueTabel.repaint();
		queueScroller.revalidate();
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		Q = c.getPlugin(JobQueuePlugin.class);
		Q.addJobListener(jobAdapter);
		synchronzerThread.start();
		queueTabel.setModel(model);
		queueTabel.getColumnModel().getColumn(1).setMaxWidth(25);
	}
	
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = super.getPluginInfo();
		info.name = "Job Monitor Plugin";
		info.vendorName = "Stefan Sechelmann";
		info.email = "sechel@math.tu-berlin.de";
		return info;
	}

}
