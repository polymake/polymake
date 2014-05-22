/*
 * Created on Jan 11, 2009
 *
 */
package de.jreality.tutorial.scene;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;

import de.jreality.geometry.SphereUtility;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.scene.event.CameraEvent;
import de.jreality.scene.event.CameraListener;
import de.jreality.scene.pick.Graphics3D;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.util.CameraUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * A simple demo showing how to use a camera listener.  It implements a simple level of
 * detail algorithm to display different resolutions of a tessellated icosahedra based
 * on the apparent size of the object.
 * 
 * To run: execute the main method, and in the graphics window activate the camera zoom tool 
 * via the mouse scroll wheel.  The camera listener calculates the expected screen extent
 * of the sphere and chooses a tessellation of the sphere which keeps the size of the
 * triangles roughly constant.
 * 
 * @author gunn
 *
 */public class CameraListenerExample {

	SceneGraphComponent sphereSGC = SceneGraphUtility.createFullSceneGraphComponent("sphere");
	private DefaultPolygonShader dps;
	private DefaultLineShader dls;
	private Viewer viewer;
	private SceneGraphPath camPath;
	private SceneGraphPath spherePath;
	private IndexedFaceSet[] levelsOfDetailSpheres;
	private Color[] colors;
	private int numLevels = 6, lastLevel = -1;
	public void doIt() {
		
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(sphereSGC.getAppearance(), true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(false);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.blue);
		dps.setSmoothShading(false);		// I like it this way ...
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setTubeDraw(true);
		dls.setLineWidth(2.0);		// in case you want no tubes...
		dls.setDiffuseColor(Color.white);

		sphereSGC.getAppearance().setAttribute(POLYGON_SHADER+"."+DIFFUSE_COLOR, Color.white);
		viewer = JRViewer.display(sphereSGC);
		viewer.getSceneRoot().addTool(new ClickWheelCameraZoomTool());
		// set up two scene graph paths, one to the camera and one to the sphere
		camPath = viewer.getCameraPath();
		spherePath = SceneGraphUtility.getPathsToNamedNodes(
				viewer.getSceneRoot(), 
				"sphere").get(0);
		// set up a level-of-detail structure representing a sphere
		levelsOfDetailSpheres = new IndexedFaceSet[numLevels];
		colors = new Color[numLevels];
		for (int i = 0; i<numLevels; ++i) {
			levelsOfDetailSpheres[i] = SphereUtility.tessellatedIcosahedronSphere(i);
			colors[i] = new Color(i/(numLevels-1f), 0, ((numLevels-1)-i)/(numLevels-1f));
		}
		// set up geometry based on initial conditions
		update();
		// add the listener, which just calls this class's update() method
		CameraListener camListener = new CameraListener() {
			public void cameraChanged(CameraEvent ev) {
				update();
			}
			
		};
		CameraUtility.getCamera(viewer).addCameraListener(camListener);
		CameraUtility.encompass(viewer);
	}
	
	/**
	 * Use the path to the object and to the sphere, plus other information from the
	 * Camera, to calculate the net object to NDC (normalized device coordinates) transformation.
	 * Use this to estimate how large the object appears.
	 * 
	 * Note: the matrix calculations here could be done somewhat more conveniently using
	 * an instance of {@link Graphics3D} using the constructor {@link Graphics3D#Graphics3D(Viewer)}
	 * and setting {@link Graphics3D#setCurrentPath(SceneGraphPath)} to <i>spherePath</i>, then
	 * this method has only to call {@link Graphics3D#getObjectToNDC()}.  But it's probably good to see how to do it
	 * by hand.
	 */
	public void update()	{
		double[] s2w = spherePath.getMatrix(null);
		double[] w2c = camPath.getInverseMatrix(null);
		double[] c2ndc = CameraUtility.getCameraToNDC(viewer);
		// the net transformation object to normalized device coordinates is the 
		// product of the three matrices above.
		double[] s2ndc = Rn.times(null, Rn.times(null, c2ndc, w2c), s2w);
		double size = CameraUtility.getNDCExtent(s2ndc);
		// lowest resolution (0) corresponds to an ndc extent of exp(-4) ~= .02
		// each doubling of this extent moves up to the next tessellation, so
		// approximate screen size of triangles remains roughly constant.
		double logs = Math.log(size)/Math.log(2.0)+4;
		if (logs < 0) logs = 0;
		if (logs > (numLevels-1)) logs = (numLevels-1);
		int which = (int) logs;
		System.err.println("size = "+size+" which = "+which);
		if (which == lastLevel) return;
		sphereSGC.setGeometry(levelsOfDetailSpheres[which]);
		dls.setTubeRadius(.033*Math.pow(.5, which));
		dps.setDiffuseColor(colors[which]);
		lastLevel = which;
	}
	
	public static void main(String[] args) {	
		CameraListenerExample cle = new CameraListenerExample();
		cle.doIt();
	}
	

	
}
