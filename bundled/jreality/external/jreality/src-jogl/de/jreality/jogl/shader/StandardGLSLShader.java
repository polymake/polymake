package de.jreality.jogl.shader;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.io.IOException;

import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.scene.Appearance;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.GlslProgram;
import de.jreality.util.Input;

public abstract class StandardGLSLShader {
	boolean needsRendered = true;
	GlslProgram glslProgram = null;

	abstract String getShaderLocation();

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		if (glslProgram == null) {
			try {
				Appearance ap = new Appearance();
				glslProgram = new GlslProgram(ap, POLYGON_SHADER,
						Input.getInput(getShaderLocation()), null);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		needsRendered = true;
	}

	public GlslProgram getStandardShader() {
		return glslProgram;
	}

	protected void render(JOGLRenderer jr) {
		// the only reason we're doing it here is because only now do we know
		// what jrs is
		// System.err.println("writing glsl shader");
		if (needsRendered) { // return;
			JOGLRenderingState jrs = jr.renderingState;
			glslProgram.setUniform("lightingEnabled", jrs.lighting);
			glslProgram.setUniform("transparencyEnabled",
					jrs.transparencyEnabled);
			glslProgram.setUniform("numTextures", jrs.texUnitCount);
			// System.err.println("Number of texture units = "+jrs.texUnitCount);
			glslProgram.setUniform("transparency",
					(float) (1.0f - jrs.diffuseColor[3]));
			glslProgram.setUniform("numLights", jrs.numLights);
			glslProgram.setUniform("fogEnabled", jrs.fogEnabled);
			// needsRendered = false;
		}
		GlslLoader.render(glslProgram, jr);
	}

	public void postRender(GL2 gl) {
		GlslLoader.postRender(glslProgram, gl);
	}

}
