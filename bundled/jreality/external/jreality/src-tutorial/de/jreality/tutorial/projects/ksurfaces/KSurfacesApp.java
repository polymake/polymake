package de.jreality.tutorial.projects.ksurfaces;

import java.awt.Color;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.shader.CommonAttributes;
import de.jreality.tutorial.util.polygon.DragPointSet;
import de.jreality.tutorial.util.polygon.PointSequenceView;
import de.jreality.tutorial.util.polygon.SubdividedPolygon;
import de.jtem.beans.InspectorPanel;
import de.jtem.jrworkspace.plugin.PluginInfo;

/** This class contains the application to display the K-surfaces and their Gauss map.
 * 
 * @author G. Paul Peters, 24.07.2009
 *
 */
public class KSurfacesApp {
	
	private int m=3; 
	private int n=10; 
	private double a1=1; 
	private double a2=.9;
	private double b1=1.2;
	private double b2=1.1;
	private int time=1;
	private double[][][] gaussMap;
	private double[][][] initialGaussMap;
	private double[][][] map;
	
	final private SceneGraphComponent root=new SceneGraphComponent("k-surfaces");
	final private SceneGraphComponent gaussMapSgc=new SceneGraphComponent("Gauss map");
	final private IndexedFaceSetFactory gaussMapIfs = new IndexedFaceSetFactory();
	final private SceneGraphComponent mapSgc=new SceneGraphComponent("The K-surface");
	final private IndexedFaceSetFactory mapIfs = new IndexedFaceSetFactory();

	final private SceneGraphComponent initalData0=new SceneGraphComponent("initial data, t=0");
	final private IndexedLineSetFactory initialDataIls0 = new IndexedLineSetFactory();
	final private DragPointSet dragPointSet;
	final private SubdividedPolygon time1SubdivPoly;
	
	final private SceneGraphComponent sphere=new SceneGraphComponent("unit sphere");
	
