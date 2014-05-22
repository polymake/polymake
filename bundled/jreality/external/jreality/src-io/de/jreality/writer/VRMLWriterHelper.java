package de.jreality.writer;

import java.awt.Color;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.LinkedList;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;

public class VRMLWriterHelper {

	private HashMap<Integer, GeoParts> geometryParts;
	
	private class GeoParts{
		int geoCount=0;
		int faceCount=0;
		int lineCount=0;
		int pointCount=0;
		boolean facesDefined=false; 
		boolean linesDefined=false; 
		boolean pointsDefined=false;
	} 
	
	/** counts how often a Geometry is used.
	 *  divide into Faces, Edges, Points 
	 * @author gonska
	 */
	private class MyVisitor extends SceneGraphVisitor{
		private EffectiveAppearance effApp= EffectiveAppearance.create();
		private DefaultGeometryShader dgs=ShaderUtility.createDefaultGeometryShader(effApp);
		private boolean faces=false;
		private boolean lines=false;
		private boolean points=false;
		
		public MyVisitor() {}
		public MyVisitor(MyVisitor mv) {
			effApp=mv.effApp;
		}		
		public void visit(SceneGraphComponent c) {
			c.childrenAccept(new MyVisitor(this));			
			super.visit(c);
		}
		public void visit(Appearance a) {
			effApp=effApp.create(a);
			dgs = ShaderUtility.createDefaultGeometryShader(effApp);
			super.visit(a);
		}
		public void visit(IndexedFaceSet i) {
			if (dgs.getShowFaces())faces=true;
			super.visit(i);
		}
		public void visit(IndexedLineSet g) {
			if (dgs.getShowLines())lines=true;
			super.visit(g);
		}
		public void visit(PointSet p) {
			if (dgs.getShowPoints())points=true;
			 // remember all kinds of PointSet:
			if(geometryParts.containsKey(p.hashCode())){
				GeoParts gp=geometryParts.get(p.hashCode());
				if (faces) gp.faceCount++;
				if (lines) gp.lineCount++;
				if (points) gp.pointCount++;
			}
			else {
				GeoParts gp= new GeoParts();
				if (faces) gp.faceCount++;
				if (lines) gp.lineCount++;
				if (points) gp.pointCount++;
				geometryParts.put(p.hashCode(), gp);
			}
			super.visit(p);
		}
	};
		
