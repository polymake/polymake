package de.jreality.jogl.shader;

import javax.media.opengl.GL;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.CameraUtility;

public class StereoPolygonShader extends DefaultPolygonShader {
	Texture2D leftTexture2D, rightTexture2D;
	JOGLTexture2D joglLeft, joglRight;

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		leftTexture2D = rightTexture2D = null;
		joglLeft = joglRight = null;
		if (AttributeEntityUtility.hasAttributeEntity(
				Texture2D.class,
				ShaderUtility.nameSpace(name, CommonAttributes.TEXTURE_2D
						+ ".left"), eap)) {
			leftTexture2D = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class, ShaderUtility
							.nameSpace(name, CommonAttributes.TEXTURE_2D
									+ ".left"), eap);
			// LoggingSystem.getLogger(this).fine("Got texture 2d for eap "+((Appearance)
			// eap.getAppearanceHierarchy().get(0)).getName());
			joglLeft = new JOGLTexture2D(leftTexture2D);
		}
		if (AttributeEntityUtility.hasAttributeEntity(
				Texture2D.class,
				ShaderUtility.nameSpace(name, CommonAttributes.TEXTURE_2D
						+ ".right"), eap)) {
			rightTexture2D = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class, ShaderUtility
							.nameSpace(name, CommonAttributes.TEXTURE_2D
									+ ".right"), eap);
			// LoggingSystem.getLogger(this).fine("Got texture 2d for eap "+((Appearance)
			// eap.getAppearanceHierarchy().get(0)).getName());
			joglRight = new JOGLTexture2D(rightTexture2D);
		}
	}

	@Override
	public void preRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL gl = jr.globalGL;
		JOGLTexture2D currTex = null;
		if (jrs.currentEye == CameraUtility.LEFT_EYE && joglLeft != null) {
			joglTexture2D = joglLeft;
		} else if (jrs.currentEye == CameraUtility.RIGHT_EYE
				&& joglRight != null) {
			joglTexture2D = joglRight;
		}
		super.preRender(jrs);
	}
}
