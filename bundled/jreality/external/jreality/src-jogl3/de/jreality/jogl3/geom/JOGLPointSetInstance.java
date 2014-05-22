package de.jreality.jogl3.geom;

import java.util.LinkedList;

import javax.media.opengl.GL3;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniformFloat;
import de.jreality.jogl3.geom.JOGLGeometryInstance.InstanceFontData;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.LabelShader;
import de.jreality.jogl3.shader.PointShader;
import de.jreality.jogl3.shader.SpherePointShader;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;

public class JOGLPointSetInstance extends JOGLGeometryInstance {
	
	GLShader pointSphereShader = GLShader.defaultPointSphereShader;
	GLShader pointShader = GLShader.defaultPointShader;
	
	public JOGLPointSetInstance(PointSet node) {
		super(node);
	}
	
	private int labelsChangedNoCache = 0;
	private LabelRenderData labelData = new LabelRenderData();
	@Override
	public void render(JOGLRenderState state, int width, int height) {
		if(eap == null)
			return;
		JOGLPointSetEntity pse = (JOGLPointSetEntity) getEntity();
		if(vertexDraw){
			if(pse.labelsChangedNo != labelsChangedNoCache){
				//update label texture
				updateLabelTextureAndVBOsAndUniforms(state.getGL(), labelData, pse.labels, ifd);
				labelsChangedNoCache = pse.labelsChangedNo;
			}
			
			boolean spheresDraw = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POINT_SHADER, CommonAttributes.SPHERES_DRAW), CommonAttributes.SPHERES_DRAW_DEFAULT);
			if(spheresDraw)
				SpherePointShader.render(pse, pointSetPolygonUniforms, pointReflMap, pointSphereShader, state);
			else{
				PointShader.render(pse, pointSetUniforms, pointShader, state);
			}
			
			if(!transparencyEnabled && labelData.drawLabels)
				LabelShader.render(labelData, pse.labels, state);
		}
	}
	@Override
	public void renderDepth(JOGLRenderState state, int width, int height) {
		// TODO Auto-generated method stub
				if(eap==null)
					return;
				
				JOGLPointSetEntity pse = (JOGLPointSetEntity) getEntity();
				boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT);
				if(visible){
					if(labelData.drawLabels)
						LabelShader.renderDepth(labelData, pse.labels, state, width, height);
				}
	}

	@Override
	public void addOneLayer(JOGLRenderState state, int width, int height, float alpha) {
		// TODO Auto-generated method stub
				if(eap==null)
					return;
				
				JOGLPointSetEntity pse = (JOGLPointSetEntity) getEntity();
				boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT);
				if(visible){
					if(labelData.drawLabels)
						LabelShader.addOneLayer(labelData, pse.labels, state, width, height);
				}
	}
	
	public LinkedList<GlUniform> pointSetUniforms = new LinkedList<GlUniform>();
	public LinkedList<GlUniform> pointSetPolygonUniforms = new LinkedList<GlUniform>();
	public InstanceFontData ifd = new InstanceFontData();
	public GlTexture pointTexture = new GlTexture();
	public GlReflectionMap pointReflMap = new GlReflectionMap();
	@Override
	public void updateAppearance(SceneGraphPath sgp, GL3 gl, boolean appChanged, boolean geomLengthChanged, boolean geomPosChanged) {
		pointSetUniforms = new LinkedList<GlUniform>();
		
		//pointSphereShader = updateAppearance(GLShader.defaultPointShader, sgp, gl, pointSetPolygonUniforms, pointTexture, new GlReflectionMap(), CommonAttributes.POINT_SHADER);
		pointSphereShader = updateAppearance(ifd, GLShader.defaultPointSphereShader, sgp, gl, pointSetPolygonUniforms, pointTexture, pointReflMap, "pointShader.polygonShader");
		JOGLPointSetEntity pse = (JOGLPointSetEntity) getEntity();
		pointShader = updateAppearance(ifd, GLShader.defaultPointShader, sgp, gl, pointSetUniforms, pointTexture, new GlReflectionMap(), CommonAttributes.POINT_SHADER);
		updateLabelTextureAndVBOsAndUniforms(gl, labelData, pse.labels, ifd);

	}
}