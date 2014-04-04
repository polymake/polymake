package de.jreality.plugin.menu;

import static de.jreality.scene.Appearance.INHERITED;
import static de.jreality.shader.CommonAttributes.BACKGROUND_COLOR;
import static de.jreality.shader.CommonAttributes.BACKGROUND_COLORS;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.util.HashMap;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.ButtonGroup;
import javax.swing.ButtonModel;
import javax.swing.JMenu;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.UIManager;

import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.ShaderUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class BackgroundColor extends Plugin {

	public static Color[] defaultBackgroundColor = new Color[]{
		new Color(225, 225, 225), new Color(225, 225, 225),
		new Color(255, 225, 180), new Color(255, 225, 180), };

	private Scene scene;
	private JMenu menu;
	private ButtonGroup buttonGroup;
	private HashMap<String, ButtonModel> nameToButton = new  HashMap<String, ButtonModel>();
	private HashMap<String, Color[]> nameToColors = new  HashMap<String, Color[]>();
	
	private ViewMenuBar viewerMenu;

	public BackgroundColor() {

		menu = new JMenu("Background");
		menu.setIcon(ImageHook.getIcon("color_swatch.png"));
		buttonGroup = new ButtonGroup();

		addChoice(
				"Default",
				new Color[] {
						new Color(225, 225, 225), new Color(225, 225, 225),
						new Color(255, 225, 180), new Color(255, 225, 180)
				}
		);
		addChoice("White", Color.white);
		addChoice("Gray", new Color(225, 225, 225));
		addChoice("Black", Color.black);
		addChoice("Transparent Black", new Color(0,0,0,0));
		addChoice("UI Background", Color.black);

		setColor("UI Background");
	}

	public String getColor() {
		return buttonGroup.getSelection().getActionCommand();
	}
	
	/**
	 * Sets the scene root's background color.
	 * @param colors list of colors with length = 1 or 4
	 */
	public void setColor(String name) {
		if (!nameToColors.containsKey(name)) {
			System.err.println("Color name not registered: " + name);
			return;
		}
		nameToButton.get(name).setSelected(true);
		if (scene != null) {
			Color[] colors = nameToColors.get(name);
			if (name.equals("UI Background")) {
				colors = new Color[] {UIManager.getColor("Panel.background")};
			}
			if (colors == null || (colors.length!=1 && colors.length!=4)) {
				throw new IllegalArgumentException("illegal length of colors[]");
			}
			SceneGraphComponent root = scene.getSceneRoot();
			Appearance app = root.getAppearance();
			if (app == null) {
				app = new Appearance("root appearance");
				ShaderUtility.createRootAppearance(app);
				root.setAppearance(app);
			}

			//trim colors[] if it contains the same 4 colors
			if (colors.length == 4) {
				boolean equal = true;
				for (int i = 1; i < colors.length; i++)
					if (colors[i] != colors[0]) equal = false;
				if (equal) colors = new Color[]{ colors[0] };
			}

			app.setAttribute(BACKGROUND_COLOR, (colors.length==1)? colors[0] : INHERITED);
			app.setAttribute(BACKGROUND_COLORS, (colors.length==4)? colors : INHERITED); 
		}
	}

	public JMenu getMenu() {
		return menu;
	}

	@SuppressWarnings("serial")
	public void addChoice(final String name, final Color ... colors) {
		nameToColors.put(name, colors);
		Action action = new AbstractAction(name) {

			public void actionPerformed(ActionEvent e) {
				setColor(name);
			}

		};
		JRadioButtonMenuItem item = new JRadioButtonMenuItem(action);
		item.getModel().setActionCommand(name);
		nameToButton.put(name, item.getModel());
		buttonGroup.add(item);
		menu.add(item);
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Viewer Background";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("arrow.png");
		return info;
	}

	public void install(Scene scene) {
		this.scene = scene;
		setColor(getColor());
	}

	@Override
	public void install(Controller c) throws Exception {
		install(c.getPlugin(Scene.class));
		viewerMenu = c.getPlugin(ViewMenuBar.class);
		viewerMenu.addMenuItem(
				getClass(),
				10.0,
				getMenu(),
				"Viewer"
		);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		viewerMenu.removeAll(getClass());
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setColor(c.getProperty(getClass(), "color", getColor()));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "color", getColor());
		super.storeStates(c);
	}
}
