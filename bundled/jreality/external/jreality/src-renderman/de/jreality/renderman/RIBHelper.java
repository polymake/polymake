package de.jreality.renderman;

import java.awt.Color;
import java.awt.Font;
import java.awt.image.BufferedImage;
import java.awt.image.renderable.ParameterBlock;
import java.beans.Statement;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;

import javax.imageio.ImageIO;
import javax.imageio.ImageWriter;

import de.jreality.backends.label.LabelUtility;
import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.renderman.shader.CustomPolygonShader;
import de.jreality.renderman.shader.DefaultPolygonShader;
import de.jreality.renderman.shader.RendermanShader;
import de.jreality.renderman.shader.SLShader;
import de.jreality.renderman.shader.TwoSidePolygonShader;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeCollection;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.LineShader;
import de.jreality.shader.PointShader;
import de.jreality.shader.PolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.ImageUtility;
import de.jreality.util.LoggingSystem;

public class RIBHelper {
	
	
	public static RendermanShader processShader(AttributeCollection shader, RIBVisitor ribv, String name){
		if(shader instanceof PolygonShader)
			return processPolygonShader((PolygonShader)shader, ribv, name);
		if(shader instanceof LineShader)
			return processLineShader((LineShader)shader, ribv, name);
		if(shader instanceof PointShader)
			return processPointShader((PointShader)shader, ribv, name);		
		else return null;
	}

	private static RendermanShader processPolygonShader(PolygonShader ps, RIBVisitor ribv, String name)	{
		RendermanShader rs = null;
		Color Cs = null;
		double transparency = 0.0;
		if (ps instanceof de.jreality.shader.DefaultPolygonShader)	{
			de.jreality.shader.DefaultPolygonShader dps = (de.jreality.shader.DefaultPolygonShader) ps;
			DefaultPolygonShader rdps = new DefaultPolygonShader(dps);
			rdps.setFromEffectiveAppearance(ribv, ribv.eAppearance, name);
			rs = rdps;
			Cs = dps.getDiffuseColor();
			transparency = dps.getTransparency();
			if (ribv.useOldTransparency)	{
				double alpha = Cs.getAlpha()/255.0;
				transparency = (1-alpha) * transparency;
			}
			ribv.cs=dps.getDiffuseColor();
			ribv.smooth = dps.getSmoothShading();
		} 
		else if (ps instanceof de.jreality.shader.TwoSidePolygonShader)	{
			de.jreality.shader.TwoSidePolygonShader dps = (de.jreality.shader.TwoSidePolygonShader) ps;
			TwoSidePolygonShader rdps = new TwoSidePolygonShader(dps);
			rdps.setFromEffectiveAppearance(ribv, ribv.eAppearance, name);
			rs = rdps;
			de.jreality.shader.DefaultPolygonShader dpss = ((de.jreality.shader.DefaultPolygonShader)dps.getFront());
			Cs = dpss.getDiffuseColor();
			transparency = dpss.getTransparency();
			if (ribv.useOldTransparency)	{
				double alpha = Cs.getAlpha()/255.0;
				transparency = (1-alpha) * transparency;
			}
			ribv.cs=dpss.getDiffuseColor();
			ribv.smooth = dpss.getSmoothShading();
			// TODO figure out how to read out a reasonable "smooth" and "cs" value from this shader
		}
		else {
			LoggingSystem.getLogger(ShaderUtility.class).warning("Unknown shader class "+ps.getClass());
		}
		
		//overwrite DefaultPolygonShader if there is a surface-SLShader set
		SLShader orig = new SLShader("orig");
	    Object foo = ribv.eAppearance.getAttribute(name+"."+CommonAttributes.RMAN_SURFACE_SHADER,orig);
	    if (!(foo == Appearance.DEFAULT || !(foo instanceof SLShader) || foo == orig)){
	    	rs=new CustomPolygonShader();
	    	rs.setFromEffectiveAppearance(ribv,ribv.eAppearance, name);	    	
	    }		
		
		float[] csos = extractCsOs(Cs, 
				(!(ribv.handlingProxyGeometry && ribv.opaqueTubes) && ribv.transparencyEnabled) ? transparency : 0f,
				ribv.useOldTransparency);
		ribv.ri.color(csos);
		ribv.ri.shader(rs);
		
		return rs;
	}
	
