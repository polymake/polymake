package de.jreality.plugin.basic;

import de.jreality.jogl.InstrumentedViewer;
import de.jreality.jogl.JOGLViewer;
import de.jreality.jogl.plugin.InfoOverlay;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.Viewer;
import de.jreality.util.LoggingSystem;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;

@Experimental
public class InfoOverlayPlugin extends Plugin {
	
	private View sceneView;
	private InfoOverlay infoOverlay;
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "InfoOverlay";
		info.vendorName = "Charles Gunn";
		info.icon = ImageHook.getIcon("luperot.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		sceneView = c.getPlugin(View.class);
//		Component viewComp = sceneView.getViewer().getViewingComponent();
		Viewer[] vlist = sceneView.getViewer().getViewers();
		InstrumentedViewer joglViewer = null;
		// this is unreliable; adding a DomeViewer (subclass of JOGLViewer) also
		// broke original version (without break;)
	
		for (Viewer v : vlist)	{
			if (v instanceof InstrumentedViewer) {
				joglViewer = (InstrumentedViewer) v;
				joglViewer.installOverlay();
			}
		}
		
		for (Viewer v : vlist)	{
			if (v instanceof JOGLViewer) {
				joglViewer = (InstrumentedViewer) v;
			}
		}
		if (joglViewer == null)  { // signal error 
			LoggingSystem.getLogger(this).warning("No Jogl Viewer in viewer switch!");
			return;
		}
		infoOverlay = InfoOverlay.perfInfoOverlayFor();
		infoOverlay.setInstrumentedViewer(joglViewer);
		infoOverlay.setVisible(true);
	}
	
	@Override
	public void uninstall(Controller c) throws Exception {
//		SceneGraphComponent root = sceneView.getSceneRoot();
		infoOverlay.setVisible(false);
		
		sceneView = c.getPlugin(View.class);
//		Component viewComp = sceneView.getViewer().getViewingComponent();
		Viewer[] vlist = sceneView.getViewer().getViewers();
		InstrumentedViewer joglViewer = null;
		// this is unreliable; adding a DomeViewer (subclass of JOGLViewer) also
		// broke original version (without break;)
		for (Viewer v : vlist)	{
			if (v instanceof InstrumentedViewer) {
				joglViewer = (InstrumentedViewer) v;
				joglViewer.uninstallOverlay();
			}
		}
	}
	
	public InfoOverlay getInfoOverlay() {
		return infoOverlay;
	}
}
