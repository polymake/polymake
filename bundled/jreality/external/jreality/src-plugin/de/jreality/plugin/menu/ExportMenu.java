package de.jreality.plugin.menu;

import java.awt.Component;

import javax.swing.JMenu;
import javax.swing.SwingUtilities;

import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.ui.viewerapp.SunflowMenu;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.ui.viewerapp.actions.file.ExportImage;
import de.jreality.ui.viewerapp.actions.file.ExportOBJ;
import de.jreality.ui.viewerapp.actions.file.ExportPDF;
import de.jreality.ui.viewerapp.actions.file.ExportPS;
import de.jreality.ui.viewerapp.actions.file.ExportSTL;
import de.jreality.ui.viewerapp.actions.file.ExportSVG;
import de.jreality.ui.viewerapp.actions.file.ExportScreenshot;
import de.jreality.ui.viewerapp.actions.file.ExportU3D;
import de.jreality.ui.viewerapp.actions.file.SaveScene;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.UIFlavor;

public class ExportMenu extends Plugin implements UIFlavor {
	
	private ViewMenuBar viewMenuBar;
	private JMenu 
		exportMenu = new JMenu("Export");
	private ExportPDF
		exportPDF = null;
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Export";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("arrow.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		viewMenuBar = c.getPlugin(ViewMenuBar.class);
		View view = c.getPlugin(View.class);
		ViewerSwitch viewer = view.getViewer();
		Component parent = viewer.getViewingComponent();
		exportMenu.setIcon(ImageHook.getIcon("disk.png"));
		exportMenu.add(new ExportImage("Image", viewer, parent));
		exportMenu.add(new ExportScreenshot("Screenshot", viewer, parent));
		exportMenu.add(new ExportSVG("SVG", viewer, parent));
		exportMenu.add(new ExportPS("PS", viewer, parent));
		exportMenu.add(new ExportSTL("STL", viewer, parent));
		exportMenu.add(new ExportOBJ("OBJ", viewer, parent));
		exportMenu.add(new ExportU3D("U3D", viewer, parent));
		exportMenu.add(exportPDF = new ExportPDF("PDF", viewer, parent));
		exportMenu.add(new SunflowMenu(viewer));
		SaveScene saveSceneAction = new SaveScene("Save Scene", viewer, parent);
		saveSceneAction.setIcon(ImageHook.getIcon("disk.png"));
		viewMenuBar.addMenuItem(getClass(), 1, saveSceneAction, "File");
		viewMenuBar.addMenuItem(getClass(), 2, exportMenu, "File");
	}
	
	
	public void mainUIChanged(String uiClass) {
		SwingUtilities.updateComponentTreeUI(exportPDF.getAccessory());
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		viewMenuBar.removeMenu(getClass(), exportMenu);
		super.uninstall(c);
	}

}
