package de.jreality.ui.viewerapp.actions.camera;

import java.awt.event.ActionEvent;

import de.jreality.scene.Viewer;
import de.jreality.util.CameraUtility;

public class LoadCameraPreferences extends AbstractCameraAction {

	public LoadCameraPreferences(String name, Viewer v) {
		super(name, v);
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		CameraUtility.loadPreferences(CameraUtility.getCamera(viewer));
	}

}