	private static RendermanShader processLineShader(LineShader ls, RIBVisitor ribv, String name)	{
		RendermanShader rs = null;
		Color Cs = null;
		double transparency = 0.0;
		if (ls instanceof de.jreality.shader.DefaultLineShader)	{
			de.jreality.shader.DefaultLineShader dls=(de.jreality.shader.DefaultLineShader)ls;
			ribv.drawTubes=dls.getTubeDraw();
			ribv.tubeRadius=new Float(dls.getTubeRadius()).floatValue();
			ribv.cs=dls.getDiffuseColor();
			if(dls.getTubeDraw()){				
				return processPolygonShader(dls.getPolygonShader(), ribv, name+".polygonShader");
			}
			else{				
				Cs=dls.getDiffuseColor();
				//ribv.tubeRadius=new Float(dls.getLineWidth()).floatValue();				
				Appearance slApp=new Appearance();
				SLShader sls=new SLShader("constant");				
				slApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER,"free");
				slApp.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.RMAN_SURFACE_SHADER, sls);
				EffectiveAppearance slEApp=EffectiveAppearance.create();
				slEApp=slEApp.create(slApp);
				rs=new CustomPolygonShader();
				rs.setFromEffectiveAppearance(ribv, slEApp, "lineShader");		
			}
		}else {
			LoggingSystem.getLogger(ShaderUtility.class).warning("Unknown shader class "+ls.getClass());
		}
		float[] csos = extractCsOs(Cs, 
				(!(ribv.handlingProxyGeometry && ribv.opaqueTubes) && ribv.transparencyEnabled) ? transparency : 0f,
				ribv.useOldTransparency);
		ribv.ri.color(csos);
		ribv.ri.shader(rs);
		
		return rs;
	}
	
	private static RendermanShader processPointShader(PointShader vs, RIBVisitor ribv, String name)	{
		RendermanShader rs = null;
		Color Cs = null;
		double transparency = 0.0;
		if (vs instanceof de.jreality.shader.DefaultPointShader)	{
			de.jreality.shader.DefaultPointShader dvs=(de.jreality.shader.DefaultPointShader)vs;			
			ribv.drawSpheres=dvs.getSpheresDraw();
			double rad = dvs.getPointRadius();
			ribv.pointRadius= (float) rad;
			ribv.cs=dvs.getDiffuseColor();
			if(dvs.getSpheresDraw()){			
				return processPolygonShader(dvs.getPolygonShader(), ribv, name+".polygonShader");
			}else{
				Cs=dvs.getDiffuseColor();
				//ribv.pointRadius=new Float(dvs.getPointSize()).floatValue();				
				Appearance slApp=new Appearance();
				SLShader sls=new SLShader("constant");				
				slApp.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER,"free");
				slApp.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.RMAN_SURFACE_SHADER, sls);
				EffectiveAppearance slEApp=EffectiveAppearance.create();
				slEApp=slEApp.create(slApp);
				rs=new CustomPolygonShader();
				rs.setFromEffectiveAppearance(ribv, slEApp, "pointShader");
			}
		}else {
			LoggingSystem.getLogger(ShaderUtility.class).warning("Unknown shader class "+vs.getClass());
		}
		float[] csos = extractCsOs(Cs, 
				(!(ribv.handlingProxyGeometry && ribv.opaqueTubes) && ribv.transparencyEnabled) ? transparency : 0f,
				ribv.useOldTransparency);
		ribv.ri.color(csos);
		ribv.ri.shader(rs);
		
		return rs;
	}
	
	
	protected static float[] extractCsOs(Color color, double transparency, boolean useOldTransparency)	{
		float[] csos = new float[4];
		float colorAlpha = 1.0f;
		if (color != Appearance.INHERITED) {
			float[] c = ((Color) color).getRGBComponents(null);
			if (c.length == 4)
				colorAlpha = c[3];
			csos[0] = c[0];
			csos[1] = c[1];
			csos[2] = c[2];
		}

		csos[3] = 1f - (float) transparency;
		// TODO remove this if we decide finally to not allow transparency control via alpha channel of Color
		if (useOldTransparency) csos[3] *= colorAlpha;
		return csos;
	}

	public static void writeShader(String name, String shaderName ) {
		try {
		    File file = new File(name);
		    LoggingSystem.getLogger(RIBHelper.class).fine("writing in  "+name);
		    file = new File(file.getParent(),shaderName);
		    LoggingSystem.getLogger(RIBHelper.class).fine("checking on "+file+" exists "+file.exists());
		    if(!file.exists()) {
		    	OutputStream os = new FileOutputStream(file);
		    	InputStream is = DefaultPolygonShader.class.getResourceAsStream(shaderName);
		    
		    	int c = 0;
		    	while((c =is.read())!=-1) {
		    		os.write(c);
		    	}
		    	os.close();
		    	is.close();
		    }
		} catch (FileNotFoundException e) {
		    e.printStackTrace();
		} catch (IOException e) {
		    e.printStackTrace();
		}
	}

	/**
	 * @param cam
	 * @return
	 */
	public static float[] fTranspose(double[] mat) {
	    float[] tmat = new float[16];
	    for (int i = 0; i < 4; i++) 
	        for (int j = 0;j<4;j++){
	            tmat[i + 4*j] = (float) mat[j+4*i];
	        }
	    return tmat;
	}

	public static String str(String name) {
	    return "\""+name+"\"";
	}


	/**
	 * @param w2
	 * @param map
	 */
	public static void writeMap(PrintWriter w2, Map map) {
	    if(map!=null) {
	    Set keys = map.keySet();
	        for (Iterator key = keys.iterator(); key.hasNext();) {
	            String element = (String) key.next();
	            w2.print("\""+ element+"\" ");
	            RIBHelper.writeObject(w2,map.get(element));
	        }
	    }
	    w2.println("");
	}

	/**
	 * @param w2
	 * @param object
	 */
	public static void writeObject(PrintWriter w2, Object object) {
	    if(object instanceof double[]) {
	    	object = Rn.convertDoubleToFloatArray((double[]) object);
	    }
	    if(object instanceof float[]) {
	        float[] f = (float[]) object;
	        w2.print("[");
	        for (int i = 0; i < f.length; i++) {
	            w2.print(f[i]+" ");
	        }
	        w2.print("]");
	        return;
	    }
	    if(object instanceof int[]) {
	        int[] f = (int[]) object;
	        w2.print("[");
	        for (int i = 0; i < f.length; i++) {
	            w2.print(f[i]+" ");
	        }
	        w2.print("]");
	        return;
	    }
	    if(object instanceof Color) {
	        w2.print("[");
	        float[] rgb = ((Color)object).getRGBComponents(null);
	        for (int i = 0; i < 3; i++) {
	            w2.print(rgb[i]+" ");
	        }
	        w2.print("]");
	        return;
	    }
	
	    if(object instanceof String) {
	        w2.print("\""+object+"\"");
	        return;
	    }
	    w2.print(" "+object+" ");
	}
  
  
  public static void writeTexture(ImageData data, String noSuffix){
    
    BufferedImage img;
	  for (Iterator iter = ImageIO.getImageWritersByMIMEType("image/tiff"); iter.hasNext(); ) {
	   System.err.println("Writer: "+((ImageWriter) iter.next()).getClass().getName());
	  }
	        
	  if (true) {
	   // TODO temporary as long as ImageData does not return a propper BufferedImage
		  img = ImageUtility.getValidBufferedImage(data);
	  } else {
	   img = (BufferedImage) data.getImage();
	  }
	  // force alpha channel to be "pre-multiplied"
	  img.coerceData(true);
	
	  boolean worked=true;
		try {
		  Class encParamClass = Class.forName("com.sun.media.jai.codec.TIFFEncodeParam");
		  
	      Object encodeParam = encParamClass.newInstance();
	      Object compField = encParamClass.getField("COMPRESSION_DEFLATE").get(null);
	      
	      new Statement(encodeParam, "setCompression", new Object[]{compField}).execute();
	      new Statement(encodeParam, "setDeflateLevel", new Object[]{9}).execute();
	      
	      ParameterBlock pb = new ParameterBlock();
	      pb.addSource(img);
	      pb.add(new FileOutputStream(noSuffix+".tiff"));
	      pb.add("tiff");
	      pb.add(encodeParam);
	      
		  new Statement(Class.forName("javax.media.jai.JAI"), "create", new Object[]{"encode", pb}).execute();
	
		} catch(Throwable e) {
			  worked=false;
			  LoggingSystem.getLogger(RIBVisitor.class).log(Level.CONFIG, "could not write TIFF: "+noSuffix+".tiff", e);
		}
		if (!worked) {
		    try {
				 worked =ImageIO.write(img, "PNG", new File(noSuffix+".png"));
		    } catch (IOException e) {
		      e.printStackTrace();
			}
		    if (!worked) 
		     LoggingSystem.getLogger(RIBVisitor.class).log(Level.CONFIG, "could not write PNG: {0}.png", noSuffix);
		  }
	}
  
  
  	public static void createRIBLabel(PointSet ps, DefaultTextShader ts, RIBVisitor ribv){
		if (!ts.getShowLabels().booleanValue())
			return;
		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createPointImages(ps, font, c);
		DoubleArrayArray coords=ps.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
  		
  		writeLabel(ribv, img, coords, null, offset, alignment, scale);
  	}
  	
  	public static void createRIBLabel(IndexedLineSet ils, DefaultTextShader ts, RIBVisitor ribv){
		if (!ts.getShowLabels().booleanValue())
			return;
		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createEdgeImages(ils, font, c);
		DoubleArrayArray coords=ils.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
  		IntArrayArray inds=ils.getEdgeAttributes(Attribute.INDICES).toIntArrayArray();
  		writeLabel(ribv, img, coords, inds, offset, alignment, scale);
  	}

  	public static void createRIBLabel(IndexedFaceSet ifs, DefaultTextShader ts, RIBVisitor ribv){
		if (!ts.getShowLabels().booleanValue())
			return;
		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createFaceImages(ifs, font, c);
		DoubleArrayArray coords=ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
  		IntArrayArray inds=ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray();
  		writeLabel(ribv, img, coords, inds, offset, alignment, scale);
  	}
  	
	private static IndexedFaceSet bb = Primitives.texturedQuadrilateral(new double[] { 0, 1,
			0, 1, 1, 0, 1, 0, 0, 0, 0, 0 });

	private static void writeLabel(RIBVisitor ribv, ImageData[] labels, DoubleArrayArray vertices, IntArrayArray indices, double[] offset, int alignment, double scale) {
		Matrix c2o = new Matrix(Rn.times(null,ribv.world2Camera, ribv.object2world.getMatrix(null))).getInverse();
		double[] bbm = new double[16];
		for (int i = 0, n = labels.length; i < n; i++) {
			ImageData img = labels[i];
			String labelName = new File(ribv.writeTexture(img,Texture2D.GL_CLAMP, Texture2D.GL_CLAMP)).getName();	
			LabelUtility.calculateBillboardMatrix(bbm, img.getWidth() * scale, img.getHeight()* scale, offset, alignment, c2o.getArray(), LabelUtility.positionFor(i, vertices,indices), Pn.EUCLIDEAN);	
			ribv.ri.transformBegin();
			ribv.ri.concatTransform(fTranspose(bbm));
			HashMap<String, Object> shaderMap = new HashMap<String, Object>();
			shaderMap.put("string texturename", labelName);
			ribv.ri.surface("constantTexture", shaderMap);
			ribv.pointPolygon(bb, null);
			ribv.ri.transformEnd();
		}
	}
	
	
	/**
	 * Shifts lineShader.polygonShader-Attributes to polygonShader-Attributes
	 * Is used when tubes are splitted in several geometries by the BallAndStickFactory
	 * in RIBVisitor._visit(IndexedLineSet)
	 * 
	 * @param DefaultGeometryShader dgs
	 * @return the shifted Appearance
	 */	
	public static Appearance shiftTubesAppearance(DefaultGeometryShader dgs){
		Appearance ap = new Appearance();				
		if (dgs.getLineShader() instanceof de.jreality.shader.DefaultLineShader)	{		
			if(((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader() instanceof de.jreality.shader.DefaultPolygonShader){								
				ap.setAttribute("polygonShader", de.jreality.shader.DefaultPolygonShader.class);								
				de.jreality.shader.DefaultPolygonShader lsps =(de.jreality.shader.DefaultPolygonShader)((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader();
				evaluateShader(ap, lsps, "");
			}
			else if(((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader() instanceof de.jreality.shader.TwoSidePolygonShader){
				ap.setAttribute("polygonShader", de.jreality.shader.TwoSidePolygonShader.class);
				de.jreality.shader.DefaultPolygonShader lspsf =(de.jreality.shader.DefaultPolygonShader)((de.jreality.shader.TwoSidePolygonShader)((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader()).getFront();
				de.jreality.shader.DefaultPolygonShader lspsb=(de.jreality.shader.DefaultPolygonShader)((de.jreality.shader.TwoSidePolygonShader)((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader()).getBack();
				evaluateShader(ap, lspsf, ".front");
				evaluateShader(ap, lspsb, ".back");
			}
			else LoggingSystem.getLogger(ShaderUtility.class).warning("the following instance of lineShader.polygonShader is not supported in RIBHelper.shiftTubesAppearance(DefaultGeometryShader):\n"+((de.jreality.shader.DefaultLineShader)dgs.getLineShader()).getPolygonShader()); 
		}
		else LoggingSystem.getLogger(ShaderUtility.class).warning("the following instance of lineShader is not supported in RIBHelper.shiftTubesAppearance(DefaultGeometryShader):\n"+dgs.getLineShader()); 
		return ap;
	}
	
	/**
	 * method for shiftTubesAppearance(DefaultGeometryShader)
	 * sets Attributes
	 * 
	 * @param evalToApp
	 * @param shader
	 * @param side
	 * @return
	 */	
	private static Appearance evaluateShader(Appearance evalToApp, de.jreality.shader.DefaultPolygonShader shader, String side){			
		evalToApp.setAttribute("polygonShader"+side+".diffuseColor",shader.getDiffuseColor());									
		evalToApp.setAttribute("polygonShader"+side+".ambientCoefficient",shader.getAmbientCoefficient());									
		evalToApp.setAttribute("polygonShader"+side+".ambientColor",shader.getAmbientColor());									
		evalToApp.setAttribute("polygonShader"+side+".diffuseCoefficient",shader.getDiffuseCoefficient());									
		evalToApp.setAttribute("polygonShader"+side+".specularCoefficient",shader.getSpecularCoefficient());									
		evalToApp.setAttribute("polygonShader"+side+".smoothShading",shader.getSmoothShading());									
		evalToApp.setAttribute("polygonShader"+side+".specularColor",shader.getSpecularColor());									
		evalToApp.setAttribute("polygonShader"+side+".specularExponent",shader.getSpecularExponent());
		evalToApp.setAttribute("polygonShader"+side+".transparency",shader.getTransparency());
		if(shader.getTexture2d()==null)
			evalToApp.setAttribute("polygonShader"+side+".texture2d",  Appearance.DEFAULT);
		else
			TextureUtility.createTexture(evalToApp, "polygonShader"+side, shader.getTexture2d().getImage(), false);
		if(shader.getReflectionMap()==null)
			evalToApp.setAttribute("polygonShader"+side+"."+CommonAttributes.REFLECTION_MAP,  Appearance.DEFAULT);
		else{                      
			CubeMap lineCubeMap=TextureUtility.createReflectionMap(evalToApp, "polygonShader"+side, TextureUtility.getCubeMapImages(shader.getReflectionMap()));
			lineCubeMap.setBlendColor(shader.getReflectionMap().getBlendColor());
		}
		return evalToApp;
	}
	
	
}
