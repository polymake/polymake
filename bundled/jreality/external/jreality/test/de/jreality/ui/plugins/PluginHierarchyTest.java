package de.jreality.ui.plugins;

import de.jreality.plugin.basic.ToolSystemPlugin;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.scene.Avatar;
import de.jreality.plugin.scene.Lights;
import de.jreality.plugin.scene.Terrain;
import de.jtem.jrworkspace.plugin.simplecontroller.SimpleController;

public class PluginHierarchyTest {

	public static void main(String[] args) {
		SimpleController c = new SimpleController();
		c.registerPlugin(new ToolSystemPlugin());
		c.registerPlugin(new View());
		c.registerPlugin(new ContentLoader());
		c.registerPlugin(new Terrain());
		c.registerPlugin(new Avatar());
		c.registerPlugin(new Lights());
		c.startup();
	}

}
