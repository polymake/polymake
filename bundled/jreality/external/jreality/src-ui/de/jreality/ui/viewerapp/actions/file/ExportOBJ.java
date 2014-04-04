package de.jreality.ui.viewerapp.actions.file;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.FileOutputStream;

import javax.swing.JComponent;

import de.jreality.geometry.GeometryMergeFactory;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.RemoveDuplicateInfo;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.Attribute;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.WriterOBJ;

public class ExportOBJ extends AbstractJrAction {

	private Viewer viewer;
	private SceneGraphComponent sgc;
	private JComponent options;

	public ExportOBJ(String name, Viewer viewer, Component parentComp) {
		super(name, parentComp);

		if (viewer == null)
			throw new IllegalArgumentException("Viewer is null!");
		this.viewer = viewer;
		this.sgc = this.viewer.getSceneRoot();

		setShortDescription("Export the current scene as OBJ file");
	}

	public ExportOBJ(String name, SceneGraphComponent sgc, Component parentComp) {
		super(name, parentComp);

		if (sgc == null)
			throw new IllegalArgumentException("SceneGraphComponent is null!");
		this.sgc = sgc;

		setShortDescription("Export the current scene as OBJ file");
	}
	
	@Override
	public void actionPerformed(ActionEvent e) {
		
		File file = FileLoaderDialog.selectTargetFile(parentComp, options, "obj", "OBJ Files");
		if (file == null) return;  //dialog cancelled

		try {
//			WriterVRML.write(viewer.getSceneRoot(), new FileOutputStream(file));
			// First have to combine all geometries into a single IFS
	        GeometryMergeFactory mergeFact= new GeometryMergeFactory();
	        IndexedFaceSet result=mergeFact.mergeGeometrySets(sgc);
	        result = (IndexedFaceSet) RemoveDuplicateInfo.removeDuplicateVertices(result, (Attribute[]) null);
	        boolean orient = IndexedFaceSetUtility.makeConsistentOrientation(result);
	        System.err.println("Export OBJ: oriented = "+orient);
	        IndexedFaceSetUtility.calculateAndSetFaceNormals(result);
	        IndexedFaceSetUtility.calculateAndSetVertexNormals(result);
			WriterOBJ.write(result, new FileOutputStream(file));
		} catch (Exception exc) {
			exc.printStackTrace();
		}			
	}
}
