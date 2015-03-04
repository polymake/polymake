package de.jreality.plugin.scripting;

import static javax.swing.JOptionPane.YES_NO_OPTION;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

import javax.swing.BorderFactory;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.AbstractTableModel;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.scripting.swing.ButtonCellEditor;
import de.jreality.plugin.scripting.swing.ButtonCellRenderer;
import de.jtem.jrworkspace.plugin.Controller;

public class PythonGUIManager extends JPanel implements ActionListener, ListSelectionListener {

	private static final long 
		serialVersionUID = 1L;
	private static final Random
		random = new Random();
	private static final Icon
		addIcon = ImageHook.getIcon("add.png"),
		removeIcon = ImageHook.getIcon("delete.png");
	private PythonScriptTool
		activeTool = null;
	private Controller
		controller = null;
	@SuppressWarnings("rawtypes")
	private List<PythonGUIPlugin>
		guiPlugins = new LinkedList<PythonGUIPlugin>();
	private PluginTableModel
		pluginModel = new PluginTableModel();
	private JTable
		pluginTable = new JTable(pluginModel);
	private JScrollPane
		pluginScroller = new JScrollPane(pluginTable);
	private GUITableModel
		guiModel = new GUITableModel();
	private JTable
		guiTable = new JTable(guiModel);
	private JScrollPane
		guiScroller = new JScrollPane(guiTable);
	private JCheckBox
		useGuiChecker = new JCheckBox("Show GUI Shrinker");
	private JPanel
		backendPanel = new JPanel();

	
	private class CreateGuiButton extends JButton implements ActionListener {

		private static final long 
			serialVersionUID = 1L;
		private PythonGUIPlugin<?>
			guiPlugin = null;
		
		public CreateGuiButton(PythonGUIPlugin<?> plugin) {
			this.guiPlugin = plugin;
			setIcon(addIcon);
			addActionListener(this);
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			if (activeTool == null) return;
			long id = random.nextLong();
			PythonGUI<?> gui = guiPlugin.getGUI(id);
			activeTool.getGuiList().add(gui);
			activeTool.updateGUI();
			updateBackendPanel();
			updateGUITable();
		}
		
	}
	
	private class RemoveGUIButton extends JButton implements ActionListener {

		private static final long 
			serialVersionUID = 1L;
		private PythonGUI<?>
			gui = null;
		
		public RemoveGUIButton(PythonGUI<?> gui) {
			this.gui = gui;
			setIcon(removeIcon);
			addActionListener(this);
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			Window w = SwingUtilities.getWindowAncestor(PythonGUIManager.this);
			int result = JOptionPane.showConfirmDialog(w, "Really delete this variable?", "Delete Variable", YES_NO_OPTION);
			if (result != JOptionPane.YES_OPTION) return;
			activeTool.getGuiList().remove(gui);
			activeTool.updateGUI();
			updateBackendPanel();
			gui.deleteProperties(controller);
			updateGUITable();
		}
		
	}
	
	private class PluginTableModel extends AbstractTableModel {

		private static final long 
			serialVersionUID = 1L;

		@Override
		public int getRowCount() {
			return guiPlugins.size();
		}

		@Override
		public int getColumnCount() {
			return 3;
		}

		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			return columnIndex == 2;
		}
		
		@Override
		public Class<?> getColumnClass(int columnIndex) {
			switch (columnIndex) {
			case 0:
				return Icon.class;
			case 1:
				return String.class;
			case 2:
				return CreateGuiButton.class;
			default:
				return String.class;
			}
		}
		
