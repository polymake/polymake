package de.jreality.plugin.content;

import java.awt.Window;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.IOException;

import javax.swing.Action;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JFileChooser;
import javax.swing.SwingUtilities;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.basic.ViewMenuBar;
import de.jreality.plugin.basic.ViewToolBar;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.reader.Readers;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.data.Attribute;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.util.Input;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.flavor.UIFlavor;

public class ContentLoader extends Plugin implements UIFlavor {

	private Content 
		content = null;
	private View
		view = null;
	private ViewMenuBar 
		viewerMenuAggregator = null;
	private ViewToolBar
		viewToolBar = null;
	private final JCheckBox 
		smoothNormalsCheckBox = new JCheckBox("smooth normals"),
		removeAppsCheckBox = new JCheckBox("ignore appearances");
	private JFileChooser 
		chooser = FileLoaderDialog.createFileChooser();
	private ContentLoadAction
		contentLoadAction = new ContentLoadAction();

	public ContentLoader() {
		Box checkBoxPanel = new Box(BoxLayout.Y_AXIS);
		JCheckBox smoothNormalsCheckBox = new JCheckBox("Smooth Normals");
		JCheckBox removeAppsCheckBox = new JCheckBox("Ignore Appearances");

		checkBoxPanel.add(smoothNormalsCheckBox);
		checkBoxPanel.add(removeAppsCheckBox);

		chooser.setAccessory(checkBoxPanel);
		chooser.setMultiSelectionEnabled(false);
	}
	
	private class ContentLoadAction extends AbstractJrAction {

		private static final long 
			serialVersionUID = 1L;

		public ContentLoadAction() {
			super("Load Content");
			setIcon(ImageHook.getIcon("folder_brick.png"));
			setShortDescription("Load Content");
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			loadFile();
		}
		
	}
	
	public Action getAction() {
		return contentLoadAction;
	}
	
	public void install(View view, Content content) {
		this.view = view;
		this.content = content;
	}

	private void loadFile() {
		File file = null;
		Window w = SwingUtilities.getWindowAncestor(view.getViewer().getViewingComponent());
		if (chooser.showOpenDialog(w) == JFileChooser.APPROVE_OPTION) {
			file = chooser.getSelectedFile();
		}
		if (file != null) {
			try {
				SceneGraphComponent read = Readers.read(Input.getInput(file));
				SceneGraphComponent tempRoot = new SceneGraphComponent();
				tempRoot.addChild(read);
				tempRoot.accept(new SceneGraphVisitor() {
					@Override
					public void visit(SceneGraphComponent c) {
						if (removeAppsCheckBox.isSelected() && c.getAppearance() != null) c.setAppearance(null); 
						c.childrenWriteAccept(this, false, false, false, false, true,
								true);
					}
					@Override
					public void visit(IndexedFaceSet i) {
						if (i.getFaceAttributes(Attribute.NORMALS) == null) IndexedFaceSetUtility.calculateAndSetFaceNormals(i);
						if (i.getVertexAttributes(Attribute.NORMALS) == null) IndexedFaceSetUtility.calculateAndSetVertexNormals(i);
						if (smoothNormalsCheckBox.isSelected()) IndexedFaceSetUtility.assignSmoothVertexNormals(i, -1);
					}
				});
				tempRoot.removeChild(read);
				content.setContent(read);
			} catch (IOException e) {
				e.printStackTrace();
			}
			smoothNormalsCheckBox.setSelected(false);
			removeAppsCheckBox.setSelected(false);
		}
	}


	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Content Loader";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("diskette.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		View viewPlugin = c.getPlugin(View.class);
		Content content = JRViewerUtility.getContentPlugin(c);
		install(
				viewPlugin,
				content
		);
		viewerMenuAggregator = c.getPlugin(ViewMenuBar.class);
		viewerMenuAggregator.addMenuItem(getClass(), 0.0, contentLoadAction, "File");
		viewToolBar = c.getPlugin(ViewToolBar.class);
		viewToolBar.addAction(getClass(), 0.0, contentLoadAction);
		viewToolBar.addSeparator(getClass(), 0.1);
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		viewerMenuAggregator.removeAll(getClass()); 
		viewToolBar.removeAll(getClass());
	}
	
	@Override
	public void restoreStates(Controller c) throws Exception {
		setCurrentDirectory(c.getProperty(getClass(), "currentDirectory", getCurrentDirectory()));
		super.restoreStates(c);
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		c.storeProperty(getClass(), "currentDirectory", getCurrentDirectory());
		super.storeStates(c);
	}

	public String getCurrentDirectory() {
		return chooser.getCurrentDirectory().getAbsolutePath();
	}
	
 	public void setCurrentDirectory(String directory) {
		File dir = new File(directory);
		if (dir.exists() && dir.isDirectory()) {
			chooser.setCurrentDirectory(dir);
		} else {
			System.out.println(
					"failed to restore ContentLoader directory "+directory
			);
		}
	}
 	
 	public void mainUIChanged(String uiClass) {
 		SwingUtilities.updateComponentTreeUI(chooser);
 	}
 	
}

