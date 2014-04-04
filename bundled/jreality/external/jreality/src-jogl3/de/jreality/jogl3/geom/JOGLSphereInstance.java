package de.jreality.jogl3.geom;

import java.util.LinkedList;

import javax.media.opengl.GL3;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniformFloat;
import de.jreality.jogl3.geom.JOGLGeometryInstance.InstanceFontData;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.helper.TransparencyHelper;
import de.jreality.jogl3.shader.SphereShader;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Sphere;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;

public class JOGLSphereInstance extends JOGLGeometryInstance {
	
	GLShader sphereShader = GLShader.defaultSphereShader;
	GLShader sphereShaderDepth = TransparencyHelper.depth;
	GLShader sphereShaderTransp = TransparencyHelper.transpSphere;
	
	public JOGLSphereInstance(Sphere node) {
		super(node);
	}

	@Override
	public void render(JOGLRenderState state, int width, int height) {
		if(eap == null)
			return;
		JOGLSphereEntity se = (JOGLSphereEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		boolean transparencyEnabled = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.TRANSPARENCY_ENABLED), false);
		if(visible && !transparencyEnabled)
			SphereShader.render(se, sphereUniforms, polygonTexture, polygonReflMap, sphereShader, state);
	}

	@Override
	public void renderDepth(JOGLRenderState state, int width, int height) {
		if(eap == null)
			return;
		JOGLSphereEntity se = (JOGLSphereEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		if(visible)
			SphereShader.renderDepth(se, sphereShaderDepth, state, width, height);
	}

	@Override
	public void addOneLayer(JOGLRenderState state, int width, int height, float alpha) {
		//TODO
		if(eap==null)
			return;
		JOGLSphereEntity se = (JOGLSphereEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		if(visible)
			SphereShader.addOneLayer(se, sphereUniforms, polygonTexture, polygonReflMap, sphereShaderTransp, state, width, height, alpha);
			//SphereShader.render(se, sphereUniforms, polygonTexture, polygonReflMap, sphereShader, state);
	}

	public LinkedList<GlUniform> sphereUniforms = new LinkedList<GlUniform>();
	public InstanceFontData ifd = new InstanceFontData();
	public GlTexture polygonTexture = new GlTexture();
	public GlReflectionMap polygonReflMap = new GlReflectionMap();
	@Override
	public void updateAppearance(SceneGraphPath sgp, GL3 gl, boolean appChanged, boolean geomLengthChanged, boolean geomPosChanged) {
		sphereUniforms = new LinkedList<GlUniform>();
		sphereShader = updateAppearance(ifd, GLShader.defaultSphereShader, sgp, gl, sphereUniforms, polygonTexture, polygonReflMap, CommonAttributes.POLYGON_SHADER);
	}
}