package de.jreality.portal;

import java.awt.geom.Rectangle2D;
import java.io.IOException;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLViewer;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.GlslProgram;
import de.jreality.util.CameraUtility;
import de.jreality.util.Input;

public class CylindricalPerspectiveViewer extends JOGLViewer {

	GlslProgram cylProg;
	Camera cam;
	
	public CylindricalPerspectiveViewer() {
		super();
		super.setStereoType(HARDWARE_BUFFER_STEREO);
		System.out.println("CYLINDRICAL PERSPECTIVE VIEWER");
		renderer=new JOGLRenderer(this) {
			@Override
			public void display(GL2 gl) {
				whichEye=CameraUtility.MIDDLE_EYE;
				super.display(gl);
			}
			@Override
			protected void setupLeftEye(int width, int height) {
				setViewport(0, 0, width, height);
				globalGL.glDrawBuffer(GL2.GL_BACK_LEFT);
				renderingState.clearBufferBits = clearColorBits | GL.GL_DEPTH_BUFFER_BIT;
				if (cylProg != null) cylProg.setUniform("eye", 0.);
				else whichEye=CameraUtility.LEFT_EYE;
				//System.out.println("\tLEFT EYE");
			}
			@Override
			protected void setupRightEye(int width, int height) {
				setViewport(0,0, width, height);
				globalGL.glDrawBuffer(GL2.GL_BACK_RIGHT);
				renderingState.clearBufferBits = clearColorBits | GL.GL_DEPTH_BUFFER_BIT;
				if (cylProg != null) cylProg.setUniform("eye", 1.);
				else whichEye=CameraUtility.RIGHT_EYE;
				//System.out.println("\tRIGHT EYE");
			}
		};
	}
	
	@Override
	public void setCameraPath(SceneGraphPath p) {
		super.setCameraPath(p);
		cam = p == null ? null : (Camera) p.getLastElement();
	}
	
	@Override
	public void setSceneRoot(SceneGraphComponent r) {
		checkProgram(r);
		super.setSceneRoot(r);
	}

	private void checkProgram(SceneGraphComponent sceneRoot) {
		if (sceneRoot != null && cylProg == null && sceneRoot.getAppearance()!=null) {
			try {
				cylProg = new GlslProgram(sceneRoot.getAppearance(), "polygonShader", Input.getInput(getClass().getResource("cylStereoVertexShader.glsl")), null);
				sceneRoot.getAppearance().setAttribute("polygonShadername", "glsl");
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		} else {
			cylProg=null;
			System.out.println("NO PROGRAM!!!");
		}
	}

	@Override
	public void render() {
		if (cylProg == null) checkProgram(getSceneRoot());
		if (cam != null) setParameters(cam);
		if (getStereoType() != HARDWARE_BUFFER_STEREO) setStereoType(HARDWARE_BUFFER_STEREO);
		super.render();
	}

	public void setParameters(Camera cam) {
	    if (cylProg != null) {
	    	Rectangle2D cv = cam.getViewPort();
	    	//System.out.println("setting viewport: "+cv);
	    	cylProg.setUniform("cv", new double[]{cv.getMinX(), cv.getMaxX(), cv.getMinY(), cv.getMaxY()});
	    	cylProg.setUniform("d", cam.getFocus());
	    	cylProg.setUniform("eyeSep", cam.getEyeSeparation());
	    	cylProg.setUniform("near", cam.getNear());
	    	cylProg.setUniform("far", cam.getFar());
	    }
	}
	
}
