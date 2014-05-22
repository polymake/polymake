package de.tuberlin.polymake.common.jreality;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;

import javax.swing.JLabel;
import javax.swing.JPanel;

import de.jreality.plugin.basic.View;
import de.jreality.scene.SceneGraphComponent;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;
import de.tuberlin.polymake.common.ui.PolymakeSlider;
import de.tuberlin.polymake.common.ui.SliderEvent;
import de.tuberlin.polymake.common.ui.SliderListener;

public class ExplodePlugin extends ShrinkPanelPlugin implements SliderListener {

	protected PolymakeSlider 
		explodeSlider = new PolymakeSlider("Explode", 0.0, 10.0, 0.0);
	
	private SceneGraphComponent sgc;

	private JPanel panel = new JPanel();

	public ExplodePlugin() {
		panel .setLayout(new GridBagLayout());
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
		
		explodeSlider.addSliderListener(this);
		explodeSlider.getComponent().setVisible(true);
		panel.add(new JLabel("Explode Group of Geometries"),gbc2);
		panel.add(explodeSlider.getComponent(),gbc2);
		shrinkPanel.add(panel);
	}
	
	public void setSceneGraphComponent(SceneGraphComponent sgc) {
		this.sgc = sgc;
	}
	
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Explode","polymake");
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	public void sliderValueChanged(SliderEvent event) {
		Object source = event.getSource();
		if (source == explodeSlider) {
			double val = explodeSlider.getDoubleValue()/2.0;
			Utils.explodeGeometry(sgc, val);
		}

	}

}
