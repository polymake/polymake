package de.jreality.plugin.scene;

import java.awt.Component;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.swing.JComponent;
import javax.swing.JPanel;

import de.jreality.plugin.basic.MainPanel;


/** Collects many <code>JComponent</code>s in one <code>SceneShrinkPanel</code> plugin. 
 * Subclass this plugin to get your own aggregator, e.g., see {@link MainPanel}.
 *
 */
public abstract class ShrinkPanelAggregator extends SceneShrinkPanel {

	protected Map<Class<?>, Set<MainPanelContent>>
		contentMap = new HashMap<Class<?>, Set<MainPanelContent>>();
	protected GridBagConstraints
		contraints = new GridBagConstraints();
	
	public ShrinkPanelAggregator() {
		shrinkPanel.setLayout(new GridBagLayout());
		contraints.fill = GridBagConstraints.HORIZONTAL;
		contraints.weightx = 1.0;
		contraints.insets = new Insets(1,0,1,0);
		contraints.gridwidth = GridBagConstraints.REMAINDER;
	}
	
	protected class MainPanelContent {
		
		public Component 
			comp = null;
		public double 
			priority = 0.0;
		public String 
			section = "Default";
		
		public MainPanelContent(Component c, double p, String section) {
			this.comp = c;
			this.priority = p;
			this.section = section;
		}
		
	}
	
	
	private Set<MainPanelContent> getContextSet(Class<?> context) {
		if (!contentMap.containsKey(context)) {
			contentMap.put(context, new HashSet<MainPanelContent>());
		}
		return contentMap.get(context);
	}
	
	
	public void addComponent(Class<?> context, JComponent c, double priority, String section) {
		Set<MainPanelContent> cSet = getContextSet(context);
		cSet.add(new MainPanelContent(c, priority, section));
		updateLayout();
	}
	
	public void removeComponent(Class<?> context, JComponent c) {
		if (!contentMap.containsKey(context)) {
			return;
		}
		Set<MainPanelContent> cSet = contentMap.get(context);
		MainPanelContent removeContent = null;
		for (MainPanelContent content : cSet) {
			if (content.comp == c) {
				removeContent = content;
				break;
			}
		}
		if (removeContent != null) {
			cSet.remove(removeContent);
			updateLayout();
		}
	}
	
	public void removeAll(Class<?> context) {
		Set<MainPanelContent> cSet = contentMap.remove(context);
		if (cSet != null) {
			updateLayout();
		}
	}
	
	
	protected void updateLayout() {
		shrinkPanel.removeAll();
		HashMap<String, Set<MainPanelContent>> sectionMap = new HashMap<String, Set<MainPanelContent>>();
		for (Set<MainPanelContent> cSet : contentMap.values()) {
			for (MainPanelContent c : cSet) {
				if (!sectionMap.containsKey(c.section)) {
					sectionMap.put(c.section, new HashSet<MainPanelContent>());
				}
				sectionMap.get(c.section).add(c);
			}
		}
		List<String> sectionList = new LinkedList<String>(sectionMap.keySet());
		Collections.sort(sectionList);
		for (String section : sectionList) {
			Set<MainPanelContent> sectionSet = sectionMap.get(section);
			List<MainPanelContent> sortedSection = new LinkedList<MainPanelContent>(sectionSet);
			Collections.sort(sortedSection, new ContentPriorityComparator());
			JPanel panel = new JPanel();
			panel.setLayout(new GridBagLayout());
			for (MainPanelContent content : sortedSection) {
				panel.add(content.comp, contraints);
			}
			shrinkPanel.add(panel, contraints);
		}
		shrinkPanel.revalidate();
	}
	
	
	protected class ContentPriorityComparator implements Comparator<MainPanelContent> {

		public int compare(MainPanelContent o1, MainPanelContent o2) {
			return o1.priority < o2.priority ? -1 : 1;
		}
		
	}
	

}
