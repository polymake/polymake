package de.jreality.plugin.experimental;

import java.awt.Dimension;
import java.awt.GridLayout;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.swing.Icon;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.event.CellEditorListener;
import javax.swing.event.ChangeEvent;
import javax.swing.table.AbstractTableModel;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.audio.Audio;
import de.jreality.plugin.audio.AudioOptions;
import de.jreality.plugin.audio.AudioPreferences;
import de.jreality.plugin.basic.InfoOverlayPlugin;
import de.jreality.plugin.basic.Inspector;
import de.jreality.plugin.basic.MainPanel;
import de.jreality.plugin.basic.PropertiesMenu;
import de.jreality.plugin.basic.Shell;
import de.jreality.plugin.basic.StatusBar;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewPreferences;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.menu.BackgroundColor;
import de.jreality.plugin.menu.CameraMenu;
import de.jreality.plugin.menu.DisplayOptions;
import de.jreality.plugin.menu.ExportMenu;
import de.jreality.plugin.scene.Avatar;
import de.jreality.plugin.scene.Lights;
import de.jreality.plugin.scene.SceneShrinkSlot;
import de.jreality.plugin.scene.Sky;
import de.jreality.plugin.scene.VRExamples;
import de.jreality.plugin.scene.VRPanel;
import de.jreality.plugin.scene.WindowManager;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;
import de.jtem.jrworkspace.plugin.flavor.PreferencesFlavor;

@Experimental
public class JRPluginManager extends Plugin implements PreferencesFlavor, CellEditorListener {

	private Controller
		controller = null;
	private JPanel
		panel = new JPanel();
	private JTable
		pluginTable = new JTable();
	private Map<Class<? extends Plugin>, Plugin>
		pluginMap = new HashMap<Class<? extends Plugin>, Plugin>();
	private Map<Plugin, Boolean>
		activeMap = new HashMap<Plugin, Boolean>();
	private List<Class<? extends Plugin>>
		pluginList = new LinkedList<Class<? extends Plugin>>();
	private Icon
		defaultPluginIcon = ImageHook.getIcon("plugin.png");
		
	
	public JRPluginManager() {
		panel.setLayout(new GridLayout());
		panel.add(pluginTable);
	}
	
	private void initTable() {
		pluginTable.setModel(new PluginTableModel());
		pluginTable.setRowHeight(24);
		pluginTable.getTableHeader().setPreferredSize(new Dimension(10, 0));
		pluginTable.getColumnModel().getColumn(0).setMaxWidth(24);
		pluginTable.getColumnModel().getColumn(0).setMinWidth(24);
		pluginTable.getColumnModel().getColumn(2).setMaxWidth(40);
		pluginTable.getColumnModel().getColumn(2).setMinWidth(40);
		pluginTable.getDefaultEditor(Boolean.class).addCellEditorListener(this);
		pluginTable.revalidate();
	}
	
	
	protected Plugin getPlugin(Class<? extends Plugin> pClass) {
		if (!pluginMap.containsKey(pClass)) {
			try {
				Plugin p = (Plugin)pClass.newInstance();
				if (controller.isActive(p)) {
					p = controller.getPlugin(pClass);
				}
				pluginMap.put(pClass, p);
			} catch (Exception e) {}
		}
		return pluginMap.get(pClass);
	}
	
	
	protected Boolean getActiveState(Plugin p) {
		if (!activeMap.containsKey(p)) {
			activeMap.put(p, controller.isActive(p));
		}
		return activeMap.get(p);
	}
	
	protected void setActiveState(Plugin p, Boolean active) {
		activeMap.put(p, active);
	}
	

	protected class PluginTableModel extends AbstractTableModel {

		private static final long 
			serialVersionUID = 1L;

		public int getColumnCount() {
			return 3;
		}

		public int getRowCount() {
			return pluginList.size();
		}

		public Object getValueAt(int rowIndex, int columnIndex) {
			if (rowIndex < 0 || rowIndex >= pluginList.size()) {
				return "Error";
			}
			Class<? extends Plugin> pClass = pluginList.get(rowIndex);
			Plugin p = getPlugin(pClass);
			if (p == null) {
				return null;
			}
			switch (columnIndex) {
			case 0:
				Icon icon = p.getPluginInfo().icon;
				if (icon == null) {
					icon = defaultPluginIcon;
				}
				return icon;
			case 1:
				return p.getPluginInfo().name;
			case 2:
				return getActiveState(p);
			}
			return null;
		}
		
		@Override
		public Class<?> getColumnClass(int columnIndex) {
			switch (columnIndex) {
			case 0: 
				return Icon.class;
			default:
			case 1: 
				return String.class;
			case 2: 
				return Boolean.class;
			}
		}
		
