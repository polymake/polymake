package de.jreality.reader.vrml;
/**
 * @author gonska
 */

import java.awt.Color;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.Input;
//import de.jreality.scene.IndexedFaceSet;
//import de.jreality.ui.viewerapp.ViewerApp;

public class State {
	// konstanten
	public static enum Binding {
		DEFAULT,OVERALL,
		PER_PART,PER_PART_INDEXED,
		PER_FACE,PER_FACE_INDEXED,
		PER_VERTEX,PER_VERTEX_INDEXED
		,NONE // by Color:means default/overall with allready drawn color 
	}
//	public static final String[] BINDING= new String[]{	
//		"DEFAULT","OVERALL",
//		"PER_PART","PER_PART_INDEXED",
//		"PER_FACE","PER_FACE_INDEXED",
//		"PER_VERTEX","PER_VERTEX_INDEXED","NONE"};// none means it is allready set in App
	//TODO none benutzen und overall color auslagern in App mitten im baum
	public static final String[] VERTORDER= new String[]{
		"UNKNOWN_ORDERING","CLOCKWISE","COUNTERCLOCKWISE"};
	public static final String[] SHAPETYPE= new String[]{
		"UNKNOWN_SHAPE_TYPE","SOLID"};
	public static final String[] FACETYPE= new String[]{
		"UNKNOWN_FACE_TYPE","CONVEX"};
	// data
	String history="";			// Info History String
	public Color[] ambient=new Color[]{};	// color 
	public Color[] diffuse= new Color[]{};
	public Color[] specular= new Color[]{};
	public Color[] emissive= new Color[]{}; 
	public double[] shininess= new double[]{};
	public double[] transparency= new double[]{};
	public Binding materialBinding=Binding.DEFAULT; 
	public Binding normalBinding=Binding.DEFAULT;
	public SceneGraphPath camPath=new SceneGraphPath();// Graph
	public SceneGraphComponent currNode=null;
	public double [][] coords= new double[0][3];	// 3d Daten
	public double [][] normals= new double[0][3];
	public Transformation trafo=null;// TODO auslagern in den Baum
	public Transformation extraGeoTrans=null;// fuer die Abmessungen der Geometrie
	public String textureFile="";				// TexturDaten TODO auslagern in den Baum
	public int[][][] textureData= new int[][][]{{{}}};// TODO auslagern in den Baum
	public int wrapS=0; // 0=Repeat 1=clamp
	public int wrapT=0; 
	public Matrix textureTrafo = MatrixBuilder.euclidean().getMatrix(); 
	public double [][] textureCoords=new double[][]{{0,0}};
	public int vertexDraw=0;// visibility // 0= inherit, 1= true, 2= false; 
	public int edgeDraw=0;
	public int faceDraw=0; 
	public int vertOrder=0;	// ShapeHints
	public int shapeType=0;
	public int faceType=0;
	public double creaseAngle = 0.5f;
	public int defTyp=DefUseData.NO_TYP;
	
//	-------------- construct --------------
	public State(){	}
	/**
	 * this is a deep-copyConstructor
	 * @param original
	 */
	public State(State orig){
		history= orig.history;
		int n;// color
		n=orig.ambient.length;	ambient= new Color[n];
		for (int i=0;i<n;i++)ambient[i]=new Color(orig.ambient[i].getRGB());
		n=orig.diffuse.length;	diffuse= new Color[n];
		for (int i=0;i<n;i++)diffuse[i]=new Color(orig.diffuse[i].getRGB());
		n=orig.emissive.length;	emissive= new Color[n];
		for (int i=0;i<n;i++)emissive[i]=new Color(orig.emissive[i].getRGB());
		n=orig.specular.length;	specular= new Color[n];
		for (int i=0;i<n;i++)specular[i]=new Color(orig.specular[i].getRGB());
		n=orig.shininess.length;shininess= new double[n];
		for (int i=0;i<n;i++)	shininess[i]=orig.shininess[i];
		n=orig.transparency.length;	transparency= new double[n];
		for (int i=0;i<n;i++)	transparency[i]=orig.transparency[i];
		materialBinding=orig.materialBinding;// bindings
		normalBinding=orig.normalBinding;
		camPath=orig.camPath;		// camPath
		currNode=orig.currNode;		//curr Node
		n=orig.coords.length;	coords= new double[n][];// coords
		for (int i=0;i<n;i++)
			coords[i]= new double[]{orig.coords[i][0],orig.coords[i][1],orig.coords[i][2]};
		n=orig.normals.length;	normals= new double[n][]; // normals
		for (int i=0;i<n;i++)
			normals[i]= new double[]{orig.normals[i][0],orig.normals[i][1],orig.normals[i][2]};
		if (orig.trafo==null)	trafo=null;				// transfo
		else {
			double[] mOld =orig.trafo.getMatrix();
			double[] m= new double[16];
			for (int i=0;i<16;i++)	m[i]=mOld[i];
			trafo= new Transformation(m);}
		if (orig.extraGeoTrans==null)	extraGeoTrans=null; // extraGeoTrans
		else {
			double[] mOld =orig.extraGeoTrans.getMatrix();
			double[] m= new double[16];
			for (int i=0;i<16;i++)	m[i]=mOld[i];
			extraGeoTrans= new Transformation(m);}
		textureFile=orig.textureFile;		// texture -file
		textureData= new int [][][]{{{}}};  // texture -Data
		if (orig.textureData.length>0 
				&& orig.textureData[0].length>0
				&& orig.textureData[0][0].length>0){
			int a=orig.textureData.length;
			int b=orig.textureData[0].length;
			int c=orig.textureData[0][0].length;
			textureData= new int[a][b][c];
			for (int i=0;i<a;i++)
				for(int j=0;j<b;j++)
					for(int k=0;k<c;k++)
						textureData[i][j][k]=orig.textureData[i][j][k];
		}
		wrapS=orig.wrapS;				// texture -wrap
		wrapT=orig.wrapT;
		double[] temp= new double[16];	// texture -trafo
		for (int i=0;i<16;i++)
			temp[i]=orig.textureTrafo.getArray()[i];
		textureTrafo=new Matrix(temp);
		if (orig.textureCoords.length>0 	// texture -coords
				&& orig.textureCoords[0].length>0){
			int a=orig.textureCoords.length;
			int b=orig.textureCoords[0].length;
			textureCoords= new double[a][b];
			for (int i=0;i<a;i++)
				for(int j=0;j<b;j++)
					textureCoords[i][j]=orig.textureCoords[i][j];
		}
		vertexDraw=orig.vertexDraw;	// visibility  
		edgeDraw=orig.edgeDraw;
		faceDraw=orig.faceDraw; 
		vertOrder=orig.vertOrder;	// ShapeHints
		shapeType=orig.shapeType;
		faceType=orig.faceType;
		creaseAngle = orig.creaseAngle;
	}
		
//	 -------------- get ---------------------	
	// TODO braucht das wer?
	public int colorLength(){
		int n=Math.max(ambient.length,diffuse.length);
		int m=Math.max(specular.length,emissive.length);
		return Math.max(n,m);
	}

