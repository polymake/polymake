package de.jreality.tutorial.plugin;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JComboBox;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.PropertiesMenu;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class Persistence extends ViewShrinkPanelPlugin {
	public enum Geometries {
		Icosahedron(Primitives.icosahedron()),
		ColoredCube(Primitives.coloredCube()),
		Tetrahedron(Primitives.tetrahedron()),
		Torus(Primitives.torus(Math.sqrt(2), 1, 20, 20));
		
		public final Geometry geometry;
		Geometries(Geometry geometry) {	this.geometry=geometry;}
	}

	private final JComboBox comboBox=new JComboBox(Geometries.values());
	private final SceneGraphComponent sgc=new SceneGraphComponent("A Geometry");

	public Persistence() {
		setInitialPosition(SHRINKER_LEFT);
		getShrinkPanel().add(comboBox);
		
		comboBox.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				sgc.setGeometry(
						((Geometries)comboBox.getSelectedItem()).geometry);
				}
		});
	}
	
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Primitive Chooser");
	}
	
	public void install(Controller c) throws Exception {
		super.install(c);
		c.getPlugin(Scene.class)
			.getContentComponent()
			.addChild(sgc);
	}

	public void uninstall(Controller c) throws Exception {
		c.getPlugin(Scene.class)
			.getContentComponent()
		    .removeChild(sgc);
		super.uninstall(c);
	}
	
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);
		comboBox.setSelectedItem(
				c.getProperty(getClass(), "geometry", Geometries.values()[0]));
	}
	
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		c.storeProperty(getClass(), "geometry", (Geometries) comboBox.getSelectedItem());
	}

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.registerPlugin(new Persistence());
		v.registerPlugin(new ContentTools());
		v.registerPlugin(new PropertiesMenu());
		v.setPropertiesResource(Persistence.class, "PersistenceProperties.xml");
		v.setShowPanelSlots(true, false, false, false);
		v.startup();
	}
}
