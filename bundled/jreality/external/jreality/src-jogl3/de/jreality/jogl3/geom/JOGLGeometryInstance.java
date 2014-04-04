package de.jreality.jogl3.geom;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.font.FontRenderContext;
import java.awt.font.TextLayout;
import java.awt.image.BufferedImage;
import java.util.LinkedList;

import javax.media.opengl.GL3;
import javax.swing.SwingConstants;

import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.JOGLTexture2D;
import de.jreality.jogl3.geom.Label;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.glsl.GLShader.ShaderVar;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.ShaderVarHash;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.proxy.tree.SceneTreeNode;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public abstract class JOGLGeometryInstance extends SceneTreeNode {

	//the "o" indicates that this code is only neccessary for the optimization of scenes with 10.000 small
		//geometries or more.
		protected boolean oChangedLength = true;
		protected boolean oChangedPos = true;
		protected boolean oChangedAtt = true;
		public boolean oChangedLength() {
			return oChangedLength;
		}
		
		public boolean oChangedPositions() {
			return oChangedPos;
		}
		
		public boolean oChangedAttributes() {
			return oChangedAtt;
		}
		
		public void resetOChangedLength() {
			oChangedLength = false;
		}
		
		public void resetOChangedPositionsOrAttributes() {
			oChangedPos = false;
			oChangedAtt = false;
		}
	
	public abstract class GlUniform<T>{
		public GlUniform(String name, T value){
			this.name = name;
			this.value = value;
		}
		public String name;
		public T value;
		
		public abstract void bindToShader(GLShader shader, GL3 gl);
		
		
	}
	public class GlUniformInt extends GlUniform<Integer>{

		public GlUniformInt(String name, Integer value) {
			super(name, value);
		}
		public void bindToShader(GLShader shader, GL3 gl){
			ShaderVarHash.bindUniform(shader, name, value, gl);
		}
	}
	public class GlUniformFloat extends GlUniform<Float>{

		public GlUniformFloat(String name, Float value) {
			super(name, value);
		}
		public void bindToShader(GLShader shader, GL3 gl){
			ShaderVarHash.bindUniform(shader,  name, value, gl);
		}
	}
	public class GlUniformVec extends GlUniform<float[]>{

		public GlUniformVec(String name, float[] value) {
			super(name, value);
		}
		public void bindToShader(GLShader shader, GL3 gl){
			ShaderVarHash.bindUniform(shader, name, value, gl);
		}
	}
	public class GlUniformMat4 extends GlUniform<float[]>{

		public GlUniformMat4(String name, float[] value) {
			super(name, value);
		}
		public void bindToShader(GLShader shader, GL3 gl){
			ShaderVarHash.bindUniformMatrix(shader, name, value, gl);
        }
	}
//	public class GlUniformVec3 extends GlUniform<float[]>{
//
//		public GlUniformVec3(String name, float[] value) {
//			super(name, value);
//		}
//		public void bindToShader(GLShader shader, GL3 gl){
//			ShaderVarHash.bindUniform(shader, name, value, gl);
//		}
//	}
	
	//TODO make private
	public EffectiveAppearance eap;
	
	//TODO
	//public GLShader;
	//public Appearance Attributes for this shader;
	
	protected JOGLGeometryInstance(Geometry node) {
		super(node);
	}

	public abstract void render(JOGLRenderState state, int width, int height);
	public abstract void renderDepth(JOGLRenderState state, int width, int height);
	public abstract void addOneLayer(JOGLRenderState state, int width, int height, float alpha);
	
	private String retrieveType(String name){
		String[] s = name.split("_");
		if(s.length > 1 && s[1].equals("polygonShader"))
			return s[0]+"."+s[1];
		return s[0];
	}
	
	private String retrieveName(String name){
		String[] s = name.split("_");
		if(s.length > 2 && s[1].equals("polygonShader"))
			return name.substring(s[0].length()+s[1].length()+2);
		if(s.length > 1)
			return name.substring(s[0].length()+1);
		else
			return s[0];
	}
	
	public class InstanceFontData{
		public Font font;
		public double scale;
		public double[] offset;
		public int alignment;
		public Color color;
		public boolean drawLabels;
	}
	
	public class LabelRenderData{
		public Texture2D[] tex;//
		public GLVBOFloat[] points;//
		public GLVBOFloat[] ltwh;//
		//public float scale;//
		public float[] xyzOffsetScale = new float[4];//
		public float[][] xyAlignmentTotalWH;// = new float[4];//
		public boolean drawLabels;
	}
	
	private static final FontRenderContext frc;
	  private static BufferedImage bi;
		//TODO is there a better way to get a FontRenderContext???
		static {
			bi = new BufferedImage(1,1,BufferedImage.TYPE_INT_ARGB);
			frc = bi.createGraphics().getFontRenderContext();
		}
	private final int MAX_TEX_SIZE = 16000;
	public void updateLabelTextureAndVBOsAndUniforms(GL3 gl, LabelRenderData lrd, Label[] labels, InstanceFontData ifd){
		if(labels == null || labels.length == 0)
			return;
//		System.out.println("updateLabelTextureAndVBOsAndUniforms called");
		lrd.drawLabels = ifd.drawLabels;
		lrd.xyzOffsetScale[3] = (float)ifd.scale;
		
		lrd.xyzOffsetScale[0] = (float)ifd.offset[0];
		lrd.xyzOffsetScale[1] = (float)ifd.offset[1];
		lrd.xyzOffsetScale[2] = (float)ifd.offset[2];
		
		
		
		//first, find out how many textures we need
		int texcount = 1;
		
		BufferedImage buf;
		int totalwidth = 0, totalheight = 0, hh[][] = new int[labels.length][];
		int width[] = new int[labels.length];
		String[][] ss = new String[labels.length][];
		float border[] = new float[labels.length];
		for(int j = 0; j < labels.length; j++){
			border[j] = 0.0f;
			ss[j] = labels[j].text.split("\n");
			width[j] = 0;
			int height = 0;
			hh[j] = new int[ss[j].length];
		  
			
			if (ifd.font == null)
				ifd.font = new Font("Sans Serif",Font.PLAIN,48);
			// process the strings to find out how large the image needs to be
			// I'm not sure if I'm handling the border correctly: should a new border
			// be added for each string?  Or only for the first or last?
		  
			//only measuring the size of the rectangle needed to draw all these lines of text
			for (int i = 0; i < ss[j].length; ++i) {
				String s = ss[j][i];
				if (s == null || s.length() == 0) {
					buf=bi;
				}
				TextLayout tl = new TextLayout(s, ifd.font, frc);
				Rectangle r = tl.getBounds().getBounds();
				hh[j][i] = (int) ifd.font.getLineMetrics(s, frc).getHeight();
				height += hh[j][i];
				int tmp = (r.width + 20);
				if (tmp > width[j]) width[j] = tmp;
				float ftmp = hh[j][i] - tl.getDescent();
				if (ftmp > border[j]) border[j] = ftmp;
			}
			if(height > totalheight)
				totalheight = height;
			totalwidth += width[j];
			
//			System.out.println("totalwidth = " + totalwidth);
//			System.out.println("totalwidth 1st loop = " + totalwidth);
			if(totalwidth > MAX_TEX_SIZE){
				texcount++;
				totalwidth = width[j];
				totalheight = height;
			}
		}
//		System.out.println("texcount = " + texcount);
		lrd.tex = new Texture2D[texcount];//
		lrd.points = new GLVBOFloat[texcount];//
		lrd.ltwh = new GLVBOFloat[texcount];//
		lrd.xyAlignmentTotalWH = new float[texcount][];//
		
		int[] pointCounter = new int[texcount];
		//and everything again, this time saving all variables
		texcount = 0;
		totalwidth = 0;
		for(int j = 0; j < labels.length; j++){
			pointCounter[texcount]++;
			border[j] = 0.0f;
			//maybe unneccessary
			ss[j] = labels[j].text.split("\n");
			width[j] = 0;
			int height = 0;
			//maybe unneccessary
//			hh[j] = new int[ss[j].length];
		  
			
			if (ifd.font == null)
				ifd.font = new Font("Sans Serif",Font.PLAIN,48);
			// process the strings to find out how large the image needs to be
			// I'm not sure if I'm handling the border correctly: should a new border
			// be added for each string?  Or only for the first or last?
		  
			//only measuring the size of the rectangle needed to draw all these lines of text
			for (int i = 0; i < ss[j].length; ++i) {
				String s = ss[j][i];
				if (s == null || s.length() == 0) {
					buf=bi;
				}
				TextLayout tl = new TextLayout(s, ifd.font, frc);
				Rectangle r = tl.getBounds().getBounds();
				//maybe unneccessary
//				hh[j][i] = (int) ifd.font.getLineMetrics(s, frc).getHeight();
				height += hh[j][i];
				int tmp = (r.width + 20);
				if (tmp > width[j]) width[j] = tmp;
				float ftmp = hh[j][i] - tl.getDescent();
				if (ftmp > border[j]) border[j] = ftmp;
			}
			if(height > totalheight)
				totalheight = height;
			totalwidth += width[j];
			
//			System.out.println("totalwidth 2nd loop = " + totalwidth);
			if(totalwidth > MAX_TEX_SIZE){
//				System.out.println("texcount inside loop = " + texcount);
				pointCounter[texcount]--;
				pointCounter[texcount+1] = 1;
				lrd.xyAlignmentTotalWH[texcount] = new float[4];
				lrd.xyAlignmentTotalWH[texcount][2] = totalwidth-width[j];
				lrd.xyAlignmentTotalWH[texcount][3] = totalheight;
				switch(ifd.alignment){
					case SwingConstants.NORTH  : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
					case SwingConstants.EAST   : lrd.xyAlignmentTotalWH[texcount][0] = 0; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
					case SwingConstants.SOUTH  : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = 0; break;
					case SwingConstants.WEST   : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
					case SwingConstants.CENTER : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
					case SwingConstants.NORTH_EAST : lrd.xyAlignmentTotalWH[texcount][0] = 0; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
					//case SwingConstants.SOUTH_EAST : default
					case SwingConstants.SOUTH_WEST : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = 0f; break;
					case SwingConstants.NORTH_WEST : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
				}
				texcount++;
				totalwidth = width[j];
				totalheight = height;
			}
		}
//		System.out.println("texcount = " + texcount);
		lrd.xyAlignmentTotalWH[texcount] = new float[4];
		lrd.xyAlignmentTotalWH[texcount][2] = totalwidth;
		lrd.xyAlignmentTotalWH[texcount][3] = totalheight;
		switch(ifd.alignment){
			case SwingConstants.NORTH  : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
			case SwingConstants.EAST   : lrd.xyAlignmentTotalWH[texcount][0] = 0; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
			case SwingConstants.SOUTH  : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = 0; break;
			case SwingConstants.WEST   : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
			case SwingConstants.CENTER : lrd.xyAlignmentTotalWH[texcount][0] = -.5f; lrd.xyAlignmentTotalWH[texcount][1] = -.5f; break;
			case SwingConstants.NORTH_EAST : lrd.xyAlignmentTotalWH[texcount][0] = 0; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
			//case SwingConstants.SOUTH_EAST : default
			case SwingConstants.SOUTH_WEST : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = 0f; break;
			case SwingConstants.NORTH_WEST : lrd.xyAlignmentTotalWH[texcount][0] = -1f; lrd.xyAlignmentTotalWH[texcount][1] = -1f; break;
		}
		
		int jCounter = 0;
//		System.out.println("tex.length = " + lrd.tex.length);
		for(int i = 0; i < lrd.tex.length; i++){
//			System.out.println("i = " + i);
			float[] ltwh = new float[4*pointCounter[i]];
			buf = new BufferedImage((int)lrd.xyAlignmentTotalWH[i][2], (int)lrd.xyAlignmentTotalWH[i][3],BufferedImage.TYPE_INT_ARGB);
			Graphics2D g = (Graphics2D) buf.getGraphics();
			g.setBackground(new Color(0,0,0,0));
			g.clearRect(0, 0, (int)lrd.xyAlignmentTotalWH[i][2], (int)lrd.xyAlignmentTotalWH[i][3]);
			g.setColor(ifd.color);
			g.setFont(ifd.font);
			// LineMetrics lineMetrics = f.getLineMetrics(s,frc).getHeight();
			g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
					RenderingHints.VALUE_ANTIALIAS_ON);
			int widthOffset = 0;
			for(int j = jCounter; j < jCounter+pointCounter[i]; j++){
				int height = 0;
				
				for (int k = 0; k < ss[j].length; ++k) {
					g.drawString(ss[j][k], widthOffset, height + border[j]);
					height += hh[j][k];
				}
				ltwh[4*(j-jCounter)+0] = (widthOffset*1.0f)/lrd.xyAlignmentTotalWH[i][2];
				ltwh[4*(j-jCounter)+1] = 0;
				ltwh[4*(j-jCounter)+2] = (width[j]*1.0f)/lrd.xyAlignmentTotalWH[i][2];
				ltwh[4*(j-jCounter)+3] = (height*1.0f)/lrd.xyAlignmentTotalWH[i][3];
				widthOffset += width[j];
			}
			float[] points = new float[4*pointCounter[i]];
			for(int k = 0; k < pointCounter[i]; k++){
				points[4*k+0] = (float)labels[k+jCounter].position[0];
				points[4*k+1] = (float)labels[k+jCounter].position[1];
				points[4*k+2] = (float)labels[k+jCounter].position[2];
				points[4*k+3] = (float)labels[k+jCounter].position[3];
			}
			lrd.points[i] = new GLVBOFloat(gl, points, "centers");
			
			jCounter += pointCounter[i];
			lrd.ltwh[i] = new GLVBOFloat(gl, ltwh, "ltwh");
			
			ImageData img = new ImageData(buf);
			
			Texture2D labelTexture = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
			labelTexture.setImage(img);
			lrd.tex[i] = labelTexture;
		}
	}
	
	protected boolean faceDraw = false;
	protected boolean vertexDraw = false;
	protected boolean edgeDraw = false;
	protected boolean transparencyEnabled = false;
	
	public boolean getFaceDraw(){
		return faceDraw;
	}
	public boolean getVertexDraw(){
		return vertexDraw;
	}
	public boolean getEdgeDraw(){
		return edgeDraw;
	}
	public boolean getTransparencyEnabled(){
		return transparencyEnabled;
	}
	
	//in the new version we use type only to identify the shader source
	protected GLShader updateAppearance(InstanceFontData ifd, GLShader defaultShader, SceneGraphPath sgp, GL3 gl, LinkedList<GlUniform> c, GlTexture texture, GlReflectionMap reflMap, String shaderType) {
		
		GLShader shader = defaultShader;
		
		eap = EffectiveAppearance.create(sgp);
		
		edgeDraw = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.LINE_SHADER, CommonAttributes.EDGE_DRAW), CommonAttributes.EDGE_DRAW_DEFAULT);
		vertexDraw = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POINT_SHADER, CommonAttributes.VERTEX_DRAW), CommonAttributes.VERTEX_DRAW_DEFAULT);
		faceDraw = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.FACE_DRAW), CommonAttributes.FACE_DRAW_DEFAULT);
		transparencyEnabled = (boolean)eap.getAttribute(ShaderUtility.nameSpace(CommonAttributes.POLYGON_SHADER, CommonAttributes.TRANSPARENCY_ENABLED), false);
		
		
		//retrieve shader source if existent
		String[] source = new String[]{};
		
		source = (String[])eap.getAttribute(shaderType + ".glsl330-source", source);
		// has attribute key like "polygonShader::glsl330-source"
		// and an array of two Strings
		if(source != null && source.length == 2){
			System.out.println("shader type is " + shaderType);
			System.out.println("creating custom shader. source is " + source[0]);
			//TODO problem here! we are not passing back the pointer...
			shader = new GLShader(source[0], source[1]);
			shader.init(gl);
		}
		
		
		//retrieval of uniform variables for labels
		Object va = new Object();
		va = eap.getAttribute(shaderType+".textShader.font",  CommonAttributes.getDefault("font", va));
		ifd.font = (Font)va;
		
		va = eap.getAttribute(shaderType+".textShader.scale",  CommonAttributes.getDefault("scale", va));
		ifd.scale = (Double)va;
		
		va = eap.getAttribute(shaderType+".textShader.offset",  CommonAttributes.getDefault("offset", va));
		ifd.offset = (double[])va;
		
		va = eap.getAttribute(shaderType+".textShader.alignment",  CommonAttributes.getDefault("alignment", va));
		ifd.alignment = (Integer)va;
		
		va = eap.getAttribute(shaderType+".textShader.diffuseColor",  CommonAttributes.getDefault("diffuseColor", va));
		ifd.color = (Color)va;
		
		va = eap.getAttribute(shaderType+".textShader.showLabels",  CommonAttributes.getDefault("showLabels", va));
		ifd.drawLabels = (Boolean)va;
		
