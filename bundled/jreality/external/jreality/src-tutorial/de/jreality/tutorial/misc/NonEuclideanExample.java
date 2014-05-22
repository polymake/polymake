/*
 * Created on Sep 15, 2004
 *
*/
package de.jreality.tutorial.misc;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.EDGE_DRAW;
import static de.jreality.shader.CommonAttributes.LIGHTING_ENABLED;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.METRIC;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.SPECULAR_EXPONENT;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D;
import static de.jreality.shader.CommonAttributes.TUBE_RADIUS;
import static de.jreality.shader.CommonAttributes.VERTEX_DRAW;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.io.IOException;
import java.net.URL;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.ParametricSurfaceFactory;
import de.jreality.geometry.ParametricSurfaceFactory.Immersion;
import de.jreality.geometry.Primitives;
import de.jreality.geometry.SphereUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.tools.DraggingTool;
import de.jreality.tools.RotateTool;
import de.jreality.tutorial.intro.Intro07;
import de.jreality.tutorial.util.FlyTool;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.SelectionManagerImpl;
import de.jreality.util.CameraUtility;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * This class provides a simple example of using the non-euclidean features of jReality.
 * <p>
 * It is seemingly more complex than most of the other tutorial examples since the
 * built-in ViewerApp application has features which are implicitly euclidean and can't be
 * easily changed.
 * <p>
 * The idea is to construct 3 different versions of the same scene: the unit disk plus a 
 * small sphere centered at the origin.  Additionally, unit  normals are displayed on
 * the polygonal mesh representing the unit disk.  One can see from the size of the 
 * mesh polygons and the behavior of the normals, that the three geometries have different
 * qualities.
 * <p>
 * The code shows what parts of the scene graph have to be changed if you want to work with
 * non-euclidean geometry.  See the {@link #update()} method.
 * <p>
 * The standard tools {@link RotateTool} and {@link DraggingTool} have been used here; they
 * are adequate (but not perfect) when it comes to handling non-euclidean metric.
 * <p>
 * The example also uses a {@link FlyTool} class which behaves better with non-euclidean metric than
 * the tool provided in the standard distribution {@link de.jreality.tools.FlyTool}.
 * <p>
 * To shift from one metric to the next type the '1' key. The order cycles through
 * euclidean -- elliptic -- hyperbolic.
 * @author gunn
 *
 */
public class NonEuclideanExample {

	SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
	int metric = Pn.EUCLIDEAN;
	private SceneGraphComponent[] sigs = new SceneGraphComponent[3];
	private PointLight pointLight;
	private SceneGraphComponent lightNode;
	private SceneGraphComponent wireframeSphere;
	Viewer viewer;
	
	public static void main(String[] args)	{
		NonEuclideanExample hs = new NonEuclideanExample();
		hs.doIt();
	}
	
	public void doIt() {
		makeWorld();
		// standard ViewerApp is not capable of handling non-euclidean stuff\
		// we have to build our scene graph by hand
		SceneGraphComponent cameraNode =SceneGraphUtility.createFullSceneGraphComponent("camera");
	    Camera camera = new Camera();
	    cameraNode.setCamera(camera);
	    FlyTool flytool = new FlyTool();
	    flytool.setGain(.1);
		cameraNode.addTool(flytool);
		SceneGraphComponent myroot = SceneGraphUtility.createFullSceneGraphComponent("root");
		myroot.getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0,20, 40));
		myroot.addChildren(cameraNode, world);
		myroot.addTool(new ClickWheelCameraZoomTool());
	    
//		ViewerApp va = new ViewerApp(root, camPath, null, null);
//		va.setAttachNavigator(true);
//		va.setExternalNavigator(false);
//		viewer = va.getCurrentViewer();
	    viewer = JRViewer.display(myroot);
	    SceneGraphUtility.removeLights(viewer);
		world.addChild(lightNode);
		SceneGraphComponent root = viewer.getSceneRoot();
		root.getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0,20, 40));
		root.getAppearance().setAttribute(CommonAttributes.BACKGROUND_COLORS, Appearance.INHERITED);
		root.getAppearance().setAttribute(CommonAttributes.ANY_DISPLAY_LISTS, false);
		root.getAppearance().setAttribute("oneGLSL", true);  // don't ask!
	    SceneGraphPath camPath = SceneGraphUtility.getPathsBetween(root, cameraNode).get(0);
	    camPath.push(camera);
	    viewer.setCameraPath(camPath);
		SelectionManager sm = SelectionManagerImpl.selectionManagerForViewer(viewer);
		sm.setDefaultSelectionPath(new SceneGraphPath(viewer.getSceneRoot()));
		sm.setSelection(null);
