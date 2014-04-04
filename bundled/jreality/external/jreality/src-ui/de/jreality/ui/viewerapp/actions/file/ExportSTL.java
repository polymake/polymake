package de.jreality.ui.viewerapp.actions.file;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.FileOutputStream;

import javax.swing.JComponent;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.WriterSTL;

public class ExportSTL extends AbstractJrAction {

	private Viewer viewer;
	private SceneGraphComponent sgc;
	private JComponent options;

	public ExportSTL(String name, Viewer viewer, Component parentComp) {
		super(name, parentComp);

		if (viewer == null)
			throw new IllegalArgumentException("Viewer is null!");
		this.viewer = viewer;
		this.sgc = this.viewer.getSceneRoot();

		setShortDescription("Export the current scene as STL file");
	}

	public ExportSTL(String name, SceneGraphComponent sgc, Component parentComp) {
		super(name, parentComp);

		if (sgc == null)
			throw new IllegalArgumentException("SceneGraphComponent is null!");
		this.sgc = sgc;
		
		setShortDescription("Export the current scene as STL file");
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		
		File file = FileLoaderDialog.selectTargetFile(parentComp, options, "stl", "STL Files");
		if (file == null) return;  //dialog cancelled

		try {
//			WriterVRML.write(viewer.getSceneRoot(), new FileOutputStream(file));
			WriterSTL.write(sgc, new FileOutputStream(file));
		} catch (Exception exc) {
			exc.printStackTrace();
		}			
	}
}
