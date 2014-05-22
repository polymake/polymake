package de.jreality.writer;

/**
 * this is a vrml-writer.
 * it can be set up with these options:
 *   useDefs, 
 *   drawTubes, drawSpheres,
 *   moveLightsToSceneRoot 
 *   and    writeTextureFiles
 *   (describtion below)
 * @author gonska
 */

//TODO: moeglw. Camera falsch positioniert
//TODO: Labels sind noch unbehandelt

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.WeakHashMap;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Cylinder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.ImageUtility;


public class WriterVRML2{
	private boolean useDefs = true;
	private boolean drawTubes = false;
	private boolean drawSpheres = false;
	private boolean moveLightsToSceneRoot=true;
	private boolean writeTextureFiles = false;
	private boolean flipTextureUpsideDown = true;
	private boolean evaluateTextureMatrix =true;
	private boolean writeTextureCoordIndices=true;
	private boolean excludeTerrain = false;

	private VRMLWriterHelper wHelp= new VRMLWriterHelper();
	private DefaultGeometryShader dgs;
	private DefaultPolygonShader dps;
	private DefaultLineShader dls;
	private DefaultPointShader dvs;
	private DefaultTextShader dpts;
	private DefaultTextShader dlts;
	private DefaultTextShader dvts;
	private DefaultPolygonShader dlps;
	private DefaultPolygonShader dvps;
	private String fileStem = "unnamed";
	private PrintWriter out=null;
	private static final String spacing="  ";// for outlay
	private static String hist="";// for outlay
	private Color amb;
	private Color spec;
	private Color diff;
	private double tra;


