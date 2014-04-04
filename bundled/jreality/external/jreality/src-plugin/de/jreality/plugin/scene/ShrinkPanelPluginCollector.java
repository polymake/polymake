package de.jreality.plugin.scene;

import static java.awt.GridBagConstraints.BOTH;
import static java.awt.GridBagConstraints.REMAINDER;
import static java.awt.GridBagConstraints.WEST;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.util.Arrays;
import java.util.List;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

/** <p>Collect {@link ShrinkPanelPlugin}s into one {@link SceneShrinkPanel}. The shrink panels of the collection are put together in 
 * one shrink panel and properties saving is delegated to the shrink panels in this collection.
 * 
 * <p>You need to call {@link #setShrinkPanelPlugins(ShrinkPanelPlugin...)} before you use this plugin.
 * 
 * @author G. Paul Peters, 16.06.2010
 *
 */
public class ShrinkPanelPluginCollector extends SceneShrinkPanel {
	private List<ShrinkPanelPlugin> plugins;
	
	private static final GridBagConstraints gbConstraints = new GridBagConstraints();
	static {
		gbConstraints.fill = BOTH;
		gbConstraints.weightx = 1.0;
		gbConstraints.weighty = 1.0;
		gbConstraints.gridwidth = REMAINDER;
		gbConstraints.anchor = WEST;
	}
	
	/** Register the collection of plugins in this collection. Needs to be called before the collection plugin is used.
	 * 
	 * @param plugins
	 */
	public void setShrinkPanelPlugins(ShrinkPanelPlugin... plugins) {
		this.plugins = Arrays.asList(plugins);
	}
	
	@Override
	public void install(Controller c) throws Exception {
		checkPluginsNotEmpty();
		super.install(c);
		for (ShrinkPanelPlugin plugin : plugins) {
			plugin.install(c);
			ShrinkPanel shrinkPanel = plugin.getShrinkPanel();
			shrinkPanel.getParentSlot().removeShrinkPanel(shrinkPanel);
			shrinkPanel.setFloatable(false);
		}
		initPanel();
	}


	private void initPanel() {
		shrinkPanel.setLayout(new GridBagLayout());
		for (ShrinkPanelPlugin plugin : plugins) {
			shrinkPanel.add(plugin.getShrinkPanel(), gbConstraints);
		}
		shrinkPanel.revalidate();
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		checkPluginsNotEmpty();
		for (ShrinkPanelPlugin plugin : plugins) {
			plugin.uninstall(c);
		}
		super.uninstall(c);
	}

	@Override
	public void mainUIChanged(String uiClass) {
		checkPluginsNotEmpty();
		for (ShrinkPanelPlugin plugin : plugins) {
			plugin.mainUIChanged(uiClass);
		}
		super.mainUIChanged(uiClass);
	}

	@Override
	public void restoreStates(Controller c) throws Exception {
		checkPluginsNotEmpty();
		super.restoreStates(c);
		for (ShrinkPanelPlugin plugin : plugins) {
			plugin.restoreStates(c);
		}
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		checkPluginsNotEmpty();
		super.storeStates(c);
		for (ShrinkPanelPlugin plugin : plugins) {
			plugin.storeStates(c);
		}
	}
	
	private void checkPluginsNotEmpty() {
		if (plugins == null)
			throw new IllegalStateException("No plugins set.");
	}
	
	
}
