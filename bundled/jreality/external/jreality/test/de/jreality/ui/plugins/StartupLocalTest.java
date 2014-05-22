package de.jreality.ui.plugins;

import java.awt.GridLayout;

import javax.swing.JFrame;
import javax.swing.JRootPane;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;

public class StartupLocalTest {

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentSupport(ContentType.CenteredAndScaled);
		v.setContent(Primitives.sharedIcosahedron);
		JRootPane p = v.startupLocal();
		
		JFrame f = new JFrame();
		f.setSize(800, 600);
		f.setLayout(new GridLayout());
		f.add(p);
		f.setVisible(true);
	}

}
