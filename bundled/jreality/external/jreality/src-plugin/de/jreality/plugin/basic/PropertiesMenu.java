package de.jreality.plugin.basic;

import static javax.swing.JFileChooser.FILES_ONLY;
import static javax.swing.JOptionPane.ERROR_MESSAGE;
import static javax.swing.JOptionPane.YES_NO_OPTION;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.Reader;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.JFileChooser;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.util.Secure;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.PropertiesFlavor;
import de.jtem.jrworkspace.plugin.simplecontroller.widget.SaveOnExitDialog.PropertiesFileFilter;

public class PropertiesMenu extends Plugin implements PropertiesFlavor {

	
	protected class WritePropertiesAction extends AbstractAction {

		private static final long 
			serialVersionUID = 1L;

		public WritePropertiesAction() {
			putValue(Action.NAME, "Save Properties");
			putValue(Action.SMALL_ICON, ImageHook.getIcon("diskette.png"));
		}
		
		
		public void actionPerformed(ActionEvent e) {
			Component parent = SwingUtilities.getWindowAncestor(view.getCenterComponent());
			int result = fileChooser.showSaveDialog(parent);
			if (result != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File f = fileChooser.getSelectedFile();
			if (f.exists()) {
				int owr = JOptionPane.showConfirmDialog(parent, "The file " + f.getName() + " exists. Overwrite?", "File Exists", YES_NO_OPTION);
				if (owr != JOptionPane.YES_OPTION) {
					return;
				}
			}
			try {
				FileWriter w = new FileWriter(f);
				propertiesListener.writeProperties(w);
			} catch (Exception ex) {
				JOptionPane.showMessageDialog(parent, "File could not be written: " + ex.getLocalizedMessage(), "Write error", ERROR_MESSAGE);
			}
		}
		
	}
	
	protected class LoadPropertiesAction extends AbstractAction {
		
		private static final long 
			serialVersionUID = 1L;

		public LoadPropertiesAction() {
			putValue(Action.NAME, "Load Properties");
			putValue(Action.SMALL_ICON, ImageHook.getIcon("folder.png"));
		}
		
		
		public void actionPerformed(ActionEvent e) {
			Component parent = SwingUtilities.getWindowAncestor(view.getCenterComponent());
			int result = fileChooser.showOpenDialog(parent);
			if (result != JFileChooser.APPROVE_OPTION) {
				return;
			}
			File f = fileChooser.getSelectedFile();
			try {
				FileReader r = new FileReader(f);
				propertiesListener.readProperties(r);
			} catch (Exception ex) {
				JOptionPane.showMessageDialog(parent, "File could not be read: " + ex.getLocalizedMessage(), "Read error", ERROR_MESSAGE);
			}
		}
		
	}
	

	protected class LoadDefaultPropertiesAction extends AbstractAction {
		
		private static final long 
			serialVersionUID = 1L;
		
		public LoadDefaultPropertiesAction() {
			putValue(Action.NAME, "Load Default Properties");
			putValue(Action.SMALL_ICON, ImageHook.getIcon("arrow_rotate_anticlockwise.png"));
		}
		
		
		public void actionPerformed(ActionEvent e) {
			Component parent = SwingUtilities.getWindowAncestor(view.getCenterComponent());
			int result = JOptionPane.showConfirmDialog(parent, "Do you really want to load the default properties?", "Confirm", YES_NO_OPTION);
			if (result != JOptionPane.YES_OPTION) {
				return;
			}
			propertiesListener.loadDefaultProperties();
		}
		
	}
	
	private ViewMenuBar
		viewMenuBar = null;
	private View
		view = null;
	private PropertiesListener
		propertiesListener = null;
	private Action
		writeAction = new WritePropertiesAction(),
		loadAction = new LoadPropertiesAction(),
		loadDefaultAction = new LoadDefaultPropertiesAction();
	private final JFileChooser
		fileChooser = new JFileChooser();
	
	public PropertiesMenu() {
		String dir = Secure.getProperty("user.dir", "");
		fileChooser.setCurrentDirectory(new File(dir));
		fileChooser.addChoosableFileFilter(new PropertiesFileFilter());
		fileChooser.setAcceptAllFileFilterUsed(false);
		fileChooser.setFileHidingEnabled(false);
		fileChooser.setFileSelectionMode(FILES_ONLY);
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		view = c.getPlugin(View.class);
		viewMenuBar = c.getPlugin(ViewMenuBar.class);
		
		JMenu propertiesMenu = new JMenu("Properties");
		propertiesMenu.setMnemonic('P');
		viewMenuBar.addMenu(getClass(), 200, propertiesMenu);
		
		JMenuItem writeMenu = new JMenuItem(writeAction);
		writeMenu.setMnemonic('S');
		viewMenuBar.addMenuItem(getClass(), 1, writeMenu, "Properties");
		
		JMenuItem loadMenu = new JMenuItem(loadAction);
		loadMenu.setMnemonic('L');
		viewMenuBar.addMenuItem(getClass(), 2, loadMenu, "Properties");
		viewMenuBar.addMenuSeparator(getClass(), 2.5, "Properties");

		
		JMenuItem defaultMenu = new JMenuItem(loadDefaultAction);
		defaultMenu.setMnemonic('D');
		viewMenuBar.addMenuItem(getClass(), 3, defaultMenu, "Properties");
		
		c.getPlugin(PropertyPreferences.class);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		viewMenuBar.removeAll(getClass());
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Property Menu Items", "jReality Group");
	}

	public void setPropertiesListener(PropertiesListener l) {
		propertiesListener = l;
	}

	public boolean loadPreferences(Reader r) {
		if (propertiesListener == null) return false;
		propertiesListener.readProperties(r);
		return true;
	}
	
	
	
}
