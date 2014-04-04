package de.jreality.tutorial.viewer;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

import javax.swing.Timer;

import de.jreality.geometry.Primitives;
import de.jreality.jogl.AbstractViewer;
import de.jreality.jogl.JOGLFBOViewer;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tools.RotateTool;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.ui.viewerapp.ViewerSwitch;
import de.jreality.util.SceneGraphUtility;

/**
 * This is a simple example showing how to use the class {@link JOGLFBOViewer}, a subclass
 * of {@link Viewer}. 
 * This class allows you to use the rendered image from the JOGLFBOViewer as the source
 * of a {@link Texture2D} for use in another Viewer.
 * <p>
 * When the example runs, you should see a black square with a rotating icosahedron within
 * it.  You can rotate the quadrilateral, then it will be clear the icosahedron is a 2D image
 * textured onto it.  The rotation of the icosahedron can be toggled via the '1' key.
 * <p>
 * Warning: JOGLFBOViewer is not a standard jReality class. It was written to show "proof of concept".
 * It is not well-integrated into the rest of jReality (as this example shows).  But because
 * it might be useful even in its current form, I'm going ahead and writing this tutorial 
 * example.  Perhaps someone will even decide it's worth improving and figure out how to
 * do it right.
 * 
 * @author gunn
 *
 */
public class JOGLFBOTextureExample  {

	private Texture2D tex2d;
	private SceneGraphComponent world;
	boolean rotating = true,
		stereoTexture = false;
	// this method creates a simple textured quadrilateral:
	// the "movie screen" onto which the texture will be projected.
	public SceneGraphComponent makeWorld() {
		world = SceneGraphUtility.createFullSceneGraphComponent("world");
		SceneGraphComponent screen = SceneGraphUtility.createFullSceneGraphComponent("world");
		IndexedFaceSet square = Primitives.texturedQuadrilateral();
		screen.setGeometry(square);
		MatrixBuilder.euclidean().scale(stereoTexture ? 2 : 1,1,1).assignTo(screen);
		world.addChild(screen);
		world.getAppearance().setAttribute("polygonShader.diffuseColor", Color.white);
		world.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, false);
		world.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
		SimpleTextureFactory stf = new SimpleTextureFactory();
		stf.update();
		ImageData img = stf.getImageData(); 
		tex2d = TextureUtility.createTexture(world.getAppearance(), "polygonShader", img);
		
		return world;
	}
	
	JOGLFBOViewer joglFBOViewer;
	// the following sets up a second viewer, an instance of JOGLFBOViewer
	// this viewer is just used as the source of a texture which
	// is then visible in the main viewer.
	// in this example, we include a timer to rotate the scene smoothly
	// this is then visible in the texture displayed in the main viewer.
	private void setupJOGLFBOViewer() {
		// set up a simple, self-contained scene featuring an icosahedron
		// provided with light and camera
	    SceneGraphComponent rootNode = new SceneGraphComponent("root");
	    SceneGraphComponent cameraNode = new SceneGraphComponent("camera");
	    final SceneGraphComponent geometryNode = new SceneGraphComponent("geometry");
	    SceneGraphComponent lightNode = new SceneGraphComponent("light");
	    
	    rootNode.addChild(geometryNode);
	    rootNode.addChild(cameraNode);
	    cameraNode.addChild(lightNode);
	    
	    Light dl=new DirectionalLight();
	    lightNode.setLight(dl);
	    
	    IndexedFaceSet ifs = Primitives.icosahedron(); 
	    geometryNode.setGeometry(ifs);
	    
	    RotateTool rotateTool = new RotateTool();
	    geometryNode.addTool(rotateTool);

	    MatrixBuilder.euclidean().translate(0, 0, 3).assignTo(cameraNode);

		Appearance rootApp= new Appearance();
	    rootApp.setAttribute(CommonAttributes.BACKGROUND_COLOR, new Color(0f, .1f, .1f));
	    rootApp.setAttribute(CommonAttributes.DIFFUSE_COLOR, new Color(1f, 0f, 0f));
	    rootNode.setAppearance(rootApp);
	        
	    Camera camera = new Camera();
	    cameraNode.setCamera(camera);
	    SceneGraphPath camPath = new SceneGraphPath(rootNode, cameraNode);
	    camPath.push(camera);
	    
	    // create a viewer to view this simple scene
		joglFBOViewer = new JOGLFBOViewer(camPath, rootNode);
		joglFBOViewer.setSize(new Dimension(512,stereoTexture ? 256 : 512));
		joglFBOViewer.setTexture2D(tex2d);
		// set this to false to get better quality but slower texture
		joglFBOViewer.getJoglfbo().setAsTexture(true);
		if (stereoTexture) {
		    camera.setFocus(4.0);
		    camera.setEyeSeparation(.45);
		    camera.setStereo(true);	    	
			joglFBOViewer.setStereoType(AbstractViewer.CROSS_EYED_STEREO);
		}

		// a timer to rotate the scene smoothly around the vertical axis
	    Timer timer = new Timer(20, new ActionListener() {
	    	double t = 0;
			public void actionPerformed(ActionEvent e) {
				double[] mat = MatrixBuilder.euclidean().rotateY(t).getArray();
				new Matrix(mat).assignTo(geometryNode);
				if (rotating) t += .01;
				// this is just there to trigger rendering in the main viewer
				MatrixBuilder.euclidean().assignTo(world);
			}
	    	
	    });
	    timer.start();
	    
	    
	}
	
	public static void main(String args[])	{
		final JOGLFBOTextureExample tfbot = new JOGLFBOTextureExample();
		tfbot.makeWorld();
		Viewer jrv = JRViewer.display(tfbot.world);
		tfbot.setupJOGLFBOViewer();
		if (jrv instanceof ViewerSwitch)	jrv = ((ViewerSwitch)jrv).getCurrentViewer();
		if (jrv instanceof AbstractViewer) ((AbstractViewer) jrv).fbo = tfbot.joglFBOViewer;

		Component comp = ((Component) jrv.getViewingComponent());
		comp.addKeyListener(new KeyAdapter() {
 				public void keyPressed(KeyEvent e)	{ 
					switch(e.getKeyCode())	{
						
					case KeyEvent.VK_H:
						System.err.println("	1: toggle rotating");
						break;
		
					case KeyEvent.VK_1:
						tfbot.rotating = !tfbot.rotating;
						break;
				}
		
				}
			});

	}
}
