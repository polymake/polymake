package de.jreality.jogl.shader;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

public class NoneuclideanGLSLShader extends StandardGLSLShader {
	boolean poincareModel = false, needsRendered = true;
	SceneGraphPath poincarePath;
	public static final String POINCARE_MODEL = "poincareModel";
	public static final String POINCARE_PATH = "poincarePath";
	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		poincareModel = eap.getAttribute(
				ShaderUtility.nameSpace(name, POINCARE_MODEL), false);
		if (poincareModel) {
			
			poincarePath = (SceneGraphPath) eap.getAttribute(
					ShaderUtility.nameSpace(name, POINCARE_PATH),
					new SceneGraphPath());
			if (poincarePath.getLength() == 0)
				poincarePath = null;
		}
	}

	public boolean isPoincareModel() {
		return poincareModel;
	}

	public SceneGraphPath getPoincarePath() {
		return poincarePath;
	}

	protected void render(JOGLRenderer jr) {
		// the only reason we're doing it here is because only now do we know
		// what jrs is
		// System.err.println("writing glsl shader");
		if (true || needsRendered) { // return;
			JOGLRenderingState jrs = jr.renderingState;
//			System.err.println("current metric is "+jrs.currentMetric);
			glslProgram.setUniform("hyperbolic",
					jrs.currentMetric == Pn.HYPERBOLIC);
			glslProgram.setUniform("metric",
					(float) (jrs.currentMetric == Pn.HYPERBOLIC ? -1 : 1));
//			glslProgram.setUniform("Nw", 1.0);
			glslProgram.setUniform("useNormals4", jrs.normals4d);
			glslProgram.setUniform("poincareModel", poincareModel);
			if (poincarePath != null) {
				double[] H2Cam = Rn.times(null, jrs.worldToCamera,
						poincarePath.getMatrix(null)), cam2H = Rn.inverse(null,
						H2Cam);
				double[] H2NDC = Rn.times(null, jrs.cameraToNDC, H2Cam);
//				 System.err.println("c2p = "+Rn.matrixToString(H2NDC));
				glslProgram
						.setUniform("cam2H", Rn.convertDoubleToFloatArray(Rn
								.transpose(null, cam2H)));
				glslProgram.setUniform("H2Cam", Rn.convertDoubleToFloatArray(Rn
						.transpose(null,  Rn.inverse(null, cam2H))));
			}
		}
		super.render(jr);
		needsRendered = false;
	}

	@Override
	String getShaderLocation() {
		return "de/jreality/jogl/shader/resources/noneuclidean.vert";
	}

}
