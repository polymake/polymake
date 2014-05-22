package de.jreality.tutorial.viewer;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.InfoOverlayPlugin;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.plugin.scene.VRExamples;

/**
 * Show how to add a performance meter to a JRViewer application
 * @author gunn
 *
 */
public class PerformanceMeterExample {
	public static void main(String[] args) {
		// customize a JRViewer to have Virtual Reality support (skyboxes, terrain, etc)
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		VRExamples vrExamples = new VRExamples();
		vrExamples.getShrinkPanel().setShrinked(false);
		v.registerPlugin(vrExamples);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		// the following plugin displays performance statics (memory use, polygon count, FPS) 
		// when the JOGL backend is being used.
		v.registerPlugin(new InfoOverlayPlugin());
		v.startup();
	}

}
