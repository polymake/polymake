package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.security.PrivilegedAction;
import java.util.HashMap;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.border.EmptyBorder;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.reader.Readers;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.data.Attribute;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.util.Input;
import de.jreality.util.Secure;

public class LoadPluginVR extends AbstractPluginVR {

	// load tab
	private JPanel loadPanel;
	private JPanel fileChooserPanel;

	private HashMap<String, Integer> exampleIndices = new HashMap<String, Integer>();

	public LoadPluginVR(final String[][] examples) {
		super("load");
		Secure.doPrivileged(new PrivilegedAction<Object>() {
			public Object run() {
				makeContentFileChooser();
				return null;
			}
		});
		loadPanel = new JPanel(new BorderLayout());
		loadPanel.setBorder(new EmptyBorder(10, 10, 10, 10));
		
		if (examples != null) {
			ActionListener examplesListener = new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					String selectedBox = e.getActionCommand();
					int selectionIndex = ((Integer) exampleIndices.get(selectedBox)).intValue();
					try {
						SceneGraphComponent read = Readers.read(Input
								.getInput(examples[selectionIndex][1]));
						getViewerVR().setContent(read);
					} catch (IOException e1) {
						e1.printStackTrace();
					}
				}
			};
			
			Box buttonGroupPanel = new Box(BoxLayout.Y_AXIS);
			ButtonGroup group = new ButtonGroup();
			for (int i = 0; i < examples.length; i++) {
				JRadioButton button = new JRadioButton(examples[i][0]);
				button.addActionListener(examplesListener);
				buttonGroupPanel.add(button);
				group.add(button);
				exampleIndices.put(examples[i][0], new Integer(i));
			}
			loadPanel.add("Center", buttonGroupPanel);
		}
		JButton loadButton = new JButton("load ...");
		loadButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				switchToFileBrowser();
			}
		});
		loadPanel.add("South", loadButton);
	}

	@Override
	public JPanel getPanel() {
		return loadPanel;
	}
	
	private void makeContentFileChooser() {
		this.fileChooserPanel = new JPanel(new BorderLayout());
		final JFileChooser fileChooser = FileLoaderDialog.createFileChooser();
		final JCheckBox smoothNormalsCheckBox = new JCheckBox("smooth normals");
		final JCheckBox removeAppsCheckBox = new JCheckBox("ignore appearances");
		JPanel checkBoxPanel = new JPanel(new FlowLayout());
		fileChooserPanel.add(BorderLayout.CENTER, fileChooser);
		checkBoxPanel.add(smoothNormalsCheckBox);
		checkBoxPanel.add(removeAppsCheckBox);
		fileChooserPanel.add(BorderLayout.SOUTH, checkBoxPanel);
		fileChooser.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent ev) {
				File file = fileChooser.getSelectedFile();
				try {
					if (ev.getActionCommand() == JFileChooser.APPROVE_SELECTION
							&& file != null) {
						SceneGraphComponent read = Readers.read(Input.getInput(file));
						SceneGraphComponent tempRoot = new SceneGraphComponent();
						tempRoot.addChild(read);
						tempRoot.accept(new SceneGraphVisitor() {
							public void visit(SceneGraphComponent c) {
								if (removeAppsCheckBox.isSelected() && c.getAppearance() != null) c.setAppearance(null); 
								c.childrenWriteAccept(this, false, false, false, false, true,
										true);
							}
							public void visit(IndexedFaceSet i) {
								if (i.getFaceAttributes(Attribute.NORMALS) == null) IndexedFaceSetUtility.calculateAndSetFaceNormals(i);
								if (i.getVertexAttributes(Attribute.NORMALS) == null) IndexedFaceSetUtility.calculateAndSetVertexNormals(i);
								if (smoothNormalsCheckBox.isSelected()) IndexedFaceSetUtility.assignSmoothVertexNormals(i, -1);
							}
						});
						tempRoot.removeChild(read);
						getViewerVR().setContent(read);
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
				smoothNormalsCheckBox.setSelected(false);
				removeAppsCheckBox.setSelected(false);
				getViewerVR().switchToDefaultPanel();
			}
		});
	}
	
	public void switchToFileBrowser() {
		getViewerVR().switchToFileChooser(fileChooserPanel);
	}
}
