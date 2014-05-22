package de.jreality.reader.mathematica;

import java.awt.Color;
import java.util.ArrayList;
import java.util.LinkedList;

import de.jreality.geometry.PointSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.shader.CommonAttributes;

public class MState {
	// direktiven:
	static Color plCDefault= new Color(127,127,127);	// default- Punkt und Linienfarbe
	static Color fCDefault = new Color(255,255,255);	// default- Flaechenfarbe
//	----------------
	// fuer spaeteres einfuegen der colors und normals
	LinkedList< IndexedFaceSet> faces= new LinkedList<IndexedFaceSet>();
	// vom Graphicscomplex
	double[][] coords=null;
//	----------------
	Color color=null; // for lines, Points, and surfaceObjects 
	Color faceColor=null; // for Faces only
	Color edgeColor=null; // for edges of Faces only
//	----------------

	boolean edgeDraw=true;
	boolean faceDraw=true;

	public double[] getCoords(int index){
		if(coords==null) return new double[]{0,0,0};
		int i=(int)index;
		if(i>=coords.length) return new double[]{0,0,0};
		return coords[i];
	}
	public double[][] getIndexCoords(int[] indis){
		double[][] res= new double[indis.length][];
		for (int i = 0; i < indis.length; i++) 
			res[i]=getCoords(indis[i]);
		return res;
	}
	public MState copy(){
		MState snew= new MState();
		snew.faces=faces;
		snew.color=color;
		snew.edgeColor=edgeColor;
		snew.faceColor=faceColor;
		snew.coords=coords;
		snew.edgeDraw= edgeDraw;
		snew.faceDraw= faceDraw;
		return snew;
	} 
	public Appearance getPointSetApp(){
		Appearance app= new Appearance();
		// Point Color 
		if(color!=null){  
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		return app;
	}
	public Appearance getLineSetApp(){
		Appearance app= new Appearance();
		app.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		// Line Color
		if(color!=null){  
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		return app;
	}
	public Appearance getFaceApp(){
		Appearance app= new Appearance();
		if(!edgeDraw)
			app.setAttribute(CommonAttributes.EDGE_DRAW, false);
		if(!faceDraw)
			app.setAttribute(CommonAttributes.FACE_DRAW, false);
		// face Color
		if(faceColor!=null){ 
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, faceColor);
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-faceColor.getAlpha()/256.));
		}
		else if(color!=null){  
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		// edge Color
		if(edgeColor!=null){ 
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, edgeColor);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, edgeColor);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-edgeColor.getAlpha()/256.));
		}
		else if(color!=null){  
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		// Point Color 
		if(color!=null){  
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		return app;
	}
	public Appearance getPrimitiveApp(){
		Appearance app= new Appearance();
		app.setAttribute(CommonAttributes.EDGE_DRAW, false);
		app.setAttribute(CommonAttributes.VERTEX_DRAW, false);
		if(color!=null){  
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
		}
		return app;
	}

	public SceneGraphComponent makeCuboid(double[] vMin,double[] vMax){
		double[] vCenter =Rn.linearCombination(null, .5, vMin, .5, vMax);
		double[] vScale  =Rn.subtract(null, vMax, vCenter);
		SceneGraphComponent geo=new SceneGraphComponent();
		geo.setGeometry(Primitives.cube());
		geo.setName("Cuboid");
		geo.setAppearance(getPrimitiveApp());
		MatrixBuilder.euclidean().translate(vCenter).scale(vScale).assignTo(geo);
		return geo;
	}
	public SceneGraphComponent makeSphere(double[] center, double radius){
		SceneGraphComponent geo=Primitives.sphere(radius, center);
		geo.setName("Sphere");
		geo.setAppearance(getPrimitiveApp());
		return geo;
	}

	public SceneGraphComponent makeCylinder(double[] anfg,double[] ende,double radius) {
		double[] pos=Rn.linearCombination(null, .5, anfg, .5, ende);
		double scale=Rn.euclideanDistance(ende, anfg)/2;
		double[] dir=Rn.normalize(null, Rn.subtract(null, ende, anfg));
		SceneGraphComponent geo= new SceneGraphComponent();
		geo=Primitives.closedCylinder(20, radius, -scale, scale, Math.PI*2);
		geo.setName("closed Cylinder");
		if(geo.getTransformation()==null)
			geo.setTransformation(new Transformation());
		double[] m=MatrixBuilder.euclidean().
		  translate(pos).
		  rotateFromTo(new double[]{0,0,1}, dir).getArray();
		geo.getTransformation().multiplyOnLeft(m);
		geo.setAppearance(getPrimitiveApp());
		return geo;
	}
	public SceneGraphComponent makeLabel(String expr,double[] coords,double[] offset,double[] dir){
		PointSetFactory psf = new PointSetFactory();
		psf.setVertexCount(1);
		psf.setVertexCoordinates(new double[][]{coords});
		psf.setVertexLabels(new String[]{expr});
		psf.update();
		SceneGraphComponent geo=new SceneGraphComponent();
		geo.setAppearance(getLabelApp());
		geo.setGeometry(psf.getPointSet());
		geo.setName("Label");
		return geo;
	}
	public Appearance getLabelApp(){
		Appearance app= new Appearance();
		app.setAttribute(CommonAttributes.POINT_RADIUS, 0.0001);
		if(color!=null){  
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.TRANSPARENCY, (1.-color.getAlpha()/256.));
			app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, color);
		}
		return app;
	}
	public Color getColor(){
		return color;
	}
	public Color getLineColor(){
		return color;
	}
	public Color getEdgeColor(){
		if(edgeColor==null)
			return color;
		return edgeColor;
	}
	public Color getFaceColor(){
		if(faceColor==null)
			return color;
		return faceColor;
	}
	public Color getPointColor(){
		return color;
	}
	public void setColor(Color c){
		color=c;
		edgeColor=c;
		faceColor=c;
	}
	public void assignColorList(ArrayList< Color> cols){
		for (IndexedFaceSet f : faces ) {
			int n= f.getNumPoints();
			if(n>cols.size()) continue;
			if(f.getVertexAttributes(Attribute.COLORS)!=null)continue;
			double[][] cData=new double[n][];
			for (int i = 0; i < n; i++) 
				cData[i]=MHelper.getRgbaColor(cols.get(i));
			f.setVertexAttributes(Attribute.COLORS,new DoubleArrayArray.Array(cData));
		}
	}
	public void assignNormalList(ArrayList< double[]> norms){
		for (IndexedFaceSet f : faces ) {
			int n= f.getNumPoints();
			if(n>norms.size()) continue;
			if(f.getVertexAttributes(Attribute.NORMALS)!=null)continue;
			double[][] nData=new double[n][];
			for (int i = 0; i < n; i++){
				nData[i]=new double[norms.get(i).length]; 
				for (int j = 0; j < nData[i].length; j++) 
					nData[i][j]=norms.get(i)[j];
			}
			f.setVertexAttributes(Attribute.NORMALS,new DoubleArrayArray.Array(nData));
		}
	}
	public void assignColorList(PointSet p,ArrayList< Color> cols){
		int n= p.getNumPoints();
		if(n>cols.size()) return;
		double[][] cData=new double[n][];
		for (int i = 0; i < n; i++) 
			cData[i]=MHelper.getRgbaColor(cols.get(i));
		p.setVertexAttributes(Attribute.COLORS,new DoubleArrayArray.Array(cData));
	}
	public void assignNormalList(PointSet p,ArrayList< double[]> norms){
		int n= p.getNumPoints();
		if(n>norms.size()) return;
		double[][] nData=new double[n][];
		for (int i = 0; i < n; i++){
			nData[i]=new double[norms.get(i).length]; 
			for (int j = 0; j < nData[i].length; j++) 
				nData[i][j]=norms.get(i)[j];
		}
		p.setVertexAttributes(Attribute.NORMALS,new DoubleArrayArray.Array(nData));
	}

}