	public void inspect(SceneGraphNode c){
		geometryParts= new HashMap<Integer, GeoParts>();
		c.accept(new MyVisitor());	
	}
	public boolean isMultipleUsedFaceSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return (gp.faceCount>1);
		}
		return false;			
	} 	
	public boolean isMultipleUsedLineSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return (gp.lineCount>1);
		}
		return false;			
	} 	
	public boolean isMultipleUsedPointSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return (gp.pointCount>1);
		}
		return false;			
	} 	
	public boolean isDefinedFaceSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return gp.facesDefined;
		}
		return false;			
	} 	
	public boolean isDefinedLineSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return gp.linesDefined;
		}
		return false;			
	} 	
	public boolean isDefinedPointSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			return gp.pointsDefined;
		}
		return false;			
	} 	
	public void setDefinedFaceSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			gp.facesDefined=true;
		}
	}
	public void setDefinedLineSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			gp.linesDefined=true;
		}
	}
	public void setDefinedPointSet(Geometry g){
		if (geometryParts.containsKey(g.hashCode())){
			GeoParts gp= geometryParts.get(g.hashCode());
			gp.pointsDefined=true;
		}
	}
	static int[][] getIntIntFaceAttr(IndexedFaceSet ifs,Attribute a){
		DataList d=null;
		d=ifs.getFaceAttributes(a);
		if(d==null)return null;
		try {			return d.toIntArrayArray(null);	
		} catch (Exception e) {return null;		}		
	}
	static double[][] getDoubleDoubleFaceAttr(IndexedFaceSet ifs,Attribute a){
		DataList d=null;
		d=ifs.getFaceAttributes(a);
		if(d==null)return null;
		try {	return d.toDoubleArrayArray(null);
		} catch (Exception e) {	return null;	}		
	}
	static int[][] getIntIntEdgeAttr(IndexedLineSet ils,Attribute a){
		DataList d=null;
		d=ils.getEdgeAttributes(a);
		if(d==null)return null;
		try {			return d.toIntArrayArray(null);	
		} catch (Exception e) {return null;		}		
	}
	static double[][] getDoubleDoubleEdgeAttr(IndexedLineSet ils,Attribute a){
		DataList d=null;
		d=ils.getEdgeAttributes(a);
		if(d==null)return null;
		try {	return d.toDoubleArrayArray(null);
		} catch (Exception e) {	return null;	}		
	}
	static double[][] getDoubleDoubleVertexAttr(PointSet p,Attribute a){
		DataList d=null;
		d=p.getVertexAttributes(a);
		if(d==null)return null;
		try {	return d.toDoubleArrayArray(null);
		} catch (Exception e) {	return null;	}		
	}
	static String[] getLabelFaceAttr(IndexedFaceSet ifs){
		DataList d=null;
		d=ifs.getFaceAttributes(Attribute.LABELS);
		if(d==null)return null;
		try {			return d.toStringArray(null);	
		} catch (Exception e) {return null;		}		
	}
	static String[] getLabelEdgeAttr(IndexedLineSet ils){
		DataList d=null;
		d=ils.getEdgeAttributes(Attribute.LABELS);
		if(d==null)return null;
		try {			return d.toStringArray(null);	
		} catch (Exception e) {return null;		}		
	}
	static String[] getLabelPointAttr(PointSet p){
		DataList d=null;
		d=p.getVertexAttributes(Attribute.LABELS);
		if(d==null)return null;
		try {			return d.toStringArray(null);	
		} catch (Exception e) {return null;		}		
	}
	//-------------------------------------------------------------
	 static String str(String name) {
		 return "\""+name+"\"";
	 }
	 static void writeDoubleArray(double[] d, String hist, String append,int size,PrintWriter out) {
		 out.print(""+hist);
		 for (int i=0;i<size;i++)
			 out.print(String.format(" %13.7g",d[i]));
		 out.println(append);
	 }
	 static void writeImage(Texture2D tex,String hist,PrintWriter out) {
			String hist2=hist+"  ";
			ImageData id=tex.getImage();
			byte[] data= id.getByteArray();
			int w=id.getWidth();
			int h=id.getHeight();
			int dim= data.length/(w*h);
			// write image
			out.print(hist+"image ");
			out.println(""+w+" "+h+" "+dim);
			for (int i = 0; i < w*h; i++) {
				int mergeVal=0;
				// calculate hexvalue from colors
				for (int k = 0; k < dim; k++) {
					int val=data[i*4+k];
					if (val<0)val=val+256;
					mergeVal*=256;
					mergeVal+=val;
				}
				out.println(hist2+"0x"+ Integer.toHexString(mergeVal).toUpperCase());
			}
		}
	 static double[] colorToDoubleArray(Color c){
		 double[] d=new double[]{(double)c.getRed()/255,(double)c.getGreen()/255,(double)c.getBlue()/255};
		 return d;
	 }
	 static String ColorToString(Color c){
		 return ""+((double)c.getRed())/255+" "+((double)c.getGreen())/255+" "+((double)c.getBlue())/255;
	 }
	 static Color DoublesToColor(double[] c){
		 return new Color((float)c[0],(float)c[1],(float)c[2]);
	 }
	 static double[][] convertLineVertexColors(double[][] colors,int[][] lindis){
		 LinkedList list= new LinkedList<double[]>();
		 for (int i = 0; i < lindis.length; i++) 
			 for (int j = 0; j < lindis[i].length; j++) 
				 list.add(colors[lindis[i][j]]);
		 double[][] newCol=new double[list.size()][];
		 for (int i = 0; i < newCol.length; i++) 
			 newCol[i]=(double[])list.get(i);
		 return newCol;
	 }
	 /** returnes a Matrix which transforms a Cylinder(vrml !!!)
	  * to a Tube surrounding the line betwen v and w
	  * @param v
	  * @param w
	  * @param radius
	  * @return
	  */
	 static double[] calcCylinderMatrix(double[] v,double[] w, double radius){
		 v= new double[]{v[0],v[1],v[2]};
		 w= new double[]{w[0],w[1],w[2]};
		 double[] midpoint=Rn.linearCombination(null, .5, v, .5, w);
		 double[] direction=Rn.subtract(null, v, w);
		 double length=Rn.euclideanNorm(direction);
		 double[] martix=
			 MatrixBuilder.euclidean()
			 .translate(midpoint)
			 .rotateFromTo(new double[]{0,1,0}, direction)
			 .scale(new double[]{radius,length/2,radius})
			 .getArray();
		 return martix;
	 }
}
