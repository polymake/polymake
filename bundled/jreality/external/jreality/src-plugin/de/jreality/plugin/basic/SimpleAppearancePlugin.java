package de.jreality.plugin.basic;

import java.awt.GridLayout;

import de.jreality.scene.Appearance;
import de.jreality.ui.SimpleAppearanceInspector;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class SimpleAppearancePlugin extends ShrinkPanelPlugin {

	private SimpleAppearanceInspector
		ai = new SimpleAppearanceInspector();
	
	public void setAppearance(Appearance a) {
		ai.setAppearance(a);
	}
	
	public SimpleAppearancePlugin(Appearance a) {
		ai.setAppearance(a);
		shrinkPanel.setLayout(new GridLayout());
		shrinkPanel.add(ai);
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

}
