package de.jreality.plugin.experimental;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;

import javax.swing.DefaultComboBoxModel;
import javax.swing.Icon;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JList;
import javax.swing.plaf.basic.BasicComboBoxRenderer;

import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.SceneGraphComponent;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;

@Experimental
public class ManagedContentGUI extends Plugin implements ActionListener {

	private ManagedContent
		managedContent = null;
	private ViewToolBar
		viewToolBar = null;
	private JComboBox
		layerCombo = new JComboBox();
	private ContextComparator
		contextComparator = new ContextComparator();
	private Icon
		layersIcon = ImageHook.getIcon("layers.png");
	
	
	public ManagedContentGUI() {
		layerCombo.setRenderer(new LayerComboRenderer());
		layerCombo.setModel(new LayerModel());
		layerCombo.addActionListener(this);
		layerCombo.setPrototypeDisplayValue(LayerComboRenderer.class);
	}
	
	public void actionPerformed(ActionEvent e) {
		 if (e.getSource() == layerCombo) {
			 if (!(layerCombo.getSelectedItem() instanceof Class<?>)) {
				 return;
			 }
			 Class<?> context = (Class<?>)layerCombo.getSelectedItem();
			 if (context == LayerComboRenderer.class) {
				 return;
			 }
			 SceneGraphComponent cgc = managedContent.getContextRoot(context);
			 cgc.setVisible(!cgc.isVisible());
			 layerCombo.setSelectedItem(null);
		 }
	}
	
	
	private class ContextComparator implements Comparator<Class<?>> {
		
		public int compare(Class<?> o1, Class<?> o2) {
			return o1.getSimpleName().compareTo(o2.getSimpleName());
		}
		
	}
	
	
	private class LayerModel extends DefaultComboBoxModel {
		
		private static final long 
			serialVersionUID = 1L;

		@Override
		public Object getElementAt(int index) {
			Set<Class<?>> contextSet = managedContent.getContextSet();
			List<Class<?>> contextList = new ArrayList<Class<?>>(contextSet);
			Collections.sort(contextList, contextComparator);
			return contextList.get(index);
		}
		
		@Override
		public int getSize() {
			Set<Class<?>> contextSet = managedContent.getContextSet();
			return contextSet.size();
		}
		
	}
	
	
	private class LayerComboRenderer extends BasicComboBoxRenderer {
		
		private static final long 
			serialVersionUID = 1L;
		private JCheckBox
			layerChecker = new JCheckBox();
		
		public LayerComboRenderer() {
			setText("Content Layers");
		}
		
		@Override
		public Component getListCellRendererComponent(JList list, Object value,
				int index, boolean isSelected, boolean cellHasFocus) {
			super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
			if (value == null) {
				setText("Context Layers");
				setIcon(layersIcon);
				return this;
			}
			Class<?> context = (Class<?>)value;
			if (context == LayerComboRenderer.class) {
				setText("---------------------------------");
				return this;
			}
			layerChecker.setText(context.getSimpleName());
			SceneGraphComponent cgc = managedContent.getContextRoot(context);
			layerChecker.setSelected(cgc.isVisible());
			layerChecker.setBackground(getBackground());
			layerChecker.setForeground(getForeground());
			return layerChecker;
		}
		
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		managedContent = c.getPlugin(ManagedContent.class);
		viewToolBar = c.getPlugin(ViewToolBar.class);
		viewToolBar.addTool(getClass(), 1.0, layerCombo);
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		viewToolBar.removeAll(getClass());
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Managed Content GUI", "Stefan Sechelmann");
	}
	
}