	public static Binding getBinding(String bind){
		if(bind.equals("DEFAULT"))
			return Binding.DEFAULT;
		if(bind.equals("OVERALL"))
			return Binding.OVERALL;
		if(bind.equals("PER_PART"))
			return Binding.PER_PART;
		if(bind.equals("PER_PART_INDEXED"))
			return Binding.PER_PART_INDEXED;
		if(bind.equals("PER_FACE"))
			return Binding.PER_FACE;
		if(bind.equals("PER_FACE_INDEXED"))
			return Binding.PER_FACE_INDEXED;
		if(bind.equals("PER_VERTEX"))
			return Binding.PER_VERTEX;
		if(bind.equals("PER_VERTEX_INDEXED"))
			return Binding.PER_VERTEX_INDEXED;
		return Binding.NONE;}
	
//   -------------- set ----------------------
	/**
	 * setzt Farben und Transparenz und visibility
	 * in eine Appearance  
	 * @param useEmissive 
	 *   Emissive ist nicht implementiert.
	 *   Fuer Linien und Punkte wird die Farbe von emmissive
	 *   als diffuseColor gesetzt.
	 *   So werden Flaechen und Linien trotzdem getrennt gefaerbt.
	 * @return o.g. Appearance 
	 */

	public void setColorApp(Appearance a,boolean useEmissive){
		if (vertexDraw==1)
			a.setAttribute(CommonAttributes.VERTEX_DRAW,true);
		if (vertexDraw==2)
			a.setAttribute(CommonAttributes.VERTEX_DRAW,false);
		if (edgeDraw==1)
			a.setAttribute(CommonAttributes.EDGE_DRAW,true);
		if (edgeDraw==2)
			a.setAttribute(CommonAttributes.EDGE_DRAW,false);
		if (faceDraw==1)
			a.setAttribute(CommonAttributes.FACE_DRAW,true);
		if (faceDraw==2)
			a.setAttribute(CommonAttributes.FACE_DRAW,false);

		if(materialBinding!=Binding.OVERALL && materialBinding!=Binding.DEFAULT) return;
		//TODO: calculate shininess
		if (ambient.length>0){
			a.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.AMBIENT_COLOR,ambient[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.AMBIENT_COLOR,ambient[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.AMBIENT_COLOR,ambient[0]);
		 	a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.AMBIENT_COLOR, ambient[0]);			 	
			a.setAttribute(CommonAttributes.POINT_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.AMBIENT_COLOR,ambient[0]);
			}
		if (specular.length>0){
			a.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.SPECULAR_COLOR,specular[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.SPECULAR_COLOR,specular[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.SPECULAR_COLOR,specular[0]);
		 	a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.SPECULAR_COLOR, specular[0]);			 	
			a.setAttribute(CommonAttributes.POINT_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.SPECULAR_COLOR,specular[0]);
		}
		// VRML diffuse & emissive definition, if emissive is not supported:
		// take diffuse as base Color, take emissive if all others colors are empty
		if (diffuse.length>0){
			a.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,diffuse[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,diffuse[0]);
			a.setAttribute(CommonAttributes.LINE_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,diffuse[0]);
		 	a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, diffuse[0]);			 	
			a.setAttribute(CommonAttributes.POINT_SHADER+"."+
				 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,diffuse[0]);
		}
		else
			if (emissive.length>0){
				a.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,emissive[0]);
				a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,emissive[0]);
				a.setAttribute(CommonAttributes.LINE_SHADER+"."+
					 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,emissive[0]);
			 	a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, emissive[0]);			 	
				a.setAttribute(CommonAttributes.POINT_SHADER+"."+
					 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,emissive[0]);
			}
		// 
		if (transparency.length>0)
			if (transparency[0]!=1){
				a.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY,transparency[0]);
				a.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TRANSPARENCY,transparency[0]);
				a.setAttribute(CommonAttributes.LINE_SHADER+"."+
					 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY,transparency[0]);
			 	a.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.TRANSPARENCY, transparency[0]);			 	
				a.setAttribute(CommonAttributes.POINT_SHADER+"."+
					 	CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY,transparency[0]);
			}
	}
	/** Falls jeder Vertex Daten per Index erhaelt, muessen die Vertices mehrfach angelegt werden.  
	 */
	public boolean haveToSeparateVertices(){
		if (normalBinding  ==Binding.PER_VERTEX && normals.length>0) return true;
		if (normalBinding  ==Binding.PER_VERTEX_INDEXED && normals.length>0)return true;
		if (materialBinding ==Binding.PER_VERTEX && colorLength()>0) return true;
	    if (materialBinding ==Binding.PER_VERTEX_INDEXED && colorLength()>0) return true;
		if (!textureFile.equals(""))return true;
		if (textureData.length!=0&&textureData[0].length!=0 && textureData[0][0].length!=0)return true;
		return false;
	}
	
	/** sets the transformation resulting of trafo and 
	 * extraGeoTrans to the given SceneGraphComponent 
	 * @param sgc
	 */
	public void setTrafo(SceneGraphComponent sgc){
		if ((trafo!=null)|(extraGeoTrans!=null)){
			Transformation t= new Transformation();
			if (trafo!=null){
				t=trafo;
			}
			else{MatrixBuilder.euclidean().assignTo(t);}
			if(extraGeoTrans!=null)
				t.multiplyOnRight(extraGeoTrans.getMatrix());
			sgc.setTransformation(t);
		}
	}
	/** 
	 * sets the texture
	 * @param app
	 */ 
	public Appearance assignTexture(Appearance app, IndexedFaceSet f){
		// read Data:
		ImageData id;
		if (textureFile.equals("")){
			if (textureData.length==0||textureData[0].length==0||
					textureData[0][0].length==0)	return app;
			// uebersetze die Int[][][] nach byte[][]
			int w= textureData.length;
			int h= textureData[0].length;
			int dim= textureData[0][0].length;
			byte[] cols=new byte[w*h*4];
			for(int i=0;i<w;i++)
				for(int j=0;j<h;j++){
					if(dim==1){//grayscale
						cols[i*h*4+j*4+0]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+1]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+2]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+3]=(byte)-1;
					}						
					if(dim==2){//grayscale-Alpha
						cols[i*h*4+j*4+0]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+1]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+2]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+3]=(byte)textureData[i][j][1];
					}
					if(dim==3){//rgb
						cols[i*h*4+j*4+0]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+1]=(byte)textureData[i][j][1];
						cols[i*h*4+j*4+2]=(byte)textureData[i][j][2];
						cols[i*h*4+j*4+3]=(byte)-1;
					}
					if(dim==4){//rgb-Alpha
						cols[i*h*4+j*4+0]=(byte)textureData[i][j][0];
						cols[i*h*4+j*4+1]=(byte)textureData[i][j][1];
						cols[i*h*4+j*4+2]=(byte)textureData[i][j][2];
						cols[i*h*4+j*4+3]=(byte)textureData[i][j][3];						
					}
				}
			id=new ImageData(cols,w,h);
		}
		else{
			id=null;
			try {id = ImageData.load(Input.getInput(textureFile));}
			catch (Exception e) {}
	    }
		// fill in Data:
		if(app==null)app= new Appearance();
		double[][] texCoord = new double [f.getNumPoints()][];
		if (textureCoords.length<f.getNumPoints()) System.out.println("State.assignTexture() not enough Texturecoords");
		System.arraycopy(textureCoords,0,texCoord,0,f.getNumPoints());
		f.setVertexAttributes( Attribute.TEXTURE_COORDINATES,
				new DoubleArrayArray.Array( texCoord));
		app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,new Color(1f,1f,1f));
		app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY_ENABLED,false);
		Texture2D tex = TextureUtility.createTexture(app, CommonAttributes.POLYGON_SHADER,id);
	    tex.setTextureMatrix(textureTrafo);
	    if (wrapS==0)
	    	tex.setRepeatS(Texture2D.GL_REPEAT);
	    else tex.setRepeatS(Texture2D.GL_CLAMP);
	    if (wrapT==0)
	    	tex.setRepeatT(Texture2D.GL_REPEAT);
	    else tex.setRepeatT(Texture2D.GL_CLAMP);
	    tex.setApplyMode(Texture2D.GL_MODULATE);
		return app;
	}
}
