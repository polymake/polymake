package de.jreality.ui.viewerapp.actions.camera;

import java.awt.event.ActionEvent;

import de.jreality.scene.Viewer;
import de.jreality.util.CameraUtility;

public class SaveCameraPreferences extends AbstractCameraAction {

	public SaveCameraPreferences(String name, Viewer v) {
		super(name, v);
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		CameraUtility.savePreferences(CameraUtility.getCamera(viewer));
	}

}
