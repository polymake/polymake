package de.jreality.plugin.basic;

import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;

import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.event.AncestorEvent;
import javax.swing.event.AncestorListener;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.plugin.icon.ImageHook;
import de.jtem.beans.StringEditor;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.PreferencesFlavor;
import de.jtem.jrworkspace.plugin.flavor.PropertiesFlavor;

/** This plugin provides access to the user preferences that control saving and loading of plugin properties. 
 * 
 * @author G. Paul Peters, Mar 10, 2010
 *
 */
public class PropertyPreferences extends Plugin implements PreferencesFlavor, PropertiesFlavor {

	private PropertiesListener propertiesListener;
	private JPanel mainPage;
	
	@Override
	public void install(Controller controller) throws Exception {
		super.install(controller);
		initMainPage();
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Property Preferences";
		info.vendorName = "Paul Peters";
		info.icon = ImageHook.getIcon("plugin.png");
		return info;
	}

	public void setPropertiesListener(PropertiesListener l) {
		propertiesListener = l;
	}

	public Icon getMainIcon() {
		return ImageHook.getIcon("plugin.png");
	}

	public String getMainName() {
		return "Properties Saving";
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

	private void initMainPage() {
		mainPage = new JPanel(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(10,10,10,10);
		c.anchor = GridBagConstraints.WEST;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.weightx = 1;
		
		JLabel help = new JLabel(
				"<html>The jReality viewer consists of plugins. Many of these<br>" +
				"plugins save their state at program exit and restore their<br>" +
				"state at startup. Below you may set four preferences that <br>" +
				"control whether and where to save and load these properties.<br>" +
				"These four settings or NOT saved in the file specified below,<br>" +
				"but they will be saved as user preferences of the application<br>" +
				"at a system dependent place.</html>");
		help.setFont(help.getFont().deriveFont(Font.PLAIN).deriveFont(8));
		mainPage.add(help, c);
		
		c.insets = new Insets(2,2,2,2);
		final JCheckBox askBeforeSaveOnExitCheckBox = new JCheckBox("Quiet exit", ! propertiesListener.isAskBeforeSaveOnExit());
		askBeforeSaveOnExitCheckBox.setToolTipText("<html>Apply the three preferences below at program exit<br>" +
				" instead of showing a dialog which allows to change these preferences.</html>");
		askBeforeSaveOnExitCheckBox.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				propertiesListener.setAskBeforeSaveOnExit(! ((JCheckBox) e.getSource()).isSelected());
			}
		});
		mainPage.add(askBeforeSaveOnExitCheckBox, c);
		
		final JCheckBox saveOnExitCheckBox = new JCheckBox("Save On Exit", propertiesListener.isSaveOnExit());
		saveOnExitCheckBox.setToolTipText("<html>Plugin properties are saved to the file specified below.<br>" +
				" This setting has no effect if \"Quiet exit\" is turned off.</html>");
		saveOnExitCheckBox.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				propertiesListener.setSaveOnExit(((JCheckBox) e.getSource()).isSelected());
			}
		});
		mainPage.add(saveOnExitCheckBox, c);
		
		final JCheckBox loadFromUserPropertyFileCheckBox = new JCheckBox("Load At Startup", propertiesListener.isLoadFromUserPropertyFile());
		loadFromUserPropertyFileCheckBox.setToolTipText("At next startup plugin properties will be loaded from the file specified below.");
		loadFromUserPropertyFileCheckBox.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				propertiesListener.setLoadFromUserPropertyFile(((JCheckBox) e.getSource()).isSelected());
			}
		});
		mainPage.add(loadFromUserPropertyFileCheckBox, c);

		c.gridwidth = 1;
		c.weightx = 0;
		mainPage.add(new JLabel("Properties File: "), c);
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.weightx = 1;
		final PropertyFileChooser fileChooser = new PropertyFileChooser();
		mainPage.add(fileChooser, c);
		fileChooser.setToolTipText("Choose a file to save the plugin properties to and load them from.");

		mainPage.addAncestorListener(new AncestorListener() {
			public void ancestorAdded(AncestorEvent event) {
				SwingUtilities.invokeLater(new Runnable() {
					public void run() {
						askBeforeSaveOnExitCheckBox.setSelected(! propertiesListener.isAskBeforeSaveOnExit());
						saveOnExitCheckBox.setSelected(propertiesListener.isSaveOnExit());
						loadFromUserPropertyFileCheckBox.setSelected(propertiesListener.isLoadFromUserPropertyFile());
						fileChooser.updateTextField();
					}
				});
			}
			public void ancestorMoved(AncestorEvent event) {
			}
			public void ancestorRemoved(AncestorEvent event) {
			}
		});
	}
	
	private class PropertyFileChooser extends JPanel {
		private static final long serialVersionUID = 1L;
		private StringEditor textField;
		private JButton fileChooserButton;
		private JFileChooser userPropertiesFileChooser = new JFileChooser();
		
		public PropertyFileChooser() {
			initTextFieldAndButton();
			
			setLayout(new GridBagLayout());
			GridBagConstraints c = new GridBagConstraints();
			c.insets = new Insets(0,2,0,2);
			c.anchor = GridBagConstraints.WEST;
			c.gridwidth = 1;
			c.weightx = 1;
			add(textField.getCustomEditor(), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			c.weightx = 0;
			add(fileChooserButton, c);
		}
		
		@Override
		public void setToolTipText(String text) {
			super.setToolTipText(text);
			((JComponent)textField.getCustomEditor()).setToolTipText(text);
			fileChooserButton.setToolTipText(text);
		}
		
		public void updateTextField() {
			textField.setAsText(propertiesListener.getUserPropertyFile() == null ? "" :  propertiesListener.getUserPropertyFile());
		}

		private void initTextFieldAndButton() {
			textField = new StringEditor();
			updateTextField();
			textField.addPropertyChangeListener(new PropertyChangeListener() {
				public void propertyChange(PropertyChangeEvent evt) {
					propertiesListener.setUserPropertyFile(textField.getAsText());
				}
			});
			textField.getCustomEditor().setPreferredSize(new Dimension(240,22));
			
			fileChooserButton = new JButton("...");
			fileChooserButton.setMargin(new Insets(0, 5, 0, 5));
			fileChooserButton.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					if (propertiesListener.getUserPropertyFile()!=null) {
						userPropertiesFileChooser.setSelectedFile(new File(textField.getAsText()));
					}

					int result = userPropertiesFileChooser.showDialog(SwingUtilities.getWindowAncestor(mainPage), "Select");
					if (result != JFileChooser.APPROVE_OPTION) {
						return;
					}
					String file = userPropertiesFileChooser.getSelectedFile().getPath();
					propertiesListener.setUserPropertyFile(file);
					textField.setAsText(file);
				}
			});
			
			userPropertiesFileChooser.setFileHidingEnabled(false);
		}
	}

}