//		System.out.println("" + shaderType+".textShader.alignment " + ifd.alignment);
		
		
		//Automatic retrieval of shader attributes from appearance object for vertex/fragment shaders
		//TODO retrieve and save shader attributes in a sensible
		//fashion
		boolean hasTexture = false;
		boolean hasReflectionMap = false;
		for(ShaderVar v : shader.shaderUniforms){
			
			String name = retrieveName(v.getName());
    		String type = retrieveType(v.getName());
    		
			//if(type.equals(CommonAttributes.POINT_SHADER))
				//System.out.println("shader var is " + v.getName() + ", type is " + v.getType());
			if(v.getName().equals("projection"))
    			continue;
    		if(v.getName().equals("modelview")){
    			continue;
    		}
    		if(v.getName().equals("screenSize")){
    			continue;
    		}
    		if(v.getName().equals("screenSizeInSceneOverScreenSize")){
    			continue;
    		}
//    		if(v.getName().equals("front")){
//    			continue;
//    		}
    		if(name.equals("back")){
    			continue;
    		}
    		if(name.equals("left")){
    			continue;
    		}
    		if(name.equals("right")){
    			continue;
    		}
    		if(name.equals("up")){
    			continue;
    		}
    		if(name.equals("down")){
    			continue;
    		}
    		//TODO clean this up, it doesn't do anything I believe
    		if(v.getName().length() > 3 && v.getName().substring(0, 4).equals("_")){
    			continue;
    		}
    		if(v.getName().length() > 3 && type.equals("has")){
    			continue;
    		}
    		if(v.getName().length() > 3 && type.equals("sys")){
    			continue;
    		}
    		//System.out.println("updateAppearance " + v.getName());
    		//TODO exclude some more like light samplers, camPosition
    		//retrieve corresponding attribute from eap
    		
    		
			if(v.getType().equals("int")){
    			Object value = new Object();
//    			Set keys = eap.getApp().getAttributes().keySet();
//    			for(Object o : keys){
//    				String s = (String)o;
//    				System.out.println(s);
//    			}
    			
    			value = eap.getAttribute(ShaderUtility.nameSpace(type,name),  CommonAttributes.getDefault(retrieveName(v.getName()), value));
//    			System.out.println("" + ShaderUtility.nameSpace(type,name) + ", " + v.getType() + ", " + value.getClass());
    			if(value.getClass().equals(Integer.class)){
    				c.add(new GlUniformInt(v.getName(), (Integer)value));
    				//c.intUniforms.add(new GlUniform<Integer>(v.getName(), (Integer)value));
    				//gl.glUniform1i(gl.glGetUniformLocation(polygonShader.shaderprogram, v.getName()), (Integer)value);
    			}else if(value.getClass().equals(Boolean.class)){
    				boolean b = (Boolean)value;
    				int valueInt = 0;
        			if(b){
        				valueInt = 1;
        			}
        			c.add(new GlUniformInt(v.getName(), valueInt));
        			//gl.glUniform1i(gl.glGetUniformLocation(polygonShader.shaderprogram, v.getName()), valueInt);
    			}else{
    				//c.add(new GlUniformInt(v.getName(), 0));
    			}
    		}
    		else if(v.getType().equals("vec4")){
//    			System.out.println(v.getName());
    			Object value = new Object();
    			//System.out.println(v.getName());
    			//TODO retrieve default value somehow...
    			
    			value = eap.getAttribute(ShaderUtility.nameSpace(type,name), CommonAttributes.getDefault(name, value));
    			
    			if(value.getClass().equals(Color.class)){
    				float[] color = ((Color)value).getRGBComponents(null);
    				//System.out.println(sgp.getLastComponent().getName() + type + "." + v.getName() + color[0] + " " + color[1] + " " + color[2]);
    				c.add(new GlUniformVec(v.getName(), color));
    			}else if(value.getClass().equals(float[].class)){
    				c.add(new GlUniformVec(v.getName(), (float[])value));
    			}else if(value.getClass().equals(double[].class)){
    				double[] value2 = (double[])value;
    				c.add(new GlUniformVec(v.getName(), Rn.convertDoubleToFloatArray(value2)));
    			}else{
    				//default value
    				//c.add(new GlUniformVec4(v.getName(), new float[]{0, 0, 0, 1}));
    			}
    		}else if(v.getType().equals("vec3")){
//    			System.out.println(v.getName());
    			Object value = new Object();
    			//System.out.println(v.getName());
    			//TODO retrieve default value somehow...
    			
    			value = eap.getAttribute(ShaderUtility.nameSpace(type,name), CommonAttributes.getDefault(name, value));
    			
    			if(value.getClass().equals(float[].class)){
    				float[] tmp = (float[])value;
    				c.add(new GlUniformVec(v.getName(), tmp));
    			}else if(value.getClass().equals(double[].class)){
    				double[] value2 = (double[])value;
    				float[] tmp = Rn.convertDoubleToFloatArray(value2);
    				c.add(new GlUniformVec(v.getName(), tmp));
    			}else{
    				//default value
    				//c.add(new GlUniformVec4(v.getName(), new float[]{0, 0, 0, 1}));
    			}
    		}else if(v.getType().equals("vec2")){
//    			System.out.println(v.getName());
    			Object value = new Object();
    			//System.out.println(v.getName());
    			//TODO retrieve default value somehow...
    			
    			value = eap.getAttribute(ShaderUtility.nameSpace(type,name), CommonAttributes.getDefault(name, value));
    			if(value.getClass().equals(float[].class)){
    				float[] tmp = (float[])value;
    				c.add(new GlUniformVec(v.getName(), tmp));
    			}else if(value.getClass().equals(double[].class)){
    				double[] value2 = (double[])value;
    				float[] tmp = Rn.convertDoubleToFloatArray(value2);
    				c.add(new GlUniformVec(v.getName(), tmp));
    			}else{
    				//default value
    				//c.add(new GlUniformVec4(v.getName(), new float[]{0, 0, 0, 1}));
    			}
    		}
    		else if(v.getType().equals("float")){
//    			System.out.println(v.getName());
    			Object value = new Object();
    			//System.out.println(v.getName());
    			value = eap.getAttribute(ShaderUtility.nameSpace(type,name),  CommonAttributes.getDefault(name, value));
    			
    			if(value.getClass().equals(Double.class)){
    				Double value2 = (Double)value;
    				c.add(new GlUniformFloat(v.getName(), value2.floatValue()));
    			}else if(value.getClass().equals(Float.class)){
    				c.add(new GlUniformFloat(v.getName(), (Float)value));
    			}else{
    				//c.add(new GlUniformFloat(v.getName(), 0f));
    			}
    		}else if(v.getType().equals("sampler2D") && name.equals("image")){
    			//ImageData value = new Object();
    			//value = eap.getAttribute(ShaderUtility.nameSpace(type, "texture2d:image"), value);
    			//MyEntityInterface mif = (MyEntityInterface) AttributeEntityFactory.createAttributeEntity(MyEntityInterface.class, &quot;myEntityName&quot;, ea);
    			//Texture2D tex = (Texture2D)
    			if(AttributeEntityUtility.hasAttributeEntity(Texture2D.class, shaderType + ".texture2d", eap)){
    				Texture2D tex = (Texture2D)AttributeEntityUtility.createAttributeEntity(Texture2D.class, shaderType + ".texture2d", eap);
    				texture.setTexture(tex);
    				c.add(new GlUniformInt("_combineMode", tex.getApplyMode()));
    				texture.combineMode = tex.getApplyMode();
    				c.add(new GlUniformMat4("textureMatrix", Rn.convertDoubleToFloatArray(tex.getTextureMatrix().getArray())));
//    				System.out.println("sampler2D: "+ v.getName());
    				hasTexture = true;
    			}
    		}else if(v.getType().equals("sampler2D") && name.equals("front")){
    			if(AttributeEntityUtility.hasAttributeEntity(CubeMap.class, shaderType + ".reflectionMap", eap)){
    				CubeMap reflectionMap = TextureUtility.readReflectionMap(eap, shaderType + ".reflectionMap");
    				reflMap.setCubeMap(reflectionMap);
    				c.add(new GlUniformFloat("_reflectionMapAlpha", reflectionMap.getBlendColor().getRGBComponents(null)[3]));
    				reflMap.alpha = reflectionMap.getBlendColor().getRGBComponents(null)[3];
    				hasReflectionMap = true;
    			}else{
    				hasReflectionMap = false;
    			}
    		}else if(v.getName().equals("textureMatrix")){
    			//do nothing
    		}else{
    			System.err.println(v.getType() + " " + v.getName() + " not implemented this type yet. have to do so in JOGLGeometryInstance.updateAppearance(...).");
    		}
    		//TODO other possible types, textures
    	}
		if(!hasTexture){
			texture.removeTexture();
		}
		if(!hasReflectionMap){
			reflMap.removeTexture();
		}
		return shader;
	}

	public abstract void updateAppearance(SceneGraphPath sgp, GL3 gl, boolean appChanged, boolean geomLengthChanged, boolean geomPosChanged);
}
