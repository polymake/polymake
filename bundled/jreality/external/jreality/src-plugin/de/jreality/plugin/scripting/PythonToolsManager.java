package de.jreality.plugin.scripting;

import static javax.swing.JOptionPane.WARNING_MESSAGE;
import static javax.swing.JOptionPane.YES_NO_OPTION;

import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import javax.imageio.ImageIO;
import javax.swing.BorderFactory;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.filechooser.FileFilter;
import javax.swing.table.AbstractTableModel;
import javax.swing.text.BadLocationException;
import javax.swing.text.Document;
import javax.swing.text.PlainDocument;

import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.flavor.FrontendFlavor;
import de.jtem.jrworkspace.plugin.flavor.PreferencesFlavor;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

public class PythonToolsManager extends Plugin implements PreferencesFlavor, ListSelectionListener, ActionListener, FrontendFlavor {

	protected static final String
		DEFAULT_SOURCE = "print('Hallo Welt')\nprint(C)";
	protected static final Icon
		DEFAULT_ICON = ImageHook.getIcon("python.png");
	private JPanel
		panel = new JPanel(),
		bottomPanel = new JPanel(),
		topPanel = new JPanel();
	private ShrinkPanel
		fileLinkPanel = new ShrinkPanel("File Link");
	private Controller
		controller = null;
	private PythonConsole
		console = null;
	private Icon
		icon = ImageHook.getIcon("python.png");
	private List<PythonScriptTool>
		tools = new LinkedList<PythonScriptTool>();
	private ToolTableModel
		toolTableModel = new ToolTableModel();
	private JTable
		toolsTable = new JTable(toolTableModel);
	private JScrollPane
		toolsScroller = new JScrollPane(toolsTable);
	private JButton
		importButton = new JButton(ImageHook.getIcon("folder2.png")),
		exportButton = new JButton(ImageHook.getIcon("disk.png")),
		addToolButton = new JButton(ImageHook.getIcon("add.png")),
		removeToolButton = new JButton(ImageHook.getIcon("delete.png"));
	private Document
		sourceDocument = new PlainDocument();
	private JTextArea
		sourceArea = new JTextArea(sourceDocument);
	private JScrollPane
		sourceScroller = new JScrollPane(sourceArea);
	private JTextField
		nameField = new JTextField(15),
		menuPathField = new JTextField(15);
	private JButton
		executeButton = new JButton("Execute", ImageHook.getIcon("cog_go.png")),
		saveButton = new JButton("Save and Update", ImageHook.getIcon("cog_edit.png")),
		browseFileLinkButton = new JButton("..."),
		browseIconButton = new JButton("...");
	private JCheckBox
		useMenuEntryChecker = new JCheckBox("Menu Path"),
		useToolItemChecker = new JCheckBox("Tool Bar Item"),
		useFileLinkChecker = new JCheckBox("Load Source From File");
	private JTextField
		fileLinkField = new JTextField();
	private JFileChooser
		toolFileChooser = new JFileChooser(),
		fileLinkChooser = new JFileChooser(),
		iconChooser = new JFileChooser();
	private JLabel
		iconLabel = new JLabel("Tool Icon");
	private FrontendListener
		frontendListener = null;
	private PythonGUIManager
		guiManager = new PythonGUIManager();
	private ShrinkPanel
		guiShrinker = new ShrinkPanel("Variable Interface");
		
	
	private boolean
		listenersEnabled = true;
		
	public static void main(String[] args) {
		JFrame f = new JFrame();
		f.setSize(800, 800);
		PythonToolsManager m = new PythonToolsManager();
		f.setLayout(new GridLayout());
		f.add(m.panel);
		f.setVisible(true);
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}
	
