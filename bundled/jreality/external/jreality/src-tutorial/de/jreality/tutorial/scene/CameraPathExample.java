package de.jreality.tutorial.scene;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.List;

import javax.swing.Timer;

import de.jreality.geometry.FrameFieldType;
import de.jreality.geometry.PolygonalTubeFactory;
import de.jreality.geometry.Primitives;
import de.jreality.geometry.TubeFactory;
import de.jreality.geometry.TubeUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Camera;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.util.SceneGraphUtility;

public class CameraPathExample {

	private static List<SceneGraphPath> lightPaths;
	private static SceneGraphComponent movingLightSGC;
	static boolean frenet = false;
	private static PolygonalTubeFactory polygonalTubeFactory;
	public static void main(String[] args) {
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		final SceneGraphComponent child1 =  SceneGraphUtility.createFullSceneGraphComponent("knot"),
			child2 = SceneGraphUtility.createFullSceneGraphComponent("point");
		
		world.addChildren(child1, child2);
		IndexedLineSet torus1 = Primitives.discreteTorusKnot(1, .4, 2, 3, 1000);
		polygonalTubeFactory = new PolygonalTubeFactory(torus1, 0);
		polygonalTubeFactory.setClosed(true);
		polygonalTubeFactory.setMatchClosedTwist(true);
		polygonalTubeFactory.setGenerateTextureCoordinates(true);
		polygonalTubeFactory.setRadius(.1);
		polygonalTubeFactory.setGenerateEdges(true);
		polygonalTubeFactory.setFrameFieldType(frenet ? FrameFieldType.FRENET : FrameFieldType.PARALLEL);
		polygonalTubeFactory.update();
		
		IndexedFaceSet torus1Tubes = polygonalTubeFactory.getTube();
		child1.setGeometry(torus1Tubes);
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(child1.getAppearance(), true);
		dgs.setShowFaces(true);
		dgs.setShowPoints(false);
		dgs.setShowLines(false);
		DefaultLineShader dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setTubeDraw(true);
		dls.setTubeRadius(.003);
		dls.setDiffuseColor(Color.green);

		SimpleTextureFactory stf = new SimpleTextureFactory();
		stf.setColor(0, new Color(0,0,0,0));
		stf.update();
		ImageData id = stf.getImageData();
		final DefaultPolygonShader dpls = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dpls.setDiffuseColor(Color.pink);
		dpls.setAmbientCoefficient(.05);
		Texture2D tex = TextureUtility.createTexture(child1.getAppearance(), POLYGON_SHADER,id);
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(5,50,1).getMatrix());

		final SceneGraphComponent axes = TubeFactory.getXYZAxes();
		// flip the z-direction of the axes to agree with fact that the camera
		// points in the negative z-direction
		MatrixBuilder.euclidean().scale(.5).scale(1,1,-1).assignTo(axes);
		child2.addChild(axes);
		dgs = ShaderUtility.createDefaultGeometryShader(child2.getAppearance(), true);
		dgs.setShowPoints(true);
		DefaultPointShader dps = (DefaultPointShader) dgs.createPointShader("default");
		dps.setSpheresDraw(true);
		dps.setPointRadius(.01);
		dps.setDiffuseColor(Color.white);
		
		final TubeUtility.FrameInfo[] frames = polygonalTubeFactory.getFrameField();
		// set up a timer to move the position of the second camera along the
		// torus knot, using the frame field from the tube factory.
		// the z-direction has to be flipped to agree with the fact that the
		// jReality camera points in the negative z-direction
		final Timer movepoint = new Timer(40, new ActionListener() {
			int count = 0;
			public void actionPerformed(ActionEvent e) {
				MatrixBuilder.euclidean(new Matrix(frames[count].frame.clone())).
					rotateZ(Math.PI/2 + (frenet ? 0.0 : frames[count].phi)).
					scale(1,1,-1).assignTo(child2);
				count = (count+1)%frames.length;
			}
			
		});
		movepoint.start();
		
//		final ViewerApp va = new ViewerApp(world); 
//		va.setAttachNavigator(true);
//		va.setExternalNavigator(false);
//		va.update();
//		va.display();
//		final Viewer viewer = va.getCurrentViewer();
//		CameraUtility.encompass(viewer);
		final Viewer viewer = JRViewer.display(world);
		final SceneGraphPath campath = viewer.getCameraPath();
		Camera camera = new Camera();
		camera.setNear(.015);
		camera.setFieldOfView(90);
		// set up second camera path, ending in the moving point on the curve
		child2.setCamera(camera);
		final SceneGraphPath campath2 = SceneGraphUtility.getPathsBetween(
				viewer.getSceneRoot(), 
				child2).get(0);
		campath2.push(child2.getCamera());
		movingLightSGC = new SceneGraphComponent("moving light");
		child2.addChild(movingLightSGC);
		movingLightSGC.setVisible(false);
		
		lightPaths = SceneGraphUtility.collectLights(viewer.getSceneRoot());
		Light l = (Light) lightPaths.get(0).getLastElement();
		movingLightSGC.setLight(l);
		
		Component comp = ((Component) viewer.getViewingComponent());
		comp.addKeyListener(new KeyAdapter() {
 				public void keyPressed(KeyEvent e)	{ 
					switch(e.getKeyCode())	{
						
					case KeyEvent.VK_H:
						System.err.println("	1: toggle camera path");
						break;
		
					case KeyEvent.VK_1:
						final SceneGraphPath cp = viewer.getCameraPath();
						Scene.executeWriter(viewer.getSceneRoot(), new Runnable() {

							public void run() {
								boolean alternative = cp == campath;
								viewer.setCameraPath(alternative ? campath2 : campath);
								axes.setVisible(!alternative);
								dpls.setAmbientCoefficient(alternative ? .2 : .05);
								movingLightSGC.setVisible(alternative);
								for (SceneGraphPath sgp : lightPaths)	{
									sgp.getLastComponent().setVisible(!alternative);
								}

							}
							
						});
						break;
					case KeyEvent.VK_2:
						if (movepoint.isRunning()) movepoint.stop();
						else movepoint.start();
						break;
//					currently not working
//					case KeyEvent.VK_3:
//						movepoint.stop();
//						frenet = !frenet;
//						polygonalTubeFactory.setFrameFieldType(frenet ? FrameFieldType.FRENET : FrameFieldType.PARALLEL);
//						polygonalTubeFactory.update();
//						break;
				}
		
				}
			});

	}

}
