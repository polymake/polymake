package de.jreality.ui.plugins;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;

public class ViewerTest {

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.setContent(Primitives.coloredCube());
		v.startup();
	}

}
