package de.jreality.jogl;

import java.util.HashMap;
import java.util.List;

import javax.media.opengl.GL2;

import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphPathObserver;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.SpotLight;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;

public class JOGLLightHelper {

	private double mat[] = new double[16];
	private transient double[][] matlist;
	private int lightCount = GL2.GL_LIGHT0;
	private GL2 lightGL = null;
	private int maxLights = 8;
	protected JOGLRenderer jr;

	protected JOGLLightHelper(JOGLRenderer r) {
		jr = r;
	}

	private SceneGraphVisitor ogllv = new SceneGraphVisitor() {
		public void visit(Light l) {
			wisit(l, lightGL, lightCount);
		}

		public void visit(DirectionalLight l) {
			wisit(l, lightGL, lightCount);
		}

		public void visit(PointLight l) {
			wisit(l, lightGL, lightCount);
		}

		public void visit(SpotLight l) {
			wisit(l, lightGL, lightCount);
		}
	};

	HashMap<SceneGraphPath, SceneGraphPathObserver> lightListeners = new HashMap<SceneGraphPath, SceneGraphPathObserver>();

	protected void resetLights(GL2 globalGL, List<SceneGraphPath> lights) {
		for (int i = 0; i < maxLights; ++i) {
			globalGL.glLightf(GL2.GL_LIGHT0 + i, GL2.GL_SPOT_CUTOFF, 0f);
			globalGL.glLightf(GL2.GL_LIGHT0 + i, GL2.GL_SPOT_EXPONENT,
					(float) 0);
			globalGL.glLightf(GL2.GL_LIGHT0 + i, GL2.GL_CONSTANT_ATTENUATION,
					1.0f);
			globalGL.glLightf(GL2.GL_LIGHT0 + i, GL2.GL_LINEAR_ATTENUATION,
					0.0f);
			globalGL.glLightf(GL2.GL_LIGHT0 + i, GL2.GL_QUADRATIC_ATTENUATION,
					0.0f);
			globalGL.glDisable(GL2.GL_LIGHT0 + i);
		}
		int n = lights.size();
		for (int i = 8; i < n; ++i)
			lights.remove(i);
		for (SceneGraphPath sgp : lights) {
			SceneGraphPathObserver sgpo = new SceneGraphPathObserver(sgp);
			sgpo.addTransformationListener(new TransformationListener() {

				public void transformationMatrixChanged(TransformationEvent ev) {
					jr.lightsChanged = true;
				}

			});
			lightListeners.put(sgp, sgpo);
		}
		cacheLightMatrices(lights);
	}

	protected void cacheLightMatrices(List<SceneGraphPath> lights) {
		int n = lights.size();
		matlist = new double[n][16];
		for (int i = 0; i < n; ++i) {
			SceneGraphPath lp = (SceneGraphPath) lights.get(i);
			SceneGraphNode light = lp.getLastElement();
			if (!(light instanceof Light)) {
				JOGLConfiguration.theLog
						.warning("Invalid light path: no light there");
				continue;
			}
			lp.getMatrix(matlist[i]);
		}

	}

	protected void disposeLights() {
		for (SceneGraphPathObserver obs : lightListeners.values()) {
			obs.dispose();
		}
		lightListeners.clear();
	}

	public void enableLights(GL2 globalGL, int num) {
		for (int i = 0; i < num; ++i)
			globalGL.glEnable(GL2.GL_LIGHT0 + i);
	}

	public void processLights(GL2 globalGL, List<SceneGraphPath> lights) {
		lightCount = GL2.GL_LIGHT0;
		lightGL = globalGL;
		int n = lights.size();
		for (int i = 0; i < n; ++i) {
			SceneGraphNode light = lights.get(i).getLastElement();
			globalGL.glPushMatrix();
			globalGL.glMultTransposeMatrixd(matlist[i], 0);
			light.accept(ogllv);
			globalGL.glPopMatrix();
			lightCount++;
		}
	}

	private float[] zDirection = { 0, 0, 1, 0 }; // (float)10E-10};

	private float[] origin = { 0, 0, 0, 1 };

	private void wisit(Light dl, GL2 globalGL, int lightCount) {
		globalGL.glLightf(lightCount, GL2.GL_SPOT_CUTOFF, 180f); // use cutoff
																	// ==
																	// 0 as
																	// marker
																	// for
																	// invalid
																	// lights in
																	// glsl
		globalGL.glLightfv(lightCount, GL2.GL_DIFFUSE,
				dl.getScaledColorAsFloat(), 0);
		float f = (float) dl.getIntensity();
		float[] specC = { f, f, f };
		globalGL.glLightfv(lightCount, GL2.GL_SPECULAR, specC, 0);
		globalGL.glLightfv(lightCount, GL2.GL_AMBIENT,
				dl.getScaledColorAsFloat(), 0);
	}

	private void wisit(DirectionalLight dl, GL2 globalGL, int lightCount) {
		wisit((Light) dl, globalGL, lightCount);
		globalGL.glLightfv(lightCount, GL2.GL_POSITION, zDirection, 0);
	}

	private void wisit(PointLight dl, GL2 globalGL, int lightCount) {
		// gl.glLightfv(lightCount, GL.GL_AMBIENT, lightAmbient);
		wisit((Light) dl, globalGL, lightCount);
		globalGL.glLightfv(lightCount, GL2.GL_POSITION, origin, 0);
		globalGL.glLightf(lightCount, GL2.GL_CONSTANT_ATTENUATION,
				(float) dl.getFalloffA0());
		globalGL.glLightf(lightCount, GL2.GL_LINEAR_ATTENUATION,
				(float) dl.getFalloffA1());
		globalGL.glLightf(lightCount, GL2.GL_QUADRATIC_ATTENUATION,
				(float) dl.getFalloffA2());
	}

	private void wisit(SpotLight dl, GL2 globalGL, int lightCount) {
		wisit((PointLight) dl, globalGL, lightCount);
		globalGL.glLightf(lightCount, GL2.GL_SPOT_CUTOFF,
				(float) ((180.0 / Math.PI) * dl.getConeAngle()));
		globalGL.glLightfv(lightCount, GL2.GL_SPOT_DIRECTION, zDirection, 0);
		globalGL.glLightf(lightCount, GL2.GL_SPOT_EXPONENT,
				(float) dl.getDistribution());
	}

}