	private static enum GeoTyp{TEX_FACE,FACE,TUBE,LINE,SPHERE,POINT}
	// -----------------constructor----------------------
	/** this Writer can write vrml2 */
	public WriterVRML2(File f) {
		try {
			out = new PrintWriter(new FileOutputStream(f));
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		setWritePath(f.getParent());
		String wrl = f.getName();
		int end = wrl.indexOf(".");
		if (end == -1) end = wrl.length();
		wrl = wrl.substring(0, end);
		setFileStem(wrl);
	}
	public WriterVRML2(OutputStream outS) {	
		out=new PrintWriter( outS );
	}
	/** this Writer can write vrml2 */
	private WriterVRML2(FileWriter outS) {
		out=new PrintWriter( outS );
	}
	/** this Writer can write vrml2 */
	private WriterVRML2(PrintWriter outS) {
		out=outS;
	}
	
	public void setFileStem(String s)	{
		fileStem = s;
	}
	/** writes a vrml2-file of the scene into the stream
	 * (use default settings)  
	 * @param sceneRoot
	 * @param stream
	 */
	private static void write(SceneGraphComponent sceneRoot, FileOutputStream stream)   {
		WriterVRML writer = new WriterVRML(stream);
		writer.write(sceneRoot);
	}
	String writePath = "";
	WeakHashMap<ImageData, String> textureMaps = new WeakHashMap<ImageData, String>();
	public void setWritePath(String path)	{
		writePath = path;
		if (!writePath.endsWith("/")) writePath = writePath + "/";
	}

//	---------------------------------------

	public void write( SceneGraphNode sgn ){
		out.println("#VRML V2.0 utf8");
		// what should be defined
		if(useDefs){
			wHelp.inspect(sgn);
		}
		if(moveLightsToSceneRoot){
			// collect Lights
			out.println(""+hist+"Group { # collected lights and sceneRoot" );
			String oldhist= hist;
			hist=hist+spacing;
			out.println(hist+"children [");
			hist=hist+spacing;

			sgn.accept(new MyLightVisitor());
			sgn.accept(new MyVisitor());
			hist=oldhist;
			out.println(""+hist+spacing+"]");			
			out.println(""+hist+"}");
		}
		else
			sgn.accept(new MyVisitor());
		out.flush();
	}
//	------------------ 
	private void updateShaders(EffectiveAppearance eap) {
		dgs = ShaderUtility.createDefaultGeometryShader(eap);

		if (dgs.getPointShader() instanceof DefaultPointShader){	
			dvs = (DefaultPointShader) dgs.getPointShader();
			if (dvs.getTextShader() instanceof DefaultTextShader){
				dvts=(DefaultTextShader)dvs.getTextShader();
			}
			else dvts=null;
			if (dvs.getPolygonShader() instanceof DefaultPolygonShader){
				dvps=(DefaultPolygonShader)dvs.getPolygonShader();
			}
			else dvps=null;
		}
		else dvs = null;
		if (dgs.getLineShader() instanceof DefaultLineShader){ 
			dls = (DefaultLineShader) dgs.getLineShader();
			if (dls.getTextShader() instanceof DefaultTextShader){
				dlts=(DefaultTextShader)dls.getTextShader();
			}
			else dlts=null;
			if (dls.getPolygonShader() instanceof DefaultPolygonShader){
				dlps=(DefaultPolygonShader)dls.getPolygonShader();
			}
			else dlps=null;
		}
		else dls = null;
		if (dgs.getPolygonShader() instanceof DefaultPolygonShader){
			dps = (DefaultPolygonShader) dgs.getPolygonShader();
			if (dps.getTextShader() instanceof DefaultTextShader)
				dpts=(DefaultTextShader) dps.getTextShader();
			else dpts=null;
		}
		else dps = null;


	}
//	---------------------
//	---------------------------- start writing --------------------
	// --------------- helper Classes ---------------------

	int textureCount = 0;

	private   void writeCoords(double[][] coords,String hist) {// done
		/*Coordinate {
		  exposedField MFVec3f point  []
		}*/
		out.println(hist+"Coordinate { point [");
		String hist2=hist+spacing;
		double[][] coords3 = coords;
		if (coords[0].length == 4)
			coords3 = Pn.dehomogenize(null, coords3);
		for(int i=0;i<coords.length;i++){
			VRMLWriterHelper.writeDoubleArray(coords3[i],hist2,",",3,out);
		}
		out.println(hist+"]}");
	}


	private  void writeColors(double[][] colors,String hist) {//done
		/*	Color {
		  exposedField MFColor color  []
		}*/
		out.println(hist+"Color { color [");
		String hist2=hist+spacing;
		for(int i=0;i<colors.length;i++){
			VRMLWriterHelper.writeDoubleArray(colors[i],hist2,",",3,out);
		}
		out.println(hist+"]}");
	}
	private   void writeNormals(double[][] normals,String hist) {//done
		/*Normal {
  			exposedField MFVec3f vector  []
		}*/
		out.println(hist+"Normal { vector [");
		String hist2=hist+spacing;
		for(int i=0;i<normals.length;i++){
			VRMLWriterHelper.writeDoubleArray(normals[i],hist2,",",3,out);
		}
		out.println(hist+"]}");
	}

	private  void writeIndices(int[][] in,String hist) {
		out.println(hist+"coordIndex ["); 		
		for (int i=0;i<in.length;i++){
			int le=in[i].length;
			out.print(hist+spacing);
			for(int j=0;j<le;j++){
				out.print(""+in[i][j]+", ");
			}
			out.println(" -1, ");
		}
		out.println(hist+"]");
	}
	private  void writeTexture(Texture2D tex){
		/*ImageTexture {
		  exposedField MFString url     []
		  field        SFBool   repeatS TRUE
		  field        SFBool   repeatT TRUE
		}*/
		/*PixelTexture {
			  exposedField SFImage  image      0 0 0
			  field        SFBool   repeatS    TRUE
			  field        SFBool   repeatT    TRUE
			}*/
		//// ---------------- old -----------------
		int[] channels = {1,2,3,0};
		try {
			Matrix mat= tex.getTextureMatrix();
			if (mat==null) throw new IOException("missing texture component");
			ImageData id=tex.getImage();
			if (id==null) throw new IOException("missing texture component");
			// write texture:
			String hist2=hist+spacing;
			if (writeTextureFiles)	{
				String fileName = textureMaps.get(tex.getImage());
				if (fileName == null)	{
					fileName = fileStem+String.format("-texture-%04d", textureCount)+".png";
					String fullName = writePath+fileName;
					textureCount++;
					BufferedImage image = (BufferedImage) tex.getImage().getImage();
					image = ImageUtility.getValidBufferedImage(tex.getImage());
					ImageUtility.writeBufferedImage( new File(fullName),image);				
					textureMaps.put(tex.getImage(), fileName);
				}
				out.println(hist+"ImageTexture { ");
				out.println(hist2+"url "+"\""+fileName+"\" ");
				writeTextureRepeat(hist, tex);
				out.println(hist+"}");
			} 
			else {
				out.println(hist+"PixelTexture { ");
				VRMLWriterHelper.writeImage(tex, hist2, out);
				writeTextureRepeat(hist2, tex);					
				out.println(hist+"}");
			}
		} catch (IOException e) {}

		//// ---------------- old end -----------------
	}
	private void writeTextureRepeat(String hist,Texture2D tex){
		out.print(hist+"repeatS ");
		if(tex.getRepeatS()==tex.REPEAT_S_DEFAULT)out.println("TRUE");
		else out.println("FALSE");
		out.print(hist+"repeatT ");
		if(tex.getRepeatT()==tex.REPEAT_S_DEFAULT)out.println("TRUE");
		else out.println("FALSE");
	}

	private  void writeTexCoords(double[][] texCoords,String hist) {//done
		/*TextureCoordinate {
			  exposedField MFVec2f point  []
			}*/
		if(evaluateTextureMatrix){
			Texture2D tex=dps.getTexture2d();
			if (tex != null && tex instanceof Texture2D) {
				Matrix mat= new Matrix(tex.getTextureMatrix().getArray());
				if(flipTextureUpsideDown){
					Matrix flip=MatrixBuilder.euclidean().translate(0,1,0).scale(1,-1,1).getMatrix();
//		Oops! I got my matrix arithmetic mixed up; this has to happen "LAST", before
//		the texture is looked up in the image
//					mat.multiplyOnRight(flip);
					mat.multiplyOnLeft(flip);
				}
				int dim=texCoords[0].length;
				for (int i = 0; i < texCoords.length; i++) {
					if ( dim == 2)	
						texCoords[i]=new double[]{texCoords[i][0],texCoords[i][1],0,1};
					if( dim == 3)
						texCoords[i]=new double[]{texCoords[i][0],texCoords[i][1],texCoords[i][2],1};
				}
				//mat.transpose();
				Rn.matrixTimesVector(texCoords, mat.getArray(), texCoords );
				Pn.dehomogenize(texCoords, texCoords);
				for (int i = 0; i < texCoords.length; i++)	{
					texCoords[i]=new double[]{texCoords[i][0],texCoords[i][1]};
				}				
			}
		}
		String hist2=hist+spacing;
		out.println(hist+"TextureCoordinate { point [");
		for(int i=0;i<texCoords.length;i++)
			VRMLWriterHelper.writeDoubleArray(texCoords[i],hist2,",",2,out);
		out.println(hist+"]}");
	}	
	private  void writeMaterial(GeoTyp typ){
		/*Material {
		  exposedField SFFloat ambientIntensity  0.2
		  exposedField SFColor diffuseColor      0.8 0.8 0.8
		  exposedField SFColor emissiveColor     0 0 0
		  exposedField SFFloat shininess         0.2
		  exposedField SFColor specularColor     0 0 0
		  exposedField SFFloat transparency      0
		}*/
		String histold= hist;
		hist=hist+spacing;
		out.println(hist+"Material { ");
		hist=hist+spacing;
		// have to set all colors at once, otherwise unset colors return to defaultValue
		switch (typ){
		case TEX_FACE:
		case FACE: 
		case SPHERE: 
		case TUBE:{
			if(spec!=null)	out.println(hist+"specularColor " + VRMLWriterHelper.ColorToString(spec) );
			if(tra!=0){ out.println(hist+"transparency " + tra );	}
			if(diff!=null)	out.println(hist+"diffuseColor " + VRMLWriterHelper.ColorToString(diff) );
			break;
		}
		case LINE:
		case POINT:
			if(diff!=null)	out.println(hist+"emissiveColor " + VRMLWriterHelper.ColorToString(diff) );
			break;
		}
		hist= histold;
		out.println(hist+spacing+"}");
	}
	private  void writeTextureTransform(Texture2D tex,GeoTyp typ){
		String hist2=hist+spacing;
		/*		TextureTransform {
			  exposedField SFVec2f center      0 0
			  exposedField SFFloat rotation    0
			  exposedField SFVec2f scale       1 1
			  exposedField SFVec2f translation 0 0
		}*/
		Matrix mat= new Matrix(tex.getTextureMatrix().getArray());
		if(flipTextureUpsideDown){
			Matrix flip=MatrixBuilder.euclidean().translate(0,1,0).scale(1,-1,1).getMatrix();
			mat.multiplyOnRight(flip);				
		}
		FactoredMatrix matrix= new FactoredMatrix(mat.getArray());
		double[] trans=matrix.getTranslation();
		double ang=matrix.getRotationAngle();
		double[] rotA=matrix.getRotationAxis();
		double[] scale = matrix.getStretch();
		if(ang>0){
			if(rotA[2]<0) ang=-ang;
		}
		out.println(hist+"TextureTransform {");
		out.println(hist2+"center 0 0");
		out.println(hist2+"rotation  "+ang);
		out.println(hist2+"scale "+scale[0]+" "+scale[1]);
		out.println(hist2+"translation  "+trans[0]/trans[3]+" "+trans[1]/trans[3]);
		out.println(hist+"}");
	}
	private void writeApp(GeoTyp typ){
		String histOld=hist;
		String hist2=hist+spacing;
		String hist3=hist2+spacing;
		out.println(hist+"Appearance { ");
		hist=hist2;
		out.println(hist+"material ");
		hist=hist3;
		writeMaterial(typ);
		hist=hist2;
		out.println(hist+"texture ");
		hist=hist3;
		Texture2D tex=dps.getTexture2d();
		if (tex!=null&&typ==GeoTyp.TEX_FACE){
			writeTexture(tex);
			hist=hist2;
			out.println(hist+"textureTransform ");
			hist=hist3;
			if(!evaluateTextureMatrix)
				writeTextureTransform(tex,typ);
		}
		hist=histOld;
		out.println(hist+"} ");		
	}
	/** wraps the geometry in a shapeNode which supports an appearance
	 *  iff an appearance is given or forcedDiffuseColor is not null.
	 *  forcedDiffuseColor will be used (if not null) instead of diffusecolor from 
	 *  EffectiveAppearance    
	 * @param typ
	 * @param forcedDiffuseColor
	 * @return
	 */
	private boolean tryWriteShapeNode(GeoTyp typ,double[] forcedDiffuseColor){
		switch (typ){
		case TEX_FACE:
		case FACE:{
			amb = dps.getAmbientColor();
			spec= dps.getSpecularColor();
			diff= dps.getDiffuseColor();
			tra= dps.getTransparency();			
			if(tra==0&&diff==null&&forcedDiffuseColor==null&&spec==null&&amb==null) return false;
			break;
		}
		case TUBE:{
			amb = dlps.getAmbientColor();
			spec= dlps.getSpecularColor();
			diff= dlps.getDiffuseColor();
			tra= dlps.getTransparency();
			if(tra==0&&diff==null&&forcedDiffuseColor==null&&spec==null&&amb==null) return false;
			break;
		}
		case LINE:{
			diff= dls.getDiffuseColor();
			if(diff==null) return false;
			break;
		}
		case SPHERE:{
			amb = dvps.getAmbientColor();
			spec= dvps.getSpecularColor();
			diff= dvps.getDiffuseColor();
			tra= dvps.getTransparency();
			if(tra==0&&diff==null&&forcedDiffuseColor==null&&spec==null&&amb==null) return false;
			break;
		}
		case POINT:{
			diff= dvs.getDiffuseColor();
			if(diff==null) return false;
			break;
		}
		}
		if(forcedDiffuseColor!=null)
			diff=VRMLWriterHelper.DoublesToColor(forcedDiffuseColor);
		writeShapeNode(typ);
		return true;
	}
	private void closeShapeNode(String oldHist){
		hist=oldHist;
		out.println(hist+"}");// close shapeNode
	}
	private void writeShapeNode(GeoTyp typ){
		String hist2=hist+spacing;
		String hist3=hist2+spacing;
		out.println(hist+"Shape { ");
		hist=hist2;
		out.println(hist+"appearance ");
		hist=hist3;
		writeApp(typ);
		hist=hist2;
		out.println(hist+"geometry ");
		hist=hist3;// will be closed with closeShapeNode()
	}

	private void writeTrafo(Transformation trafo){
		FactoredMatrix fm= new FactoredMatrix(trafo.getMatrix());
		fm.update();
		double[] cen=fm.getCenter();
		if(cen!=null){
			out.println(hist+spacing+"center ");
			VRMLWriterHelper.writeDoubleArray(cen, hist, "", 3, out);
		}
		out.println(hist+spacing+"rotation ");
		VRMLWriterHelper.writeDoubleArray(fm.getRotationAxis(), hist, "  "+fm.getRotationAngle(), 3, out);
		out.println(hist+spacing+"scale ");
		VRMLWriterHelper.writeDoubleArray(fm.getStretch(), hist, "", 3, out);
		out.println(hist+spacing+"translation ");
		VRMLWriterHelper.writeDoubleArray(fm.getTranslation(), hist, "", 3, out);
	}
	private void writeDirLight(DirectionalLight l,String hist,PrintWriter out, double[] dir){
		/*DirectionalLight {
//		exposedField SFFloat ambientIntensity  0 
		exposedField SFColor color             1 1 1
		exposedField SFVec3f direction         0 0 -1
		exposedField SFFloat intensity         1 
//		exposedField SFBool  on                TRUE 
	}*/

		double di=l.getIntensity();
		double[] dc=VRMLWriterHelper.colorToDoubleArray(l.getColor());
		out.println(hist+"DirectionalLight { # "+ l.getName());
		String oldHist= hist;
		hist=hist+spacing;
		out.println(hist + "intensity " +di);
		out.print(hist + "color " );
		VRMLWriterHelper.writeDoubleArray(dc, "", "", 3,out);
		if(dir==null)
			out.println(hist + "direction  0 0 1");
		else{
			out.print(hist + "direction ");
			VRMLWriterHelper.writeDoubleArray(dir, "", "", 3,out);
		}
		hist=oldHist;
		out.println(hist+"}");
	}
	private void writePointLight(PointLight l,String hist,PrintWriter out,double[] location){
		/*PointLight {
//		  exposedField SFFloat ambientIntensity  0 
//		  exposedField SFVec3f attenuation       1 0 0
		  exposedField SFColor color             1 1 1 
		  exposedField SFFloat intensity         1
//		  exposedField SFVec3f location          0 0 0
//		  exposedField SFBool  on                TRUE 
//		  exposedField SFFloat radius            100
		}*/
		double di=l.getIntensity();
		double[] dc=VRMLWriterHelper.colorToDoubleArray(l.getColor());
		out.println(hist+"PointLight { # "+ l.getName());
		String oldHist= hist;
		hist=hist+spacing;
		out.println(hist + "intensity " +di);
		if(location!=null){
			out.print(hist + "location ");
			VRMLWriterHelper.writeDoubleArray(location, "", "", 3,out);
		}
		out.print(hist + "color " );
		VRMLWriterHelper.writeDoubleArray(dc, "", "", 3,out);
		hist=oldHist;
		out.println(hist+"}");			
	}
	private void writeSpotLight(SpotLight l,String hist,PrintWriter out,double[] location, double[] dir){

		/*SpotLight {
//			  exposedField SFFloat ambientIntensity  0 
//			  exposedField SFVec3f attenuation       1 0 0
			  exposedField SFFloat beamWidth         1.570796
			  exposedField SFColor color             1 1 1 
			  exposedField SFFloat cutOffAngle       0.785398
			  exposedField SFVec3f direction         0 0 -1
			  exposedField SFFloat intensity         1  
//			  exposedField SFVec3f location          0 0 0  
//			  exposedField SFBool  on                TRUE
//			  exposedField SFFloat radius            100 
			}*/
		double di=l.getIntensity();
		double[] dc=VRMLWriterHelper.colorToDoubleArray(l.getColor());
		out.println(hist+"SpotLight { # "+ l.getName());
		String oldHist= hist;		hist=hist+spacing;
		out.println(hist + "intensity " +di);
		out.print(hist + "color " );
		VRMLWriterHelper.writeDoubleArray(dc, "", "", 3,out);
		if(dir!=null){
			out.print(hist + "direction ");
			VRMLWriterHelper.writeDoubleArray(dir, "", "", 3,out);
		}
		if(location!=null){
			out.print(hist + "location ");
			VRMLWriterHelper.writeDoubleArray(location, "", "", 3,out);
		}
		out.println(hist + "beamWidth "+(l.getConeAngle()-l.getConeDeltaAngle()) );
		out.println(hist + "cutOffAngle "+l.getConeAngle() );
		hist=oldHist;
		out.println(hist+"}");

	}

	// -------------- Visitor -------------------
	private class MyVisitor extends SceneGraphVisitor{
		protected EffectiveAppearance effApp= EffectiveAppearance.create();

		public MyVisitor() {}
		public MyVisitor(MyVisitor mv) {
			effApp=mv.effApp;
		}		
		public void visit(SceneGraphComponent c) {// fin
			if(!c.isVisible())return;
			if(excludeTerrain && c.getName().equals("backdrop"))return;
			Transformation trafo= c.getTransformation();
			if (trafo!=null){
				out.println(""+hist+"Transform { # "+c.getName());
				writeTrafo(trafo);
			}
			else{
				out.println(""+hist+"Group { # "+c.getName());
			}
			String oldhist= hist;
			hist=hist+spacing;
			out.println(hist+"children [");
			hist=hist+spacing;
			c.childrenAccept(new MyVisitor(this));
			super.visit(c);
			hist=oldhist;
			out.println(""+hist+spacing+"]");			
			out.println(""+hist+"}");
		}
		public void visit(Appearance a) {// fin
			effApp=effApp.create(a);
			super.visit(a);
		}
		// ----- geometrys -----
		public void visit(Sphere s) {//done
			super.visit(s);
			if ( !dgs.getShowFaces())	return;
			String histOld= hist;
			boolean hasShapeNode=tryWriteShapeNode(GeoTyp.FACE,null);
			out.println(hist+"Sphere { radius  1}");
			if(hasShapeNode) closeShapeNode(histOld);
		}
		public void visit(Cylinder c) {//done
			super.visit(c);
			if ( !dgs.getShowFaces())	return;
			String histOld= hist;
			boolean hasShapeNode=tryWriteShapeNode(GeoTyp.FACE,null);
			out.print(hist+"Cylinder { ");
			out.print("bottom    FALSE ");
			out.print("top    FALSE ");
//			out.print("radius  1 ");
//			out.print("height  2 ");
			out.println("}");
			if(hasShapeNode) closeShapeNode(histOld);
		}
		public void visit(Geometry g) {//done
			updateShaders(effApp);
			super.visit(g);
		}

		public void visit(PointSet p) {
			super.visit(p);
			if ( !dgs.getShowPoints()|| p.getNumPoints()==0) return;
			String histOld= hist;
			GeoTyp typ=(dvs.getSpheresDraw()&&drawSpheres)?(GeoTyp.SPHERE):(GeoTyp.POINT);
			if(typ==GeoTyp.SPHERE){
				double[][] coords=VRMLWriterHelper.getDoubleDoubleVertexAttr(p, Attribute.COORDINATES);
				double[][] colors=VRMLWriterHelper.getDoubleDoubleVertexAttr(p, Attribute.COLORS);
				double radius=dvs.getPointRadius();
				double [] forcedDiffuseColor=null;
				String hist2=histOld+spacing;
				String hist3=hist2+spacing;
				hist=hist3;
				for(int i=0;i<coords.length;i++){
					out.println(""+histOld+"Transform { # sphere of PointSet ");
					out.println(hist2+"translation ");
					VRMLWriterHelper.writeDoubleArray(coords[i], hist2, "", 3, out);
					out.println(hist2+"children [ ");
					if(colors!=null)
						forcedDiffuseColor=colors[i];
					boolean hasShapeNode=tryWriteShapeNode(typ,forcedDiffuseColor);
					out.println(hist+"Sphere { radius "+radius+" }");
					if(hasShapeNode) closeShapeNode(hist3);
					out.println(hist2+"]");// children
					out.println(histOld+"}");// transform
				}
				return;
			}
			boolean hasShapeNode=tryWriteShapeNode(typ,null);
			//-------------------------------
			//	check if allready defined
			if(useDefs){
				if (wHelp.isDefinedPointSet(p)){
					out.println(""+hist+"USE "+ VRMLWriterHelper.str( p.hashCode()+"POINT")+" ");
					if(hasShapeNode) closeShapeNode(histOld);
					return;
				}
			}
			if (useDefs && wHelp.isMultipleUsedPointSet(p)){
				out.print(""+hist+"DEF "+VRMLWriterHelper.str(p.hashCode()+"POINT")+" ");
				wHelp.setDefinedPointSet(p);
			}
			else out.print(""+hist);
			// write object:
			/*PointSet {
				  exposedField  SFNode  color      NULL
				  exposedField  SFNode  coord     NULL
				}*/
			String hist2=hist;
			hist=hist+spacing;
			out.println("PointSet { # "+ p.getName());
			double[][] coords=VRMLWriterHelper.getDoubleDoubleVertexAttr(p, Attribute.COORDINATES);
			if(coords!=null){
				out.println(hist+"coord ");
				writeCoords(coords, hist+spacing);
			}
			// writes Vertex Colors
			double[][] colors=VRMLWriterHelper.getDoubleDoubleVertexAttr(p, Attribute.COLORS);
			if(colors!=null)
				if(colors.length>0){
					out.println(hist+"color ");
					writeColors(colors,hist+spacing);
				}
			hist=hist2;
			out.println(hist+"}");
			///old end -------------------
			if(hasShapeNode) closeShapeNode(histOld);
			/// --------------- Labels -------------------
//			if(dvts.getShowLabels())
//			if (p.getVertexAttributes(Attribute.LABELS)!=null){
//			String[] labels=p.getVertexAttributes(Attribute.LABELS).toStringArray(null);
//			writeLabelsAtPoints(coords, labels, hist);
//			}
		}
		public void visit(IndexedLineSet g) {
			super.visit(g);
			if ( !dls.getTubeDraw() || !dgs.getShowLines() || g.getNumEdges()==0) return;
			String histOld= hist;
			GeoTyp typ=(dls.getTubeDraw()&&drawTubes)?(GeoTyp.TUBE):(GeoTyp.LINE);
			if(typ==GeoTyp.TUBE){
				double[][] coords=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.COORDINATES);
				double[][] colors=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.COLORS);
				int[][] indices=VRMLWriterHelper.getIntIntEdgeAttr(g, Attribute.INDICES);
				if(indices==null)return;
				double radius=dls.getTubeRadius();
				double [] forcedDiffuseColor=null;
				String hist2=histOld+spacing;
				String hist3=hist2+spacing;
				hist=hist3;
				for(int[] line :indices){
					for(int i=1;i<line.length;i++){
						double[] v=coords[line[i-1]];
						double[] w=coords[line[i]];
						out.println(""+histOld+"Transform { # tubes of LineSet ");
						Transformation cylTrafo= new Transformation(VRMLWriterHelper.calcCylinderMatrix(v, w, radius));
						writeTrafo(cylTrafo);
						out.println(hist2+"children [ ");
						if(colors!=null)
							forcedDiffuseColor=colors[i];
						boolean hasShapeNode=tryWriteShapeNode(typ,forcedDiffuseColor);
						out.print(hist+"Cylinder { ");
						out.print("bottom    FALSE ");
						out.print("top    FALSE ");
//						out.print("radius  1 ");// ist default
//						out.print("height  2 ");// ist default
						out.println(" } ");
						if(hasShapeNode) closeShapeNode(hist3);
						out.println(hist2+"]");// children
						out.println(histOld+"}");// transform
					}
				}
				return;
			}
			boolean hasShapeNode=tryWriteShapeNode(typ,null);
			//-------------------------------
			//	check if allready defined
			if(useDefs){
				if (wHelp.isDefinedLineSet(g)){
					out.println(""+hist+"USE "+ VRMLWriterHelper.str( g.hashCode()+"LINE")+" ");
					if(hasShapeNode) closeShapeNode(histOld);
					return;
				}
			}
			if (useDefs && wHelp.isMultipleUsedLineSet(g)){
				out.print(""+hist+"DEF "+VRMLWriterHelper.str(g.hashCode()+"LINE")+" ");
				wHelp.setDefinedLineSet(g);
			}
			else out.print(""+hist);
			// write object:
			/*IndexedLineSet {
 //				eventIn       MFInt32 set_colorIndex
 //				eventIn       MFInt32 set_coordIndex
  				exposedField  SFNode  color             NULL
  				exposedField  SFNode  coord             NULL
// 				field         MFInt32 colorIndex        []
 //				field         SFBool  colorPerVertex    TRUE
 //				field         MFInt32 coordIndex        []
			}*/
			String hist2=hist;
			hist=hist+spacing;
			out.println("IndexedLineSet { # "+ g.getName());
			double[][] coords=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.COORDINATES);
			if(coords!=null){
				out.println(hist+"coord ");
				writeCoords(coords, hist+spacing);
			}
			int[][] indis=VRMLWriterHelper.getIntIntEdgeAttr(g, Attribute.INDICES);
			if(indis!=null)	writeIndices(indis, hist+spacing);
			double[][] colors=VRMLWriterHelper.getDoubleDoubleEdgeAttr(g, Attribute.COLORS);
			if(colors!=null)
				if(colors.length>0){
					out.println(hist+"color ");
					writeColors(colors,hist+spacing);
				}
			hist=hist2;
			out.println(hist+"}");
			///old end -------------------
			if(hasShapeNode) closeShapeNode(histOld);
			/// --------------- Labels -------------------
//			if(dvts.getShowLabels())
//			if (p.getVertexAttributes(Attribute.LABELS)!=null){
//			String[] labels=p.getVertexAttributes(Attribute.LABELS).toStringArray(null);
//			writeLabelsAtPoints(coords, labels, hist);
//			}
		}
		public void visit(IndexedFaceSet g) {//done
			super.visit(g);
			if ( !dgs.getShowFaces()|| g.getNumFaces()==0)	return;
			String histOld= hist;
			GeoTyp typ= GeoTyp.FACE;
			double[][] textCoords=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.TEXTURE_COORDINATES);
			if(textCoords!=null)
				typ=GeoTyp.TEX_FACE;
			boolean hasShapeNode=tryWriteShapeNode(typ,null);
			//	check if allready defined
			if(useDefs){
				if (wHelp.isDefinedFaceSet(g)){
					out.println(""+hist+"USE "+ VRMLWriterHelper.str( g.hashCode()+"POLYGON")+" ");
					if(hasShapeNode) closeShapeNode(histOld);
					return;
				}
			}
			if (useDefs && wHelp.isMultipleUsedFaceSet(g)){
				out.print(""+hist+"DEF "+VRMLWriterHelper.str(g.hashCode()+"POLYGON")+" ");
				wHelp.setDefinedFaceSet(g);
			}
			else out.print(""+hist);
			String hist2=hist;
			hist=hist+spacing;
			/*IndexedFaceSet {
// 				eventIn       MFInt32 set_colorIndex
// 				eventIn       MFInt32 set_coordIndex
// 				eventIn       MFInt32 set_normalIndex
// 				eventIn       MFInt32 set_texCoordIndex
  				exposedField  SFNode  color             NULL
  				exposedField  SFNode  coord             NULL
  				exposedField  SFNode  normal            NULL
  				exposedField  SFNode  texCoord          NULL
//  			field         SFBool  ccw               TRUE
//  			field         MFInt32 colorIndex        []
  				field         SFBool  colorPerVertex    TRUE
 				field         SFBool  convex            TRUE
  				field         MFInt32 coordIndex        []
// 				field         SFFloat creaseAngle       0
// 				field         MFInt32 normalIndex       []
  				field         SFBool  normalPerVertex   TRUE
  				field         SFBool  solid             TRUE
// 				field         MFInt32 texCoordIndex     []
			}*/
			out.println("IndexedFaceSet { # "+ g.getName());
			// write coordinates
			double[][] coords=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.COORDINATES);
			if(coords!=null){
				out.println(hist+"coord ");
				writeCoords(coords, hist+spacing);
			}
			// writes Indices			
			int[][] indis=VRMLWriterHelper.getIntIntFaceAttr(g, Attribute.INDICES);
			if(indis!=null) writeIndices(indis, hist);
			// write Texture coordinates
			if(textCoords!=null){
				out.println(hist+"texCoord ");
				writeTexCoords(textCoords, hist+spacing);
				if(writeTextureCoordIndices)
					out.println(hist+"texCoordIndex ");
			}
			// writes Colors
			double[][] vertColors=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.COLORS);
			double[][] faceColors=VRMLWriterHelper.getDoubleDoubleFaceAttr(g, Attribute.COLORS);
			if(dps.getSmoothShading()&&vertColors!=null){
				out.println(hist+"color ");
				writeColors(vertColors,hist+spacing);
				out.println(hist+"colorPerVertex TRUE");
			}
			else if(faceColors!=null){
				out.println(hist+"color ");
				writeColors(faceColors,hist+spacing);
				out.println(hist+"colorPerVertex FALSE");
			}
			// writes normals
			double[][] vertNormals=VRMLWriterHelper.getDoubleDoubleVertexAttr(g, Attribute.NORMALS);
			double[][] faceNormals=VRMLWriterHelper.getDoubleDoubleFaceAttr(g,Attribute.NORMALS);
			if(dps.getSmoothShading()&&vertNormals!=null){
				out.println(hist+"normal ");
				writeNormals(vertNormals, hist+spacing);
				out.println(hist+"normalPerVertex TRUE");
			}
			else if(faceNormals!=null){
				out.println(hist+"normal ");
				writeNormals(faceNormals, hist+spacing);
				out.println(hist+"normalPerVertex FALSE");
			}
			out.println(hist+"convex FALSE solid FALSE ");
			hist=hist2;
			out.println(hist+"}");
			///old end -------------------
			if(hasShapeNode) closeShapeNode(histOld);
			/// --------------- Labels? -------------------
