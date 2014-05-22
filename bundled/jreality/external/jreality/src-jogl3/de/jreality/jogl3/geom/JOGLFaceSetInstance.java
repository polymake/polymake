package de.jreality.jogl3.geom;

import java.util.LinkedList;
import java.util.WeakHashMap;

import javax.media.opengl.GL3;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.helper.TransparencyHelper;
import de.jreality.jogl3.shader.LabelShader;
import de.jreality.jogl3.shader.PolygonShader;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;

public class JOGLFaceSetInstance extends JOGLLineSetInstance {

	//GLShader polygonShader = new DefaultPolygonShader();
	GLShader polygonShader = GLShader.defaultPolygonShader;
	public GLShader getPolygonShader(){
		return polygonShader;
	}
	GLShader polygonShaderDepth = TransparencyHelper.depth;
	GLShader polygonShaderTransp = TransparencyHelper.transp;
	public JOGLFaceSetInstance(IndexedFaceSet node) {
		super(node);
	}
	
	private int labelsChangedNoCache = 0;
	public LabelRenderData labelData = new LabelRenderData();
	@Override
	public void render(JOGLRenderState state, int width, int height) {
		if(eap==null)
			return;
		
		JOGLFaceSetEntity fse = (JOGLFaceSetEntity) getEntity();
		if(faceDraw && !transparencyEnabled){
			if(fse.labelsChangedNo != labelsChangedNoCache){
				//update label texture
				updateLabelTextureAndVBOsAndUniforms(state.getGL(), labelData, fse.labels, ifd);
				labelsChangedNoCache = fse.labelsChangedNo;
			}
			PolygonShader.render(fse, faceSetUniforms, faceTexture, reflMap, polygonShader, state);
		}
		super.render(state, width, height);
		if(faceDraw && !transparencyEnabled){
			if(labelData.drawLabels)
				LabelShader.render(labelData, fse.labels, state);
		}
	}

	@Override
	public void renderDepth(JOGLRenderState state, int width, int height) {
		if(eap==null)
			return;
		super.renderDepth(state, width, height);
		JOGLFaceSetEntity fse = (JOGLFaceSetEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		if(visible){
			PolygonShader.renderDepth(fse, polygonShaderDepth, state, width, height);
			LabelShader.renderDepth(labelData, fse.labels, state, width, height);
		}
	}

	@Override
	public void addOneLayer(JOGLRenderState state, int width, int height, float alpha) {
		if(eap==null)
			return;
		super.addOneLayer(state, width, height, alpha);
		JOGLFaceSetEntity fse = (JOGLFaceSetEntity) getEntity();
		boolean visible = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		if(visible){
			PolygonShader.addOneLayer(fse, faceSetUniforms, faceTexture, reflMap, polygonShaderTransp, state, width, height, alpha);
			LabelShader.addOneLayer(labelData, fse.labels, state, width, height);
		}
	}
	
	public LinkedList<GlUniform> faceSetUniforms = new LinkedList<GlUniform>();
	public WeakHashMap<String, GlUniform> faceSetUniformsHash = new WeakHashMap<String, GlUniform>();
	public InstanceFontData ifd = new InstanceFontData();
	//public static GlTexture nullTexture = new GlTexture();
	public GlTexture faceTexture = new GlTexture();
	//private static GlReflectionMap nullReflMap = new GlReflectionMap();
	public GlReflectionMap reflMap = new GlReflectionMap();
	@Override
	public void updateAppearance(SceneGraphPath sgp, GL3 gl, boolean appChanged, boolean geomLengthChanged, boolean geomPosChanged) {
		if(geomPosChanged)
			oChangedPos = true;
		if(appChanged)
			oChangedAtt = true;
		if(geomLengthChanged)
			oChangedLength = true;
		super.updateAppearance(sgp, gl, appChanged, geomLengthChanged, geomPosChanged);
//		JOGLFaceSetEntity entity = (JOGLFaceSetEntity)this.getEntity();
//		IndexedFaceSet fs = (IndexedFaceSet)entity.getNode();
		faceSetUniforms = new LinkedList<GlUniform>();
		JOGLFaceSetEntity fse = (JOGLFaceSetEntity) getEntity();
		polygonShader = updateAppearance(ifd, GLShader.defaultPolygonShader, sgp, gl, faceSetUniforms, faceTexture, reflMap, CommonAttributes.POLYGON_SHADER);
		createFaceSetUniformsHashMap();
		updateLabelTextureAndVBOsAndUniforms(gl, labelData, fse.labels, ifd);
	}
	
	private void createFaceSetUniformsHashMap() {
		for(GlUniform glu : faceSetUniforms){
			faceSetUniformsHash.put(glu.name, glu);
		}
	}

	
}
