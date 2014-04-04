package de.jreality.plugin.basic;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.scene.SceneShrinkPanel;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

/** Use this class to get a  {@link ShrinkPanelPlugin} that will be added to the 
 * main {@link SideContainerPerspective} of {@link JRViewer} , i.e., {@link View}. Extend this class
 * and add it to the JRViewer with {@link JRViewer#registerPlugin()};
 * 
 * @author G. Paul Peters, 22.07.2009
 * 
 * @see SceneShrinkPanel
 *
 */
abstract public class ViewShrinkPanelPlugin extends ShrinkPanelPlugin {
	
	public ViewShrinkPanelPlugin() {
		super();
	}
	
	
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	

}