//			if(dvts.getShowLabels())
//			if (p.getVertexAttributes(Attribute.LABELS)!=null){
//			String[] labels=p.getVertexAttributes(Attribute.LABELS).toStringArray(null);
//			writeLabelsAtPoints(coords, labels, hist);
//			}
		}
		// ---- Lights ----
		public void visit(Light l) {
			super.visit(l);
		}
		public void visit(DirectionalLight l) {
			if(moveLightsToSceneRoot)return;
			writeDirLight(l,hist,out,null);
			super.visit(l);
		}
		public void visit(PointLight l) {
			if(moveLightsToSceneRoot)return;
			if(!(l instanceof SpotLight)){
				writePointLight(l, hist, out, null);
			}
			super.visit(l);
		}
		public void visit(SpotLight l) {
			if(moveLightsToSceneRoot)return;
			writeSpotLight(l, hist, out, null,null);
			super.visit(l);
		}
		// ----------- cam ------------
		public void visit(Camera c) {
			/*Viewpoint {
//			  eventIn      SFBool     set_bind
			  exposedField SFFloat    fieldOfView    0.785398
//			  exposedField SFBool     jump           TRUE
			  exposedField SFRotation orientation    0 0 1  0
			  exposedField SFVec3f    position       0 0 10
//			  field        SFString   description    ""
//			  eventOut     SFTime     bindTime
//			  eventOut     SFBool     isBound
			}*/
			out.println(hist+"Viewpoint { ");
			String oldHist= hist;		hist=hist+spacing;
			out.println(hist+"fieldOfView "+c.getFieldOfView()*Math.PI/180);

			// ---------------------

			double[] m=c.getOrientationMatrix();
			FactoredMatrix fm= new FactoredMatrix(m);
			fm.update();
			double[] rotAx=fm.getRotationAxis();
			double ang=fm.getRotationAngle();
			double[] pos=fm.getTranslation();
			out.println(hist + "position 0 0 0");
			// ---------------------
			hist=oldHist;
			out.println(hist+"}");
			super.visit(c);
		}
		// ---------- trafo ---------
		public void visit(Transformation t) {//fin
			// handled in SceneGraphComponent
			super.visit(t);
		}
	}
	private class MyLightVisitor extends SceneGraphVisitor{
		SceneGraphPath p= new SceneGraphPath();
		public MyLightVisitor() {}
		public void visit(SceneGraphComponent c) {// fin
			if(!c.isVisible())return;
			p.push(c);
			c.childrenAccept(this);
			super.visit(c);
			p.pop();
		}
		public void visit(DirectionalLight l) {
			FactoredMatrix fm= new FactoredMatrix(p.getMatrix(null));
			fm.update();
			double[] dir=fm.getRotation().multiplyVector(new double[]{0,0,-1,0});
			writeDirLight(l,hist,out,dir);
			super.visit(l);
		}
		public void visit(PointLight l) {
			if(!(l instanceof SpotLight)){
				FactoredMatrix fm= new FactoredMatrix(p.getMatrix(null));
				fm.update();
				double[] c=fm.getTranslation();
				if(c!=null)
					c=new double[]{c[0],c[1],c[2]};
				writePointLight(l, hist, out, c);
			}
			super.visit(l);
		}
		public void visit(SpotLight l) {
			if(moveLightsToSceneRoot){
				FactoredMatrix fm= new FactoredMatrix(p.getMatrix(null));
				fm.update();
				double[] c=fm.getTranslation();
				double[] dir=fm.getRotation().multiplyVector(new double[]{0,0,-1,0});
				writeSpotLight(l, hist, out, c,dir);
			}
			super.visit(l);
		}
	}
	//--------------------getter&setter-------------
	/** sets if a texture wil be written into a file, instead of writing the data directly into the vrml-file. */
	public void setWriteTextureFiles(boolean writeTextureFiles2) {
		writeTextureFiles = writeTextureFiles2;
	}
	/** indicates if a texture wil be written into a file, instead of writing the data directly into the vrml-file. */
	public boolean isWriteTextureFiles() {
		return writeTextureFiles;
	}
	/** indicates if a vertex will be written as the primitive Sphere, if Spheredraw is enabled */
	public boolean isDrawSpheres() {
		return drawSpheres;
	}
	/** indicates if a linesegment will be written as the primitive Tube, if Tubedraw is enabled */
	public boolean isDrawTubes() {
		return drawTubes;
	}
	/** indicates if all ligths will be written into the sceneRoot-node instead
	 *  of their correct place in the scene.
	 *  Use this if lights in the scene are used as global lights.   
	 */
	public boolean isMoveLightsToSceneRoot() {
		return moveLightsToSceneRoot;
	}
	/** indicates if Geometrys which have multiple places in the scenegraph
	 * will be written only ones and be refferenced multiple times, instead of multiple writings. 
	 */
	public boolean isUseDefs() {
		return useDefs;
	}
	/** sets if a vertex will be written as the primitive Sphere, if Spheredraw is enabled */
	public void setDrawSpheres(boolean drawSpheres) {
		this.drawSpheres = drawSpheres;
	}
	/** sets if a linesegment will be written as the primitive Tube, if Tubedraw is enabled */
	public void setDrawTubes(boolean drawTubes) {
		this.drawTubes = drawTubes;
	}
	/** sets if Geometrys which have multiple places in the scenegraph
	 * will be written only ones and be refferenced multiple times, instead of multiple writings. 
	 */
	public void setUseDefs(boolean useDefs) {
		this.useDefs = useDefs;
	}
	/** sets if all ligths will be written into the sceneRoot-node instead
	 *  of their correct place in the scene.
	 *  Use this if lights in the scene are used as global lights.   
	 */
	public void setMoveLightsToSceneRoot(boolean moveLightsToSceneRoot) {
		this.moveLightsToSceneRoot = moveLightsToSceneRoot;
	}
	/** indicates if textureMatrix information schould be assigned to the Texturecoordinates
	 * @return
	 */
	public boolean isEvaluateTextureMatrix() {
		return evaluateTextureMatrix;
	}
	/** indicates if the texture schould be fliped upside down (the correct orientation depends on the software) 
	 * @return
	 */
	public boolean isFlipTextureUpsideDown() {
		return flipTextureUpsideDown;
	}
	/** indicates if textureMatrix information schould be assigned to the Texturecoordinates
	 * @param evaluateTextureMatrix
	 */
	public void setEvaluateTextureMatrix(boolean evaluateTextureMatrix) {
		this.evaluateTextureMatrix = evaluateTextureMatrix;
	}
	/** indicates if the texture schould be fliped upside down (the correct orientation depends on the software) 
	 * @param flipTextureUpsideDown
	 */
	public void setFlipTextureUpsideDown(boolean flipTextureUpsideDown) {
		this.flipTextureUpsideDown = flipTextureUpsideDown;
	}
	/** indicates if texturecoordinate indices should be written extra.  
	 * @param writeTextureCoordIndices
	 */
	public void setWriteTextureCoordIndices(boolean writeTextureCoordIndices) {
		this.writeTextureCoordIndices = writeTextureCoordIndices;
	}
	/** indicates if texturecoordinate indices should be written extra.  
	 * @return
	 */
	public boolean isWriteTextureCoordIndices() {
		return writeTextureCoordIndices;
	}
	public void setExcludeTerrain(boolean excludeTerrain) {
		excludeTerrain = excludeTerrain;
	}
}