		@Override
		public boolean isCellEditable(int rowIndex, int columnIndex) {
			if (columnIndex <= 1) {
				return false;
			}
			Class<? extends Plugin> pClass = pluginList.get(rowIndex);
			Plugin p = getPlugin(pClass);
			if (p == null) {
				return false;
			}
			if (columnIndex == 2) {
				return p.getPluginInfo().isDynamic;
			}
			return false;
		}
		
	}
	
	
	public void editingCanceled(ChangeEvent e) {
		
	}

	public void editingStopped(ChangeEvent e) {
		int row = pluginTable.getSelectedRow();
		if (row < 0 || row >= pluginList.size()) {
			return;
		}
		Class<? extends Plugin> pClass = pluginList.get(row);
		Plugin p = getPlugin(pClass);
		if (p == null) {
			return;
		}
		if (pluginTable.getDefaultEditor(Boolean.class) == e.getSource()) {
			boolean isActive = getActiveState(p);
			if (isActive) {
				try {
					p.uninstall(controller);
				} catch (Exception e1) {
					System.out.println("could not properly uninstall plug-in " + p + ": " + e1.getLocalizedMessage());
				}
				try {
					p.storeStates(controller);
				} catch (Exception e2) {
					System.out.println("could not properly store plug-in states for " + p + ": " + e2.getLocalizedMessage());
				}
			} else {
				try {
					p.restoreStates(controller);
				} catch (Exception e2) {
					System.out.println("could not properly restore plug-in states for " + p + ": " + e2.getLocalizedMessage());
				}
				try {
					p.install(controller);
				} catch (Exception e1) {
					System.out.println("could not properly install plug-in " + p + ": " + e1.getLocalizedMessage());
				}
			}
			setActiveState(p, !isActive);
		}
		pluginTable.revalidate();
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		controller = c;
		List<? extends Plugin> allPlugins = c.getPlugins(Plugin.class);
		for (Plugin p : allPlugins) { 
			try {
				p.getClass().getConstructor();
			} catch (Exception e) {
				continue;
			}
			if (p.getClass().equals(JRPluginManager.class)) {
				continue;
			}
			if (p.getPluginInfo().isDynamic) {
				pluginList.add(p.getClass());
			}
		}
		for (Class<? extends Plugin> pClass : getAllPluginClasses()) {
			if (!pluginList.contains(pClass)) {
				pluginList.add(pClass);
			}
		}
		initTable();
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("jReality Plugin Manager", "Stefan Sechelmann");
		info.icon = ImageHook.getIcon("plugin.png");
		return info;
	}

	public Icon getMainIcon() {
		return ImageHook.getIcon("plugin.png");
	}

	public String getMainName() {
		return "jReality Plugin Manager";
	}

	public JPanel getMainPage() {
		return panel;
	}

	public int getNumSubPages() {
		return 0;
	}

	public JPanel getSubPage(int i) {
		return null;
	}

	public Icon getSubPageIcon(int i) {
		return null;
	}

	public String getSubPageName(int i) {
		return null;
	}

	
	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentUI();
		v.setShowPanelSlots(true, true, true, true);
		v.setShowToolBar(true);
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.setContent(Primitives.coloredCube());
		v.registerPlugin(new JRPluginManager());
		v.startup();
	}
	
	
	public static List<Class<? extends Plugin>> getAllPluginClasses() {
		List<Class<? extends Plugin>> l = new LinkedList<Class<? extends Plugin>>();
		// audio
		l.add(Audio.class);
		l.add(AudioOptions.class);
		l.add(AudioPreferences.class);
		// basic
		l.add(InfoOverlayPlugin.class);
		l.add(Inspector.class);
		l.add(MainPanel.class);
		l.add(PropertiesMenu.class);
		l.add(Shell.class);
		l.add(StatusBar.class);
		l.add(ViewMenuBar.class);
		l.add(ViewPreferences.class);
		l.add(ViewToolBar.class);
		// content
		l.add(ContentAppearance.class);
		l.add(ContentLoader.class);
		l.add(ContentTools.class);
		// experimental
		l.add(LoadSaveSettings.class);
		l.add(ManagedContent.class);
		l.add(ManagedContentGUI.class);
		l.add(ViewerKeyListenerPlugin.class);
		l.add(WebContentLoader.class);
		// menu
		l.add(BackgroundColor.class);
		l.add(CameraMenu.class);
		l.add(DisplayOptions.class);
		l.add(ExportMenu.class);
		// scene
		l.add(Avatar.class);
		l.add(Lights.class);
		l.add(SceneShrinkSlot.class);
		l.add(Sky.class);
		l.add(VRExamples.class);
		l.add(VRPanel.class);
		l.add(WindowManager.class);
		return l;
	}
	
}
