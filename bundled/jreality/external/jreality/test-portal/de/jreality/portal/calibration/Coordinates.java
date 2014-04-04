package de.jreality.portal.calibration;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;

public class Coordinates{
	
	private final double factor=0.01;
	
	private double[][][] axis={{{0,0,0},{1,0,0}},{{0,0,0},{0,1,0}},{{0,0,0},{0,0,1}}};
	private Color axisColors[]={new Color(255,0,0),new Color(0,255,0),new Color(0,0,255)};
	private double tubeRadius;
	private double sphereRadius;

	private IndexedFaceSetFactory[] system;
	private SceneGraphComponent[] systemNode;
	private SceneGraphComponent rootNode;
	
	public Coordinates(double scale){
		super();
		axis[0][1][0]=scale;
		axis[1][1][1]=scale;
		axis[2][1][2]=scale;
		tubeRadius=factor*scale;
		sphereRadius=3*tubeRadius;
		createSystem();
	}
	
	public Coordinates(){
		this(1);
	}
	
	private void createSystem(){
		rootNode=new SceneGraphComponent();
		system=new IndexedFaceSetFactory[3];
		systemNode=new SceneGraphComponent[3];
		
		for(int i=0; i<3; i++){		
			system[i]=new IndexedFaceSetFactory();
			system[i].setVertexCount(2);
			system[i].setVertexCoordinates(axis[i]);
			system[i].setFaceCount(1);
			system[i].setFaceIndices(new int[][] {{0,1}});
			system[i].setGenerateEdgesFromFaces(true);
			system[i].setGenerateFaceNormals(true);
			system[i].setGenerateVertexNormals(true);		
			system[i].update();
			
			systemNode[i]=new SceneGraphComponent();
			rootNode.addChild(systemNode[i]);
			systemNode[i].setGeometry(system[i].getIndexedFaceSet());
			
			systemNode[i].setAppearance(new Appearance());
			systemNode[i].getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,axisColors[i]);
			systemNode[i].getAppearance().setAttribute(CommonAttributes.LINE_SHADER+".polygonShader."+CommonAttributes.DIFFUSE_COLOR,axisColors[i]);
			systemNode[i].getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,axisColors[i]);
		}
		
		Appearance app=new Appearance();
		app.setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,Color.RED);
		app.setAttribute(CommonAttributes.FACE_DRAW,false);
		app.setAttribute(CommonAttributes.EDGE_DRAW,true);
		app.setAttribute(CommonAttributes.VERTEX_DRAW,true);
		app.setAttribute(CommonAttributes.TUBES_DRAW,true);
		app.setAttribute(CommonAttributes.SPHERES_DRAW,true);
		app.setAttribute(CommonAttributes.POINT_RADIUS,sphereRadius);
		app.setAttribute(CommonAttributes.TUBE_RADIUS,tubeRadius);
		rootNode.setAppearance(app);		
	}
	
	public void kill(int axisNum){
		rootNode.removeChild(systemNode[axisNum]);
	}
	
	public void set(int axisNum, double[] vec){
		axis[axisNum][1]=vec;
		system[axisNum].setVertexCoordinates(axis[axisNum]);
		system[axisNum].update();
	}
	
	public SceneGraphComponent getSystem(){
		return rootNode;
	}
}