//		va.setCreateMenu(false);
//		va.update();
//		va.display();
//		CameraUtility.encompass(viewer);
		update();
		Component comp = ((Component) viewer.getViewingComponent());
		comp.addKeyListener(new KeyAdapter() {
			public void keyPressed(KeyEvent e)	{ 
				switch(e.getKeyCode())	{
					
				case KeyEvent.VK_H:
					System.out.println("	1:  cycle metric");
					break;
	
				case KeyEvent.VK_1:
					metric ++;
					if (metric == 2) metric = -1;
					update();
					break;
				}
			}
		});
		
	}
	
	/**
	 * create the scene graph which will be the content of the demo
	 */
	public SceneGraphComponent makeWorld() {
		
		Appearance ap = world.getAppearance();
		ap.setAttribute(VERTEX_DRAW, false);
		// textures work too but for a first demo we leave them out.
		boolean doTexture = false;
		if (doTexture)	{
			Texture2D tex2d = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class, POLYGON_SHADER
							+ "." + TEXTURE_2D, ap, true);
			URL is = Intro07.class.getResource("gridSmall.jpg");
			ImageData id = null;
			try {
				id = ImageData.load(new Input(is));
			} catch (IOException ee) {
				ee.printStackTrace();
			}
			tex2d.setImage(id);
			Matrix foo = new Matrix();
			MatrixBuilder.euclidean().scale(10, 10, 1).assignTo(foo);
			tex2d.setTextureMatrix(foo);
		}
		
		ap.setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR,new Color(250, 250, 0));
		ap.setAttribute(LINE_SHADER+"."+POLYGON_SHADER+"."+DIFFUSE_COLOR,new Color(250, 0, 250));
		ap.setAttribute(LINE_SHADER+"."+DIFFUSE_COLOR,new Color(150,150,150));
		ap.setAttribute(SPECULAR_EXPONENT, 50.0);
		ap.setAttribute(TUBE_RADIUS, .003);
		// add a light node which gets placed above the disk (see update() below)
		lightNode = SceneGraphUtility.createFullSceneGraphComponent("child2");
		lightNode.getAppearance().setAttribute(LIGHTING_ENABLED, false);
