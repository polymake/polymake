package de.jreality.plugin.experimental;

import java.awt.Component;

import de.jreality.plugin.basic.InfoOverlayPlugin;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Viewer;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;


@Experimental
public class ViewerKeyListenerPlugin extends Plugin {

	private View sceneView;
	private de.jreality.plugin.experimental.ViewerKeyListener vkl = null;
	private Component viewComp;

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "ViewerKeyListener";
		info.vendorName = "Charles Gunn";
		info.icon = ImageHook.getIcon("luperot.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		sceneView = c.getPlugin(View.class);
		viewComp = sceneView.getViewer().getViewingComponent();
		InfoOverlayPlugin iop = c.getPlugin(InfoOverlayPlugin.class);
		if (iop.getInfoOverlay() != null) {
			iop.getInfoOverlay().setVisible(false);
		}
		Viewer viewer = sceneView.getViewer().getCurrentViewer();
		vkl = new ViewerKeyListener(viewer, null, iop == null ? null : iop.getInfoOverlay());
		viewComp.addKeyListener(vkl);

	}

	@Override
	public void uninstall(Controller c) throws Exception {
		viewComp.removeKeyListener(vkl);
	}
	
	public ViewerKeyListener getViewerKeyListener() {
		return vkl;
	}
}
