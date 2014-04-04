package de.jreality.jogl3.geom;

import java.util.LinkedList;

import javax.media.opengl.GL3;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniformFloat;
import de.jreality.jogl3.geom.JOGLGeometryInstance.InstanceFontData;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.LabelShader;
import de.jreality.jogl3.shader.LineShader;
import de.jreality.jogl3.shader.TubesLineShader;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;

public class JOGLLineSetInstance extends JOGLPointSetInstance {

	//GLShader lineShader = GLShader.defaultLineShader;
	GLShader linePolygonShader = GLShader.defaultPolygonLineShader;
	GLShader lineShader = GLShader.defaultLineShader;
	
	public JOGLLineSetInstance(IndexedLineSet node) {
		super(node);
	}
	
	private int labelsChangedNoCache = 0;
	private LabelRenderData labelData = new LabelRenderData();
	
	public void render(JOGLRenderState state, int width, int height) {
		if(eap==null)
			return;
		
		JOGLLineSetEntity lse = (JOGLLineSetEntity) getEntity();
		if(edgeDraw){
			if(lse.labelsChangedNo != labelsChangedNoCache){
				//update label texture
				updateLabelTextureAndVBOsAndUniforms(state.getGL(), labelData, lse.labels, ifd);
				labelsChangedNoCache = lse.labelsChangedNo;
			}
			
			boolean tubesDraw = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.TUBES_DRAW), CommonAttributes.TUBES_DRAW_DEFAULT);
			if(tubesDraw)
				TubesLineShader.render(lse, lineSetPolygonUniforms, lineReflMap, linePolygonShader, state, width, height);
			else{
				float lineWidth = (float)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.LINE_WIDTH), CommonAttributes.LINE_WIDTH_DEFAULT);
	        	LineShader.render(lse, lineSetUniforms, lineShader, state, lineWidth);
			}
		}
		super.render(state, width, height);
		if(edgeDraw){
			if(!transparencyEnabled && labelData.drawLabels)
				LabelShader.render(labelData, lse.labels, state);
		}
	}
	@Override
	public void renderDepth(JOGLRenderState state, int width, int height) {
		super.renderDepth(state, width, height);
		// TODO Auto-generated method stub
		if(eap==null)
			return;
		
		JOGLLineSetEntity lse = (JOGLLineSetEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT);
		if(visible){
			if(labelData.drawLabels)
				LabelShader.renderDepth(labelData, lse.labels, state, width, height);
		}
	}

	@Override
	public void addOneLayer(JOGLRenderState state, int width, int height, float alpha) {
		super.addOneLayer(state, width, height, alpha);
		
		// TODO Auto-generated method stub
		if(eap==null)
			return;
		
		JOGLLineSetEntity lse = (JOGLLineSetEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT);
		if(visible){
			if(labelData.drawLabels)
				LabelShader.addOneLayer(labelData, lse.labels, state, width, height);
		}
	}
	public LinkedList<GlUniform> lineSetUniforms = new LinkedList<GlUniform>();
	public LinkedList<GlUniform> lineSetPolygonUniforms = new LinkedList<GlUniform>();
	public InstanceFontData ifd = new InstanceFontData();
	public GlTexture lineTexture = new GlTexture();
	public GlReflectionMap lineReflMap = new GlReflectionMap();
	@Override
	public void updateAppearance(SceneGraphPath sgp, GL3 gl, boolean appChanged, boolean geomLengthChanged, boolean geomPosChanged) {
		super.updateAppearance(sgp, gl, appChanged, geomLengthChanged, geomPosChanged);
		lineSetUniforms = new LinkedList<GlUniform>();
		lineSetPolygonUniforms = new LinkedList<GlUniform>();
		
		JOGLLineSetEntity lse = (JOGLLineSetEntity) getEntity();
		
		linePolygonShader = updateAppearance(ifd, GLShader.defaultPolygonLineShader, sgp, gl, lineSetPolygonUniforms, lineTexture, lineReflMap, "lineShader.polygonShader");
		
		lineShader = updateAppearance(ifd, GLShader.defaultLineShader, sgp, gl, lineSetUniforms, lineTexture, new GlReflectionMap(), CommonAttributes.LINE_SHADER);
		
		updateLabelTextureAndVBOsAndUniforms(gl, labelData, lse.labels, ifd);
	}
}
