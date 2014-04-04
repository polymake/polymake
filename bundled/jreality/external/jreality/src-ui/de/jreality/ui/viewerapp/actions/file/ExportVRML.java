package de.jreality.ui.viewerapp.actions.file;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.FileOutputStream;

import javax.swing.AbstractAction;
import javax.swing.Box;
import javax.swing.JCheckBox;
import javax.swing.JComponent;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.ui.viewerapp.FileLoaderDialog;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jreality.writer.WriterVRML;
import de.jreality.writer.WriterVRML2;

public class ExportVRML extends AbstractJrAction {

	private Viewer viewer;
	
	private SceneGraphComponent sgc;

	private boolean writeTextureFiles = false;
	private boolean writeVrml2 = false;
	private boolean useDefs = true;
	private boolean evalTexMatrix= true;
	private boolean writeTexIndis = true;
	private boolean flipTexture = true;
	private boolean drawTubes = true;
	private boolean drawSpheres = true;
	private boolean moveLightsToSceneRoot=true;
	private boolean excludeTerrain=true;
	
	private JComponent options;

	public ExportVRML(String name, Viewer viewer, Component parentComp) {
		super(name, parentComp);

		if (viewer == null)
			throw new IllegalArgumentException("Viewer is null!");
		this.viewer = viewer;
		this.sgc = this.viewer.getSceneRoot();

		setShortDescription("Export the current scene as VRML file");
	}

	
	public ExportVRML(String name, SceneGraphComponent sgc, Component parentComp) {
		super(name, parentComp);

		if (sgc == null)
			throw new IllegalArgumentException("SceneGraphComponent is null!");
		this.sgc = sgc;

		setShortDescription("Export the current scene as VRML file");
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if (options == null) options = createAccessory();
		
		File file = FileLoaderDialog.selectTargetFile(parentComp, options, "wrl", "VRML Files");
		if (file == null) return;  //dialog cancelled

		if (options == null) options = createAccessory();

		try {
//			WriterVRML.write(viewer.getSceneRoot(), new FileOutputStream(file));

			if(writeVrml2){
				WriterVRML2 writer = new WriterVRML2(file);	
				writer.setWritePath(file.getParent()+"/");
				writer.setWriteTextureFiles(writeTextureFiles);
				writer.setDrawSpheres(drawSpheres);
				writer.setDrawTubes(drawTubes);
				writer.setEvaluateTextureMatrix(evalTexMatrix);
				writer.setFlipTextureUpsideDown(flipTexture);
				writer.setWriteTextureCoordIndices(writeTexIndis);
				writer.setMoveLightsToSceneRoot(moveLightsToSceneRoot);
				writer.setExcludeTerrain(excludeTerrain);
				writer.setUseDefs(useDefs);
				writer.write(sgc);
			}
			else{
				WriterVRML writer = new WriterVRML(new FileOutputStream(file));
				writer.setWritePath(file.getParent()+"/");
				writer.setWriteTextureFiles(writeTextureFiles);
				writer.setDrawSpheres(drawSpheres);
				writer.setDrawTubes(drawTubes);
				writer.setMoveLightsToSceneRoot(moveLightsToSceneRoot);
				writer.setExcludeTerrain(excludeTerrain);
				writer.setUseDefs(useDefs);
				writer.write(sgc);
			}
		} catch (Exception exc) {
			exc.printStackTrace();
		}			
	}
	
	JCheckBox textureCB;
	JCheckBox useDefsCB;
	JCheckBox vrml2CB;
	JCheckBox tubesCB;
	JCheckBox spheresCB;
	JCheckBox lightCB;
	JCheckBox evalTexMatrixCB;
	JCheckBox writeTexIndisCB;
	JCheckBox flipTextureCB;
	JCheckBox excludeTerrainCB;
	
	private JComponent createAccessory() {
		Box accessory = Box.createVerticalBox();
		accessory.add(Box.createVerticalGlue());
		textureCB = new JCheckBox(new AbstractAction("write texture files") {
			public void actionPerformed(ActionEvent e) {
				writeTextureFiles = textureCB.isSelected();
			}
		});
		useDefsCB = new JCheckBox(new AbstractAction("useDefs") {
			public void actionPerformed(ActionEvent e) {
				useDefs= useDefsCB.isSelected();
			}
		});
		evalTexMatrixCB = new JCheckBox(new AbstractAction("evaluate texture matrices -> only texturecoords. (problems with 'useDefs' ?)[vrml2]") {
			public void actionPerformed(ActionEvent e) {
				evalTexMatrix= evalTexMatrixCB.isSelected();
			}
		});
		writeTexIndisCB = new JCheckBox(new AbstractAction("write texture indices (only for compatibility)[vrml2]") {
			public void actionPerformed(ActionEvent e) {
				writeTexIndis= writeTexIndisCB.isSelected();
			}
		});
		flipTextureCB = new JCheckBox(new AbstractAction("texture upside down [vrml2]") {
			public void actionPerformed(ActionEvent e) {
				flipTexture= flipTextureCB.isSelected();
			}
		});
		vrml2CB= new JCheckBox(new AbstractAction("vrml2 instead of 1") {
			public void actionPerformed(ActionEvent e) {
				writeVrml2= vrml2CB.isSelected();
			}
		});
		tubesCB= new JCheckBox(new AbstractAction("draw tube-lines as cylinder [vrml2]") {
			public void actionPerformed(ActionEvent e) {
				drawTubes= tubesCB.isSelected();
			}
		});
		spheresCB= new JCheckBox(new AbstractAction("draw sphere-vertices as spheres [vrml2]") {
			public void actionPerformed(ActionEvent e) {
				drawSpheres= spheresCB.isSelected();
			}
		});
		lightCB= new JCheckBox(new AbstractAction("move lights to root (make global lights)") {
			public void actionPerformed(ActionEvent e) {
				moveLightsToSceneRoot= lightCB.isSelected();
			}
		});
		excludeTerrainCB = new JCheckBox(new AbstractAction("exclude terrain-geometry from export") {
			public void actionPerformed(ActionEvent e) {
				excludeTerrain = excludeTerrainCB.isSelected();
			}
		});
		accessory.add(vrml2CB);
		accessory.add(textureCB);
		accessory.add(useDefsCB);
		accessory.add(evalTexMatrixCB);
		accessory.add(writeTexIndisCB);
		accessory.add(flipTextureCB);
		accessory.add(tubesCB);
		accessory.add(spheresCB);
		accessory.add(lightCB);
		accessory.add(excludeTerrainCB);
		
		vrml2CB.setSelected(writeVrml2);
		textureCB.setSelected(writeTextureFiles);
		useDefsCB.setSelected(useDefs);
		evalTexMatrixCB.setSelected(evalTexMatrix);
		writeTexIndisCB.setSelected(writeTexIndis);
		flipTextureCB.setSelected(flipTexture);
		tubesCB.setSelected(drawTubes);
		spheresCB.setSelected(drawSpheres);
		lightCB.setSelected(moveLightsToSceneRoot);
		excludeTerrainCB.setSelected(excludeTerrain);
		
		return accessory;
	}
}