	public PythonToolsManager() {
		panel.setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(2, 2, 2, 2);
		c.fill = GridBagConstraints.HORIZONTAL;
		c.anchor = GridBagConstraints.WEST;
		c.gridwidth = GridBagConstraints.REMAINDER;

		topPanel.setLayout(new FlowLayout(FlowLayout.LEADING, 2, 2));
		topPanel.add(addToolButton);
		topPanel.add(removeToolButton);
		topPanel.add(new JSeparator(JSeparator.HORIZONTAL));
		topPanel.add(importButton);
		topPanel.add(exportButton);
		panel.add(topPanel, c);
		
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.fill = GridBagConstraints.BOTH;
		toolsTable.setFillsViewportHeight(true);
		toolsScroller.setViewportBorder(BorderFactory.createEtchedBorder());
		toolsScroller.setBorder(BorderFactory.createTitledBorder("Installed Tools"));
		toolsScroller.setPreferredSize(new Dimension(200, 100));
		panel.add(toolsScroller, c);
		panel.add(new JSeparator(JSeparator.VERTICAL), c);
		
		c.fill = GridBagConstraints.HORIZONTAL;
		c.gridwidth = GridBagConstraints.RELATIVE;
		panel.add(new JLabel("Name"), c);
		c.gridwidth = GridBagConstraints.REMAINDER;
		panel.add(nameField, c);
		
		c.fill = GridBagConstraints.NONE;
		c.gridwidth = GridBagConstraints.RELATIVE;
		panel.add(iconLabel, c);
		c.gridwidth = GridBagConstraints.REMAINDER;
		panel.add(browseIconButton, c);
		
		c.fill = GridBagConstraints.HORIZONTAL;
		c.gridwidth = GridBagConstraints.RELATIVE;
		panel.add(useMenuEntryChecker, c);
		c.gridwidth = GridBagConstraints.REMAINDER;
		panel.add(menuPathField, c);
		
		panel.add(useToolItemChecker, c);
		
		guiShrinker.setShrinked(true);
		guiShrinker.setLayout(new GridLayout());
		guiShrinker.add(guiManager);
		panel.add(guiShrinker, c);
		
		c.weighty = 1.0; 
		c.fill = GridBagConstraints.BOTH;
		sourceScroller.setPreferredSize(new Dimension(200, 100));
		sourceScroller.setBorder(BorderFactory.createTitledBorder("Script Source"));
		sourceScroller.setViewportBorder(BorderFactory.createEtchedBorder());
		panel.add(sourceScroller, c);
		
		c.weighty = 0.0;
		c.fill = GridBagConstraints.HORIZONTAL;
		fileLinkPanel.setLayout(new GridBagLayout());
		fileLinkPanel.add(useFileLinkChecker, c);
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.RELATIVE;
		fileLinkPanel.add(fileLinkField, c);
		c.weightx = 0.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		fileLinkPanel.add(browseFileLinkButton, c);
		c.fill = GridBagConstraints.BOTH;
		panel.add(fileLinkPanel, c);
		
		bottomPanel.setLayout(new FlowLayout(FlowLayout.TRAILING));
		bottomPanel.add(executeButton);
		bottomPanel.add(saveButton);
		
		c.weightx = 1.0;
		c.weighty = 0.0;
		panel.add(bottomPanel, c);
		
		toolsTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		toolsTable.getSelectionModel().addListSelectionListener(this);
		
		addToolButton.addActionListener(this);
		removeToolButton.addActionListener(this);
		saveButton.addActionListener(this);
		useFileLinkChecker.addActionListener(this);
		browseFileLinkButton.addActionListener(this);
		browseIconButton.addActionListener(this);
		useMenuEntryChecker.addActionListener(this);
		useToolItemChecker.addActionListener(this);
		nameField.addActionListener(this);
		menuPathField.addActionListener(this);
		executeButton.addActionListener(this);
		importButton.addActionListener(this);
		exportButton.addActionListener(this);
		
		toolsTable.getColumnModel().getColumn(0).setMaxWidth(30);
		toolsTable.getTableHeader().setPreferredSize(new Dimension(10, 0));
		toolsTable.setRowHeight(22);
		
		fileLinkField.setEditable(false);
		fileLinkPanel.setShrinked(true);
		
		fileLinkChooser.setMultiSelectionEnabled(false);
		fileLinkChooser.setDialogTitle("Open Python Script");
		fileLinkChooser.setDialogType(JFileChooser.OPEN_DIALOG);
		fileLinkChooser.setFileFilter(new FileFilter() {
			
			@Override
			public String getDescription() {
				return "Python Script (*.py)";
			}
			
			@Override
			public boolean accept(File f) {
				return f.isDirectory() || f.getName().toLowerCase().endsWith(".py");
			}
			
		});
		iconChooser.setMultiSelectionEnabled(false);
		iconChooser.setDialogTitle("Open Icon Image");
		iconChooser.setDialogType(JFileChooser.OPEN_DIALOG);
		iconChooser.setFileFilter(new FileFilter() {
			
			@Override
			public String getDescription() {
				return "PNG Image File (*.png)";
			}
			
			@Override
			public boolean accept(File f) {
				return f.isDirectory() || f.getName().toLowerCase().endsWith(".png");
			}
			
		});
		toolFileChooser.setMultiSelectionEnabled(false);
		toolFileChooser.setDialogTitle("Open Python Tool");
		toolFileChooser.setDialogType(JFileChooser.OPEN_DIALOG);
		toolFileChooser.setFileFilter(new FileFilter() {
			
			@Override
			public String getDescription() {
				return "jReality Python Tool (*.jpt)";
			}
			
			@Override
			public boolean accept(File f) {
				return f.isDirectory() || f.getName().toLowerCase().endsWith(".jpt");
			}
			
		});		
	}
	