		@Override
		public Object getValueAt(int rowIndex, int columnIndex) {
			PythonGUIPlugin<?> plugin = guiPlugins.get(rowIndex);
			switch (columnIndex) {
			case 0:
				return plugin.getPluginInfo().icon;
			default:
				return plugin.getPluginInfo().name;
			case 2:
				CreateGuiButton guiButton = new CreateGuiButton(plugin);
				return guiButton;
			}
		}
		
	}
	
	private class GUITableModel extends AbstractTableModel {

		private static final long 
			serialVersionUID = 1L;

		@Override
		public int getRowCount() {
			if (activeTool == null) {
				return 0;
			}
			return activeTool.getGuiList().size();
		}

		@Override
		public int getColumnCount() {
			return 4;
		}
		
		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			return columnIndex == 1 || columnIndex == 2 || columnIndex == 3;
		}
		
		@Override
		public Class<?> getColumnClass(int columnIndex) {
			switch (columnIndex) {
			case 0:
				return Icon.class;
			case 3:
				return RemoveGUIButton.class;
			default:
				return String.class;
			}
		}

		@Override
		public Object getValueAt(int rowIndex, int columnIndex) {
			PythonGUI<?> gui = activeTool.getGuiList().get(rowIndex);
			PythonGUIPlugin<?> guiPlugin = controller.getPlugin(gui.getPluginClass());
			switch (columnIndex) {
			case 0:
				return guiPlugin.getPluginInfo().icon;
			case 1:
				return gui.getVariableDisplay();
			case 2:
				return gui.getVariableName();
			case 3:
				RemoveGUIButton removeButton = new RemoveGUIButton(gui);
				return removeButton;
			default:
				return "";
			}
		}
		
		@Override
		public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
			super.setValueAt(aValue, rowIndex, columnIndex);
			if (columnIndex == 2 && aValue instanceof String) {
				PythonGUI<?> gui = activeTool.getGuiList().get(rowIndex);
				gui.setVariableName((String)aValue);
				activeTool.updateGUI();
			}
			if (columnIndex == 1 && aValue instanceof String) {
				PythonGUI<?> gui = activeTool.getGuiList().get(rowIndex);
				gui.setVariableDisplay((String)aValue);
				activeTool.updateGUI();
			}			
		}
		
	}
	
	
	
	public PythonGUIManager() {
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.insets = new Insets(2, 2, 2, 2);
		c.fill = GridBagConstraints.HORIZONTAL;
		add(useGuiChecker, c);
		
		c.weighty = 1.0;
		c.fill = GridBagConstraints.BOTH;
		pluginScroller.setPreferredSize(new Dimension(200, 70));
		pluginScroller.setBorder(BorderFactory.createTitledBorder("GUI Plugins"));
		pluginScroller.setViewportBorder(BorderFactory.createEtchedBorder());
		add(pluginScroller, c);
		
		c.fill = GridBagConstraints.HORIZONTAL;
		c.weightx = 0.0;
		add(new JSeparator(JSeparator.HORIZONTAL), c);
		
		c.weightx = 1.0;
		c.weighty = 1.0;
		c.fill = GridBagConstraints.BOTH;
		guiScroller.setPreferredSize(new Dimension(200, 100));
		guiScroller.setBorder(BorderFactory.createTitledBorder("GUI Elements"));
		guiScroller.setViewportBorder(BorderFactory.createEtchedBorder());
		add(guiScroller, c);
		
		c.weighty = 0.0;
		add(backendPanel, c);
		
		pluginTable.getTableHeader().setPreferredSize(new Dimension(0, 0));
		pluginTable.setDefaultRenderer(CreateGuiButton.class, new ButtonCellRenderer());
		pluginTable.setDefaultEditor(CreateGuiButton.class, new ButtonCellEditor());
		pluginTable.getColumnModel().getColumn(0).setMaxWidth(30);
		pluginTable.getColumnModel().getColumn(2).setMaxWidth(30);
		pluginTable.setRowHeight(22);
		pluginTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		pluginTable.setFillsViewportHeight(true);
		guiTable.getTableHeader().setPreferredSize(new Dimension(0, 0));
		guiTable.setDefaultRenderer(RemoveGUIButton.class, new ButtonCellRenderer());
		guiTable.setDefaultEditor(RemoveGUIButton.class, new ButtonCellEditor());
		guiTable.setRowHeight(22);
		guiTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		guiTable.getSelectionModel().addListSelectionListener(this);
		guiTable.setFillsViewportHeight(true);
		updateGUITable();
		
		useGuiChecker.addActionListener(this);
	}
	
	public void updateGUITable() {
		guiTable.setModel(new GUITableModel());
		guiTable.getColumnModel().getColumn(0).setMaxWidth(30);
		guiTable.getColumnModel().getColumn(3).setMaxWidth(30);
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		if (activeTool == null) return;
		activeTool.setUseGUI(useGuiChecker.isSelected());
		activeTool.updateGUI();
	}
	
	@Override
	public void valueChanged(ListSelectionEvent e) {
		updateBackendPanel();
	}
	
	private void updateBackendPanel() {
		backendPanel.removeAll();
		backendPanel.setLayout(new GridLayout());
		backendPanel.setBorder(BorderFactory.createTitledBorder("GUI Properties"));
		int row = guiTable.getSelectedRow();
		if (activeTool == null || row < 0 || row >= activeTool.getGuiList().size()) {
			backendPanel.revalidate();
			revalidate();
			return;
		}
		PythonGUI<?> gui = activeTool.getGuiList().get(row);
		if (gui.getBackendGUI() != null) {
			backendPanel.add(gui.getBackendGUI());
		}
		backendPanel.revalidate();
		revalidate();
	}
	
	public void setTool(PythonScriptTool tool, Controller c) {
		this.activeTool = tool;
		this.controller = c;
		guiPlugins = c.getPlugins(PythonGUIPlugin.class);
		if (activeTool == null) {
			useGuiChecker.setSelected(false);
		} else {
			useGuiChecker.setSelected(tool.isUseGUI());
		}
		pluginTable.updateUI();
		guiTable.updateUI();
		updateBackendPanel();
		revalidate();
	}
	
}
