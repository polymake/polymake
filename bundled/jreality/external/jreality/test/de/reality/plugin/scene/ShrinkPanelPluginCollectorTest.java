package de.reality.plugin.scene;

import static org.junit.Assert.assertEquals;

import java.util.List;
import java.util.logging.Level;

import org.junit.Before;
import org.junit.Test;

import de.jreality.junitutils.GuiTestUtils;
import de.jreality.junitutils.GuiTestUtils.ComponentsFinder;
import de.jreality.junitutils.PropertyVault;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.scene.ShrinkPanelPluginCollector;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;
import de.jtem.jrworkspace.plugin.simplecontroller.SimpleController;

public class ShrinkPanelPluginCollectorTest {
	private ShrinkPanelPluginCollector pluginCollector;
	MockShrinkPanelPlugin1 plugin1 = new MockShrinkPanelPlugin1();
	MockShrinkPanelPlugin2 plugin2 = new MockShrinkPanelPlugin2();
	
	@Before
	public void initPluginCollector() throws Exception {
		de.jtem.jrworkspace.logging.LoggingSystem.LOGGER.setLevel(Level.OFF);
		pluginCollector = new ShrinkPanelPluginCollector();
		pluginCollector.setShrinkPanelPlugins(plugin1, plugin2);
		SimpleController controller = new SimpleController();
		controller.registerPlugin(pluginCollector);
		controller.startupLocal();
	}
	
	@Test
	public void testShrinkPanelContent() {
		ComponentsFinder<ShrinkPanel> shrinkPanelFinder = new GuiTestUtils.ComponentsFinder<ShrinkPanel>(ShrinkPanel.class);
		List<ShrinkPanel> shrinkPanels = shrinkPanelFinder.getComponents(pluginCollector.getShrinkPanel());
		assertEquals(2, shrinkPanels.size());
	}
	
	@Test
	public void testPropertyMockInitValue() {
		assertEquals("default", plugin1.property);
		assertEquals("default", plugin2.property);
	}
	
	@Test
	public void testDefaultProperty() throws Exception {
		PropertyVault propertyVault = new PropertyVault();
		plugin1.property = "";
		plugin2.property = "";

		pluginCollector.restoreStates(propertyVault);

		assertEquals("default", plugin1.property);
		assertEquals("default", plugin2.property);
	}
	
	@Test
	public void testPropertySavingAndRestoring() throws Exception {
		plugin1.property = "Hallo1";
		plugin2.property = "Hallo2";
		PropertyVault propertyVault = new PropertyVault();

		pluginCollector.storeStates(propertyVault);
		pluginCollector.restoreStates(propertyVault);
		
		assertEquals("Hallo1", plugin1.property);
		assertEquals("Hallo2", plugin2.property);
	}
	
	class MockShrinkPanelPlugin1 extends ShrinkPanelPlugin {
		String property = "init";
		
		@Override
		public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
			return View.class;
		}

		@Override
		public PluginInfo getPluginInfo() {
			PluginInfo info = new PluginInfo("Mock Plugin 1");
			return info;
		}

		@Override
		public void restoreStates(Controller c) throws Exception {
			super.restoreStates(c);
			property = c.getProperty(this.getClass(), "test1", "default");
		}

		@Override
		public void storeStates(Controller c) throws Exception {
			super.storeStates(c);
			c.storeProperty(this.getClass(), "test1", property);		
		}
		
		
	}

	class MockShrinkPanelPlugin2 extends ShrinkPanelPlugin {
		String property = "init";

		@Override
		public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
			return View.class;
		}
	
		@Override
		public PluginInfo getPluginInfo() {
			PluginInfo info = new PluginInfo("Mock Plugin 2");
			return info;
		}
		

		@Override
		public void restoreStates(Controller c) throws Exception {
			super.restoreStates(c);
			property = c.getProperty(this.getClass(), "test2", "default");
		}

		@Override
		public void storeStates(Controller c) throws Exception {
			super.storeStates(c);
			c.storeProperty(this.getClass(), "test2", property);
		}
	}
}