//		lightNode.addChild(Primitives.sphere(.05, new double[]{0,0,0}, Pn.EUCLIDEAN));
		pointLight = new PointLight();
		pointLight.setIntensity(1);
		pointLight.setColor(new Color(250, 250, 250));
		lightNode.setLight(pointLight);
		world.addTool(new RotateTool());
		world.addTool(new DraggingTool());
		
		// construct three scenes, each basically a unit disk plus a small sphere
		// but done in such a way that the metric is respected in the creation
		// of the meshing
		double ballRadius = .015, stickRadius = .01;
		for (int i = 0; i<3; ++i)	{
			sigs[i] = SceneGraphUtility.createFullSceneGraphComponent("sig"+i);
			SceneGraphComponent disk = SceneGraphUtility.createFullSceneGraphComponent("disk"+i);
			IndexedFaceSet ifs = getDisk(i-1);
			disk.setGeometry(ifs);
			sigs[i].addChild(disk);
			SceneGraphComponent normals = SceneGraphUtility.createFullSceneGraphComponent("normals");
			normals.addChild(IndexedFaceSetUtility.displayFaceNormals(ifs, .1, i-1));
			sigs[i].addChild(normals);
			sigs[i].getAppearance().setAttribute(VERTEX_DRAW, false);
			sigs[i].getAppearance().setAttribute(EDGE_DRAW, false);
			BallAndStickFactory basf = new BallAndStickFactory(ifs);
			basf.setStickColor(Color.red);
			basf.setStickRadius(stickRadius);
			basf.setBallColor(Color.blue);
			basf.setBallRadius(ballRadius);
			basf.setMetric(i-1);
			basf.update();
			sigs[i].addChild(basf.getSceneGraphComponent());
			world.addChild(sigs[i]);
			sigs[i].setVisible(false);
		}
		// add the boundary at infinity for hyperbolic space
		wireframeSphere = Primitives.wireframeSphere();
		wireframeSphere.getAppearance().setAttribute(METRIC, Pn.EUCLIDEAN);
		wireframeSphere.getAppearance().setAttribute(LINE_SHADER+"."+TUBE_RADIUS,.0015);
		wireframeSphere.getAppearance().setAttribute(LIGHTING_ENABLED, false);
		sigs[0].addChild(wireframeSphere);
		
		// add a small sphere to the scene
		SceneGraphComponent sphere = SceneGraphUtility.createFullSceneGraphComponent();
		sphere.addChild(SphereUtility.tessellatedCubeSphere(7));
		sphere.getAppearance().setAttribute(EDGE_DRAW, false);
		MatrixBuilder.euclidean().scale(.2).assignTo(sphere);
		world.addChild(sphere);
		wireframeSphere.setVisible(metric == Pn.HYPERBOLIC);
		return world;
	}

	// some variables which depend on the metric -- hence we have three of each
	double[][] falloffs =   {{1.5,.25,0},{.5,.5,0},{.5, .5, 0}};
	double[][] cameraClips = {{.001,2},{.01, 1000},{.01,-.05}};
	double distance = .5;
	double[] unitD = {Math.tanh(distance), distance, Math.tan(distance)};
	/** 
	 * Update the scene graph (generally as a result of changed metric
	 * This involves:
	 * 	resetting the near/far clipping planes
	 *  making the correct branch of the scene graph visible
	 *  resetting the transformation of the world, the camera node, and the light node
	 *  turning on the OpenGL shader for then non-euclidean cases
	 *  making the hyperbolic boundary sphere visible if the metric is ... hyperbolic
	 */
	public void update()	{
		viewer.getSceneRoot().getAppearance().setAttribute("metric", metric);
		CameraUtility.getCamera(viewer).setNear(cameraClips[metric+1][0]);
		CameraUtility.getCamera(viewer).setFar(cameraClips[metric+1][1]);
		for (int i = 0; i<3; ++i) sigs[i].setVisible(false);
		sigs[metric+1].setVisible(true);
		// this is all we have to do to tell the backend to use the non-euclidean vertex shader
		world.getAppearance().setAttribute("useGLSL", metric != Pn.EUCLIDEAN);
		pointLight.setFalloff(falloffs[metric+1]);
		MatrixBuilder.init(null, metric).rotateX(-Math.PI/3).assignTo(world);
		MatrixBuilder.init(null, metric).translate(0,0,unitD[metric+1]).assignTo(lightNode);
		MatrixBuilder.init(null, metric).translate(0,0,2*unitD[metric+1]).
			assignTo(CameraUtility.getCameraNode(viewer));
		wireframeSphere.setVisible(metric == Pn.HYPERBOLIC);
		viewer.renderAsync();
	}

	/** Generate a "unit disk" in the requested metric.  The resulting topological "point set" is identical in all
	 * cases, but the meshing respects the metric.  That is, a polar subdivision is carried out in which the radial
	 * steps are equal (in the metric).  
	 * @param metric
	 * @return  A quad mesh representation of the unit disk
	 */
	private static IndexedFaceSet getDisk(final int metric) {
		ParametricSurfaceFactory psf = new ParametricSurfaceFactory();
		psf.setMetric(metric);
		psf.setULineCount(17);
		psf.setVLineCount(15);
	    psf.setUMin(0.0);
	    psf.setUMax(Math.PI * 2);
	    psf.setVMin(0.01);
	    psf.setVMax(.995);
	    psf.setGenerateFaceNormals(true);
	    psf.setGenerateEdgesFromFaces(true);
	    psf.setGenerateVertexNormals(true);
		psf.setImmersion( new Immersion()		{
			double[] scales = {2.5,1,Math.PI/4};
			double[] foo = {1,0,0,0};
			public boolean isImmutable() {
				return false;
			}

			public int getDimensionOfAmbientSpace() {
				return 4;
			}

			public void evaluate(double u, double v, double[] xyz, int index) {
				double[] p = Pn.dragTowards(null, P3.originP3, foo, v * scales[metric+1], metric);
				
				xyz[0] = Math.cos(u) * p[0];
				xyz[1] = Math.sin(u) * p[0];
				xyz[2] = 0.0;
				xyz[3] = p[3];
			}
			
		});
		psf.update();
		IndexedFaceSet ifs = psf.getIndexedFaceSet();
		ifs.setName("disk"+(metric+1));
		return ifs;
	}
	

}
