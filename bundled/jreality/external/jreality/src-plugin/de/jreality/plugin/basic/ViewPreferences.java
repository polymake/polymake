package de.jreality.plugin.basic;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import javax.swing.Icon;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.SceneGraphNode;
import de.jreality.ui.viewerapp.actions.AbstractJrToggleAction;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.FrontendFlavor;
import de.jtem.jrworkspace.plugin.flavor.PreferencesFlavor;

public class ViewPreferences extends Plugin implements PreferencesFlavor, ActionListener, FrontendFlavor {

	private static final boolean DEFAULT_SHOW_MENU = true;
	private static final boolean DEFAULT_SHOW_TOOLBAR = false;
	
	private ViewMenuBar
		viewMenuBar = null;
	private JPanel 
		mainPage = new JPanel();
	private JCheckBox
		useExclusiveFullscreen = new JCheckBox("Exclusive Fullscreen", true),
		threadSafeChecker = new JCheckBox("Thread Safe Scene Graph", SceneGraphNode.getThreadSafe()),
		resetTextureChecker = new JCheckBox("Reset Texture Transform on Startup", true);
	private AbstractJrToggleAction
		showMenubarToggle = null,
		showToolbarToggle = null;
	private JComboBox
		colorChooserModeCombo = new JComboBox(new String[] {"HUE", "SAT", "BRI", "RED", "GREEN", "BLUE"});
	private List<ColorPickerModeChangedListener>
		colorModeListeners = new CopyOnWriteArrayList<ColorPickerModeChangedListener>();
	private FrontendListener
		frontendListener = null;
	
	public static interface ColorPickerModeChangedListener {
		
		public void colorPickerModeChanged(int mode);
		
	}
	
	public ViewPreferences() {
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.BOTH;
		c.insets = new Insets(2,2,2,2);
		c.anchor = GridBagConstraints.CENTER;
		mainPage.setLayout(new GridBagLayout());
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		mainPage.add(threadSafeChecker, c);
		c.weightx = 0.0;
		c.gridwidth = GridBagConstraints.RELATIVE;
		mainPage.add(new JLabel("Color Chooser Mode"), c);
		c.weightx = 1.0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		mainPage.add(colorChooserModeCombo, c);
		colorChooserModeCombo.setSelectedIndex(1);
		mainPage.add(useExclusiveFullscreen, c);
		mainPage.add(resetTextureChecker, c);
		
		threadSafeChecker.addActionListener(this);
		colorChooserModeCombo.addActionListener(this);
		
		showMenubarToggle = new AbstractJrToggleAction("Show Menu Bar") {
			private static final long serialVersionUID = 1L;
			{
				setShortCut(KeyEvent.VK_M, KeyEvent.SHIFT_DOWN_MASK, true);
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				if (frontendListener != null) {
					frontendListener.setShowMenuBar(showMenubarToggle.isSelected());
				}
			}
		};
		showMenubarToggle.setSelected(DEFAULT_SHOW_MENU);
		showToolbarToggle = new AbstractJrToggleAction("Show Tool Bar") {
			private static final long serialVersionUID = 1L;
			{
				setShortCut(KeyEvent.VK_T, KeyEvent.SHIFT_DOWN_MASK, true);
			}
			@Override
			public void actionPerformed(ActionEvent e) {
				if (frontendListener != null) {
					frontendListener.setShowToolBar(showToolbarToggle.isSelected());
				}
			}
		};
		showToolbarToggle.setSelected(DEFAULT_SHOW_TOOLBAR);
	}
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Viewer Preferences";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("preferences.png");
		return info;
	}
	
	public void actionPerformed(ActionEvent e) {
		Object s = e.getSource();
		if (threadSafeChecker == s) {
			System.out.println("ThreadSafe is " + threadSafeChecker.isSelected());
			SceneGraphNode.setThreadSafe(threadSafeChecker.isSelected());
		}
		if (colorChooserModeCombo == s) {
			fireColorModeChanged(colorChooserModeCombo.getSelectedIndex());
		}
	}

	protected void fireColorModeChanged(int mode) {
		for (ColorPickerModeChangedListener l : colorModeListeners) {
			l.colorPickerModeChanged(mode);
		}
	}
	
	
	public void setShowToolBar(boolean show) {
		showToolbarToggle.setSelected(show);
		if (frontendListener != null) {
			frontendListener.setShowToolBar(showToolbarToggle.isSelected());
		}
	}
	
	
	public void setShowMenuBar(boolean show) {
		showMenubarToggle.setSelected(show);
		if (frontendListener != null) {
			frontendListener.setShowMenuBar(showMenubarToggle.isSelected());
		}
	}
	
	
	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "threadSafeSceneGraph", SceneGraphNode.getThreadSafe());
		c.storeProperty(getClass(), "colorChooserMode", colorChooserModeCombo.getSelectedIndex());
		c.storeProperty(getClass(), "showMenuBar", showMenubarToggle.isSelected());
		c.storeProperty(getClass(), "showToolBar", showToolbarToggle.isSelected());
		c.storeProperty(getClass(), "useExclusiveFullscreen", useExclusiveFullscreen.isSelected());
		c.storeProperty(getClass(), "resetTextureTransfom", resetTextureChecker.isSelected());
		super.storeStates(c);
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		threadSafeChecker.setSelected(c.getProperty(getClass(), "threadSafeSceneGraph", SceneGraphNode.getThreadSafe()));
		SceneGraphNode.setThreadSafe(threadSafeChecker.isSelected());
		colorChooserModeCombo.setSelectedIndex(c.getProperty(getClass(), "colorChooserMode", colorChooserModeCombo.getSelectedIndex()));
		showMenubarToggle.setSelected(c.getProperty(getClass(), "showMenuBar", showMenubarToggle.isSelected()));
		showToolbarToggle.setSelected(c.getProperty(getClass(), "showToolBar", showToolbarToggle.isSelected()));
		useExclusiveFullscreen.setSelected(c.getProperty(getClass(), "useExclusiveFullscreen", useExclusiveFullscreen.isSelected()));
		resetTextureChecker.setSelected(c.getProperty(getClass(), "resetTextureTransfom", true));
		super.restoreStates(c);
	}
	

	public Icon getMainIcon() {
		return null;
	}

	public String getMainName() {
		return "jReality Properties";
	}

	public JPanel getMainPage() {
		return mainPage;
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
	
	public int getColorPickerMode() {
		return colorChooserModeCombo.getSelectedIndex();
	}
	
	public boolean addColorPickerChangedListener(ColorPickerModeChangedListener l) {
		return colorModeListeners.add(l);
	}
	
	public boolean removeColorPickerChangedListener(ColorPickerModeChangedListener l) {
		return colorModeListeners.remove(l);
	}
	
	public boolean isResetTextureTransform() {
		return resetTextureChecker.isSelected();
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		viewMenuBar = c.getPlugin(ViewMenuBar.class);
		viewMenuBar.addMenuItem(getClass(), 10.1, showMenubarToggle.createMenuItem(), "Window");
		viewMenuBar.addMenuItem(getClass(), 10.2, showToolbarToggle.createMenuItem(), "Window");
		frontendListener.setShowMenuBar(showMenubarToggle.isSelected());
		frontendListener.setShowToolBar(showToolbarToggle.isSelected());
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		viewMenuBar.removeAll(getClass());
	}

	public void setFrontendListener(FrontendListener l) {
		frontendListener = l;
	}
	
	public boolean isExclusiveFullscreen() {
		return useExclusiveFullscreen.isSelected();
	}

}