	public KSurfacesApp() {
		// scene graph component for the gauss map
		gaussMapIfs.setGenerateEdgesFromFaces(true);
		gaussMapIfs.setGenerateFaceNormals(true);
		gaussMapSgc.setGeometry(gaussMapIfs.getGeometry());
		root.addChild(gaussMapSgc);

		mapIfs.setGenerateEdgesFromFaces(true);
		mapIfs.setGenerateFaceNormals(true);
		mapSgc.setGeometry(mapIfs.getGeometry());
		mapSgc.setTransformation(new Transformation(
				MatrixBuilder.euclidean().translate(3,0,0).getArray()));
		root.addChild(mapSgc);

		initalData0.setGeometry(initialDataIls0.getGeometry());
		initalData0.setAppearance(new Appearance());
		initalData0.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Color.red);
		initalData0.getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, .2);
		gaussMapSgc.addChild(initalData0);

		//initial condition at time 1 draggable points
		a1=0;a2=0;
		b1=.2;b2=.2;
		ellipticConesInitialCondition();
		//to keep the points on the unit sphre override the transform method
		dragPointSet=new DragPointSet(initialGaussMap[1]) {
			@Override
			public void transform(double[] vertex) {
				double l=R3.norm(vertex);
				vertex[0]=vertex[0]/l;
				vertex[1]=vertex[1]/l;
				vertex[2]=vertex[2]/l;
			}
		};
		dragPointSet.getBase().getAppearance().setAttribute(CommonAttributes.POINT_SHADER+"."+CommonAttributes.POINT_RADIUS, .16);
		gaussMapSgc.addChild(dragPointSet.getBase());
		dragPointSet.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				KSurfacesApp.this.update();
			}
		});
		

		//initial condition at time 1 subdivided Polygon points
		time1SubdivPoly = new SubdividedPolygon(dragPointSet);
		time1SubdivPoly.setSubdivisionLevel(0);
		PointSequenceView time1CurveView = new PointSequenceView(time1SubdivPoly);
		time1CurveView.setPointRadius(0.01);
		time1CurveView.setLineRadius(0.04);
		time1CurveView.setPointColor(Color.yellow);
		time1CurveView.setLineColor(Color.green);
		gaussMapSgc.addChild(time1CurveView.getBase());
				
		sphere.setGeometry(new Sphere());
		gaussMapSgc.addChild(sphere);
		
		update();
	}
	
	private void ellipticConesInitialCondition() { 
		initialGaussMap = new double[2][n][3];
		for (int j=0; j<n; j++){
			double s=a1*Math.sin(2*Math.PI/n*j);
			double c=a2*Math.cos(2*Math.PI/n*j);
			double f=1/Math.sqrt(s*s+c*c+1);
			initialGaussMap[0][j][0]=s*f;
			initialGaussMap[0][j][1]=c*f;
			initialGaussMap[0][j][2]=1*f;

			s=b1*Math.sin(2*Math.PI/n*(j-.5));
			c=b2*Math.cos(2*Math.PI/n*(j-.5));
			f=1/Math.sqrt(s*s+c*c+1);
			initialGaussMap[1][j][0]=s*f;
			initialGaussMap[1][j][1]=c*f;
			initialGaussMap[1][j][2]=1*f;
		}
	}
	
	//calculate the gauss map and update it via gaussMapIfs
	private void gaussMap() {
		//calculate the gauss map
		gaussMap=new double[m][n][3];
		KSurfaces.gaussMapFromInitialAnnulus(initialGaussMap, gaussMap);
		
		double[][] vertices = rearrangeVertices(gaussMap);
		int[][] faces = calcFaces();

		//put the data into the IndexedFaceSetFactory and update the Geometry
		gaussMapIfs.setVertexCount(n*m);
		gaussMapIfs.setVertexCoordinates(vertices);
		gaussMapIfs.setFaceCount(n*(m-2));
		gaussMapIfs.setFaceIndices(faces);
		gaussMapIfs.update();
		
		initialDataIls0.setVertexCount(n);
		initialDataIls0.setVertexCoordinates(gaussMap[0]);
		initialDataIls0.update();
	}

	//calculate the map of K-surface and update it via gaussMapIfs
	private void map() {
		//calculate the map
		map=new double[m][n][3];
		KSurfaces.kSurfaceFromGaussMap(gaussMap, map);
		
		double[][] vertices = rearrangeVertices(map);
		int[][] faces = calcFaces();

		//put the data into the IndexedFaceSetFactory and update the Geometry
		mapIfs.setVertexCount(n*m);
		mapIfs.setVertexCoordinates(vertices);
		mapIfs.setFaceCount(n*(m-2));
		mapIfs.setFaceIndices(faces);
		mapIfs.update();
	}

	//calculate the combinatorics of the faces for use in an IndexedFaceSetFactory
	private int[][] calcFaces() {
		int[][] faces = new int[n*(m-2)][4];
		for (int i = 0; i < m-2; i++) {
			for (int j = 0; j < n; j++) {
				int k = j + i*n;
				faces[k][0] = j       + i*n;
				faces[k][1] = j       + (i+1)*n;
				faces[k][2] = (j+1)%n + (i+2)*n;
				faces[k][3] = (j+1)%n + (i+1)*n;
			}
		}
		return faces;
	}

	//rearrange the vertices for use in an IndexedFaceSetFactory
	private double[][] rearrangeVertices(double[][][] xtVertices) {
		double[][] vertices=new double[n*m][3];
		for (int i = 0; i < m; i++) {
			for (int j = 0; j < n; j++) {
				for (int k = 0; k < 3; k++) { 
					vertices[j+i*n][k]=xtVertices[i][j][k];
				}
			}
		}
		return vertices;
	}
	
	
	/** This method needs to be called whenever the changes of the parameters should have an effect on the
	 * scene graph one gets from {@link #getRoot()}. 
	 */
	public void update() {
		n=time1SubdivPoly.getPoints().length;
		ellipticConesInitialCondition();
		initialGaussMap[1]=time1SubdivPoly.getPoints();
		gaussMap();
		map();
	}

	/**
	 * @return the root of the scene graph with the Gauss map of the k-surface
	 */
	public SceneGraphComponent getRoot() {
		return root;
	}

	public int getM() {
		return m;
	}

	public void setM(int m) {
		if (m<2) return;
		if (m<=time) return;
		this.m = m;
	}

	public int getN() {
		return n;
	}

	public void setN(int n) {
		if (n<2) return;
		this.n = n;
	}

	public double getA1() {
		return a1;
	}

	public void setA1(double a1) {
		this.a1 = a1;
	}

	public double getA2() {
		return a2;
	}

	public void setA2(double a2) {
		this.a2 = a2;
	}

	public double getB1() {
		return b1;
	}

	public void setB1(double b1) {
		this.b1 = b1;
	}

	public double getB2() {
		return b2;
	}

	public void setB2(double b2) {
		this.b2 = b2;
	}

	public int getT() {
		return time;
	}

	public void setT(int t) {
		if (t<2) return;
		if (t>=m) m=t+1;
		this.time = t;
	}

	public static void main(String[] args) {
		KSurfacesApp ksa = new KSurfacesApp();
		
		//initialize the ViewerVR
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentTools());
		v.setContent(ksa.getRoot());

		//add an Inspector for the KSurfacesApp to the viewer
		InspectorPanel inspector = new InspectorPanel();
		inspector.setObject(ksa, "update");
		ViewShrinkPanelPlugin plugin = new ViewShrinkPanelPlugin() {
			@Override
			public PluginInfo getPluginInfo() {
				return new PluginInfo("Domain");
			}
		};
		plugin.getShrinkPanel().add(inspector);
		v.registerPlugin(plugin);

		v.startup();
	}


}


