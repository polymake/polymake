package de.jreality.plugin.scene;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.HashMap;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.JRadioButton;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.Input;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;

/**
 * Example showing how to write a content providing plugin. Requires vrExamples.jar in the classpath, can
 * be found at www.jreality.de in the Download section.
 * 
 * @author weissman
 *
 */
public class VRExamples extends SceneShrinkPanel {

	String[][] examples = new String[][] {
			{ "Boy surface", "jrs/boy.jrs" },
			{ "Chen-Gackstatter surface", "obj/Chen-Gackstatter-4.obj" },
			{ "Helicoid with 2 handles", "jrs/He2WithBoundary.jrs" },
			{ "Tetranoid", "jrs/tetranoid.jrs" },
			{ "Wente torus", "jrs/wente.jrs" },
			{ "Schwarz P", "jrs/schwarz.jrs" },
			{ "Matheon bear", "jrs/baer.jrs" }
	};
	private HashMap<String, Integer> exampleIndices = new HashMap<String, Integer>();
	private Content content;
	private ContentLoader contentLoader = null;
	private JRadioButton	
		customContentButton = new JRadioButton("Load custom content...");

	private void makePanel() {
		if (examples != null) {
			ActionListener examplesListener = new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					String selectedBox = e.getActionCommand();
					int selectionIndex = ((Integer) exampleIndices.get(selectedBox)).intValue();
					try {
						SceneGraphComponent read = Readers.read(Input
								.getInput(examples[selectionIndex][1]));
						
						// The examples are aligned with z-axis pointing upwards. So we
						// rotate each example about the x-axis by -90 degrees.
						MatrixBuilder mb = MatrixBuilder.euclidean().rotateX(-Math.PI/2);
						if (read.getTransformation() != null) mb.times(read.getTransformation().getMatrix());
						mb.assignTo(read);
						
						getContent().setContent(read);
					} catch (IOException e1) {
						e1.printStackTrace();
					}
				}

			};
			
			JRadioButton first = null;
			Box buttonGroupPanel = new Box(BoxLayout.Y_AXIS);
			ButtonGroup group = new ButtonGroup();
			for (int i = 0; i < examples.length; i++) {
				JRadioButton button = new JRadioButton(examples[i][0]);
				if (first == null) first = button;
				button.addActionListener(examplesListener);
				buttonGroupPanel.add(button);
				group.add(button);
				exampleIndices.put(examples[i][0], new Integer(i));
			}
			buttonGroupPanel.add(customContentButton);
			group.add(customContentButton);
			customContentButton.setAction(contentLoader.getAction());
			shrinkPanel.setLayout(new GridLayout());
			shrinkPanel.add(buttonGroupPanel);
		}
	}
	
	private Content getContent() {
		return content;
	}
	
	@Override
	public void install(Controller c) throws Exception {
		content = JRViewerUtility.getContentPlugin(c);
		contentLoader = c.getPlugin(ContentLoader.class);
		makePanel();
		super.install(c);
	}
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("VR Examples", "jReality Group");
		info.icon = ImageHook.getIcon("bricks.png");
		return info;
	}

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addContentSupport(ContentType.TerrainAligned);
		v.registerPlugin(new ContentTools());
		ContentAppearance capp = new ContentAppearance();
		capp.getShrinkPanel().setShrinked(false);
		v.registerPlugin(capp);
		v.setShowPanelSlots(true, false, false, false);
		v.addVRSupport();
		v.registerPlugin(new VRExamples());
		v.startup();
	}
}
