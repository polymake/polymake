package de.jreality.tutorial.viewer;

import java.awt.GridLayout;

import javax.swing.JDesktopPane;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;
import javax.swing.JRootPane;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.Inspector;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

/**
* Created on Jan 12, 2010
* @author goose12
* http://www3.math.tu-berlin.de/jreality/phpbb/viewtopic.php?f=3&t=48&p=1511#p1511
*/

public class AddViewerToInternalFrame {
	   public static void main(String[] args) {
		      JRViewer v = new JRViewer();
		      v.addBasicUI();
		      v.registerPlugin(new ContentLoader());
		      v.registerPlugin(new ContentTools());
		      v.getPlugin(Inspector.class).setInitialPosition(ShrinkPanelPlugin.SHRINKER_LEFT);
		      v.addContentSupport(ContentType.CenteredAndScaled);
		      v.setShowPanelSlots(true, false, false, false);

		      // call this to avoid creating a Frame
		      JRootPane rootPanel = v.startupLocal();
		      // create your own frame, with a JInternalFrame, and
		      // put the rootPanel into it.
		      JFrame f = new JFrame("Internal Frame Test");
		      JDesktopPane desktop = new JDesktopPane();
		      JInternalFrame internalFrame1 = new JInternalFrame("Internal Frame");
		      internalFrame1.setSize(600, 400);
		      internalFrame1.setLayout(new GridLayout());
		      internalFrame1.add(rootPanel);
		      internalFrame1.setResizable(true);
		      desktop.add(internalFrame1);
		      internalFrame1.setVisible(true);
		      f.setLayout(new GridLayout());
		      f.add(desktop);
		      f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		      f.setSize(800, 600);
		      f.setVisible(true);
		   }
}
