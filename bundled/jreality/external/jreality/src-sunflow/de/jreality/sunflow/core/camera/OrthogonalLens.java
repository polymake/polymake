package de.jreality.sunflow.core.camera;

import org.sunflow.SunflowAPI;
import org.sunflow.core.CameraLens;
import org.sunflow.core.ParameterList;
import org.sunflow.core.Ray;

public class OrthogonalLens implements CameraLens {

	private float au, av;
	private float aspect, fov, focus;

	public OrthogonalLens() {
		fov = 90.0f;
		aspect = 1.0f;
		focus = 3.0f;  //default of camera
		update();
	}

	public boolean update(ParameterList pl, SunflowAPI api) {
		// get parameters
		fov = pl.getFloat("fov", fov);
		aspect = pl.getFloat("aspect", aspect);
		focus = pl.getFloat("focus", focus);
		update();
		return true;
	}

	private void update() {
		au = (float) Math.tan(Math.toRadians(fov * 0.5f));
		av = au / aspect;
	}

	public Ray getRay(float x, float y, int imageWidth, int imageHeight, double lensX, double lensY, double time) {
		x = -au + ((2.0f * au * x) / (imageWidth - 1.0f));
		y = -av + ((2.0f * av * y) / (imageHeight - 1.0f));
		return new Ray(x*focus, y*focus, 0, 0, 0, -1);
	}
}