	private class ToolTableModel extends AbstractTableModel {

		private static final long serialVersionUID = 1L;

		@Override
		public int getRowCount() {
			return tools.size();
		}

		@Override
		public int getColumnCount() {
			return 2;
		}
		
		@Override
		public Class<?> getColumnClass(int columnIndex) {
			switch (columnIndex) {
			case 0:
				return Icon.class;
			case 1:
				return PythonScriptTool.class;
			default:
				return String.class;
			}
		}

		@Override
		public Object getValueAt(int rowIndex, int columnIndex) {
			switch (columnIndex) {
			case 0:
				return tools.get(rowIndex).getIcon();
			case 1:
				return tools.get(rowIndex);
			default:
				return "-";
			}
		}
		
	}
	
	
	@Override
	public void valueChanged(ListSelectionEvent e) {
		updateTooleditor();
	}
	
	public void updateTooleditor() {
		listenersEnabled = false;
		PythonScriptTool tool = getSelectedTool();
		if (tool == null) {
			guiManager.setTool(null, controller);
			nameField.setText("");
			menuPathField.setText("");
			useFileLinkChecker.setSelected(false);
			fileLinkField.setText("");
			fileLinkField.setEnabled(false);
			browseFileLinkButton.setEnabled(false);
			sourceArea.setEditable(true);
			iconLabel.setIcon(null);
			try {
				int len = sourceDocument.getLength();
				sourceDocument.remove(0, len);
			} catch (BadLocationException e1) {
				e1.printStackTrace();
			}
			listenersEnabled = true;
			return;
		}
		guiManager.setTool(tool, controller);
		nameField.setText(tool.getName());
		menuPathField.setText(tool.getMenuPath());
		useFileLinkChecker.setSelected(tool.isUseFileLink());
		fileLinkField.setEnabled(tool.isUseFileLink());
		browseFileLinkButton.setEnabled(tool.isUseFileLink());
		sourceArea.setEditable(!tool.isUseFileLink());
		iconLabel.setIcon(tool.getIcon());
		useMenuEntryChecker.setSelected(tool.isUseMenuItem());
		useToolItemChecker.setSelected(tool.isUseToolItem());
		menuPathField.setEnabled(useMenuEntryChecker.isSelected());
		if (tool.getFileLink() != null) {
			fileLinkField.setText(tool.getFileLink().getAbsolutePath());
		} else {
			fileLinkField.setText("");
		}
		int len = sourceDocument.getLength();
		try {
			sourceDocument.remove(0, len);
			sourceDocument.insertString(0, tool.getSourceCode(), null);
		} catch (BadLocationException e1) {
			e1.printStackTrace();
		}
		listenersEnabled = true;
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		if (!listenersEnabled) return;
		PythonScriptTool tool = getSelectedTool();
		if (addToolButton == e.getSource()) {
			PythonScriptTool newTool = new PythonScriptTool(console, controller);
			tools.add(newTool);
			installTool(newTool);
			selectTool(newTool);
		}
		if (removeToolButton == e.getSource()) {
			if (tool == null) return;
			Window w = SwingUtilities.getWindowAncestor(panel);
			int result = JOptionPane.showConfirmDialog(w, "Really delete this tool?", "Delete Tool", YES_NO_OPTION);
			if (result != JOptionPane.YES_OPTION) return;
			tools.remove(tool);
			uninstallTool(tool);
		}
		if (exportButton == e.getSource()) {
			if (tool == null) return;
			Window w = SwingUtilities.getWindowAncestor(panel);
			int result = toolFileChooser.showSaveDialog(w);
			if (result != JFileChooser.APPROVE_OPTION) return;
			File f = toolFileChooser.getSelectedFile();
			if (!f.getName().toLowerCase().endsWith(".jpt")) {
				f = new File(f.getAbsolutePath() + ".jpt");
			}
			if (f.exists()) {
				int r = JOptionPane.showConfirmDialog(w, "File " + f.getName() + " exists, do you want to overwrite?", "Export Tool", YES_NO_OPTION);
				if (r != JOptionPane.YES_OPTION) return;
			}
			boolean useFileLink = tool.isUseFileLink();
			tool.setUseFileLink(false);
			PythonIOController exportController = new PythonIOController(controller);
			tool.storeProperties(exportController);
			exportController.storeProperty(getClass(), "toolId", tool.getId());
			exportController.storeProperty(getClass(), "toolName", tool.getName());
			exportController.storeProperty(getClass(), "toolIcon", tool.getIcon());
			exportController.writeProperties(f);
			tool.setUseFileLink(useFileLink);
		}
		if (importButton == e.getSource()) {
			Window w = SwingUtilities.getWindowAncestor(panel);
			int result = toolFileChooser.showOpenDialog(w);
			if (result != JFileChooser.APPROVE_OPTION) return;
			File f = toolFileChooser.getSelectedFile();
			PythonIOController importController = new PythonIOController(controller);
			importController.readProperties(f);
			long toolId = importController.getProperty(getClass(), "toolId", -1L);
			PythonScriptTool newTool = new PythonScriptTool(console, controller, toolId);
			newTool.restoreProperties(importController); 
			newTool.setNewGUIIds();
			tools.add(newTool);
			installTool(newTool);
			selectTool(newTool);
		}
		if (executeButton == e.getSource()) {
			if (tool == null) return;
			tool.execute();
			return;
		}
		if (saveButton == e.getSource()) {
			if (tool == null) return;
			uninstallTool(tool);
			tool.setName(nameField.getText());
			tool.setMenuPath(menuPathField.getText());
			int len = sourceDocument.getLength();
			try {
				String source = sourceDocument.getText(0, len);
				tool.setSourceCode(source);
			} catch (BadLocationException e1) {
				e1.printStackTrace();
			}
			installTool(tool);
		}
		if (useFileLinkChecker == e.getSource()) {
			if (tool == null) return;
			tool.setUseFileLink(useFileLinkChecker.isSelected());
		}
		if (browseFileLinkButton == e.getSource()) {
			if (tool == null) return;
			Window w = SwingUtilities.getWindowAncestor(panel);
			if (tool.getFileLink() != null) {
				fileLinkChooser.setSelectedFile(tool.getFileLink());
			}
			int r = fileLinkChooser.showOpenDialog(w);
			if (r != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File fileLink = fileLinkChooser.getSelectedFile();
			tool.setFileLink(fileLink);
		}
		if (browseIconButton == e.getSource()) {
			if (tool == null) return;
			Window w = SwingUtilities.getWindowAncestor(panel);
			int r = iconChooser.showOpenDialog(w);
			if (r != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File iconFile = iconChooser.getSelectedFile();
			try {
				BufferedImage image = ImageIO.read(iconFile);
				if (image.getWidth() > 22 || image.getHeight() > 22) {
					JOptionPane.showMessageDialog(w, "Please select a smaller image.", "Image too large", WARNING_MESSAGE);
					return;
				}
				ImageIcon icon = new ImageIcon(image);
				tool.setIcon(icon);
			} catch (IOException e1) {
				e1.printStackTrace();
			}
		}
		if (useToolItemChecker == e.getSource() || useMenuEntryChecker == e.getSource()) {
			if (tool == null) return;
			tool.setUseMenuItem(useMenuEntryChecker.isSelected());
			tool.setUseToolItem(useToolItemChecker.isSelected());
			updateTool(tool);
		}
		if (menuPathField == e.getSource() || nameField == e.getSource()) {
			if (tool == null) return;
			updateTool(tool);
		}
		updateTooleditor();
		toolsTable.revalidate();
	}
	
	
	public PythonScriptTool getSelectedTool() {
		int index = toolsTable.getSelectionModel().getMinSelectionIndex();
		if (index < 0) return null;
		if (index >= tools.size()) return null;
		return tools.get(index);
	}
	
	
	public void updateTool(PythonScriptTool tool) {
		uninstallTool(tool);
		installTool(tool);
	}
	
	public void installTool(PythonScriptTool tool) {
		ViewMenuBar menu = controller.getPlugin(ViewMenuBar.class);
		String[] menuPath = tool.getMenuPath().split(",");
		ViewToolBar toolBar = controller.getPlugin(ViewToolBar.class);
		if (tool.isUseMenuItem()) {
			menu.addMenuItem(PythonToolsManager.class, 1.0, tool, menuPath);
		}
		if (tool.isUseToolItem()) {
			toolBar.addAction(PythonToolsManager.class, 10000.0, tool);
		}
		tool.updateGUI();
		frontendListener.updateFrontendUI();
	}
	
	public void uninstallTool(PythonScriptTool tool) {
		ViewMenuBar menu = controller.getPlugin(ViewMenuBar.class);
		ViewToolBar toolBar = controller.getPlugin(ViewToolBar.class);
		menu.removeMenuItem(PythonToolsManager.class, tool);
		toolBar.removeAction(PythonToolsManager.class, tool);
		tool.removeGUI();
		frontendListener.updateFrontendUI();
	}
	public void selectTool(PythonScriptTool tool) {
		int index = tools.indexOf(tool);
		toolsTable.getSelectionModel().setSelectionInterval(index, index);
		updateTooleditor();
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		this.controller = c;
		this.console = c.getPlugin(PythonConsole.class);
		ViewToolBar toolBar = controller.getPlugin(ViewToolBar.class);
		toolBar.addSeparator(PythonToolsManager.class, 9999.0);
		for (PythonScriptTool tool : tools) {
			installTool(tool);
		}
	}
	
	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "chooserDir", fileLinkChooser.getCurrentDirectory().getAbsolutePath());
		c.storeProperty(getClass(), "iconChooserDir", iconChooser.getCurrentDirectory().getAbsolutePath());
		List<Long> toolIdList = new LinkedList<Long>();
		for (PythonScriptTool tool : tools) {
			tool.storeProperties(c);
			toolIdList.add(tool.getId());
		}
		c.storeProperty(getClass(), "toolIds", toolIdList);
	}
	
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		PythonConsole console = c.getPlugin(PythonConsole.class);
		String chooserDir = c.getProperty(getClass(), "chooserDir", ".");
		fileLinkChooser.setCurrentDirectory(new File(chooserDir));
		String iconDir = c.getProperty(getClass(), "iconChooserDir", ".");
		iconChooser.setCurrentDirectory(new File(iconDir));
		List<Long> toolIds = c.getProperty(getClass(), "toolIds", new LinkedList<Long>());
		for (long id : toolIds) {
			PythonScriptTool tool = new PythonScriptTool(console, c, id);
			tool.restoreProperties(c);
			tools.add(tool);
		}
	};
	
	
	@Override
	public String getMainName() {
		return "Python Scripting";
	}

	@Override
	public JPanel getMainPage() {
		return panel;
	}

	@Override
	public Icon getMainIcon() {
		return icon;
	}

	@Override
	public int getNumSubPages() {
		return 0;
	}
	@Override
	public String getSubPageName(int i) {
		return null;
	}
	@Override
	public JPanel getSubPage(int i) {
		return null;
	}
	@Override
	public Icon getSubPageIcon(int i) {
		return null;
	}

	@Override
	public void setFrontendListener(FrontendListener l) {
		this.frontendListener = l;
	}

}
