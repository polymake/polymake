package de.tuberlin.polymake.common.jreality;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JPanel;

import de.jreality.plugin.basic.View;
import de.jreality.scene.SceneGraphComponent;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class SplitGeometriesPlugin extends ShrinkPanelPlugin implements ActionListener {

		
	private JButton splitButton = new JButton("Split");
	
	private SceneGraphComponent geom = null;

	private JPanel panel = new JPanel();

	public SplitGeometriesPlugin(SceneGraphComponent geom) {
		this.geom = geom;
		panel.setLayout(new GridBagLayout());
		panel.setPreferredSize(new Dimension(150, 40));
		GridBagConstraints gbc1 = new GridBagConstraints();
		gbc1.fill = GridBagConstraints.BOTH;
		gbc1.weightx = 1.0;
		gbc1.gridwidth = GridBagConstraints.RELATIVE;
		gbc1.insets = new Insets(2, 2, 2, 2);
		
		GridBagConstraints gbc2 = new GridBagConstraints();
		gbc2.fill = GridBagConstraints.BOTH;
		gbc2.weightx = 1.0;
		gbc2.gridwidth = GridBagConstraints.REMAINDER;
		gbc2.insets = new Insets(2, 2, 2, 2);
		
		splitButton.addActionListener(this);
		panel.add(splitButton,gbc2);
		shrinkPanel.add(panel);
	}
	
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Split geometry","polymake");
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	@Override
	public void actionPerformed(ActionEvent arg0) {
		Utils.splitAllIndexedFaceSetsGeometry(geom);
	}
	
}
