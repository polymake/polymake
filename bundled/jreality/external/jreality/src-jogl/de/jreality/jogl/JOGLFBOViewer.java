package de.jreality.jogl;

import java.awt.Dimension;

import javax.media.opengl.GL2;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.Texture2D;

/**
 * This class is experimental. It is designed to be used as the source of a 2d
 * texture which is active in another viewer, called here the <i>main</i>
 * viewer. I don't really understand the OpenGL code; I copied some code off an
 * OpenGL tutorial somewhere and hacked at it until it worked.
 * 
 * The main methods here are {@link #setSize(Dimension)}, to set the size of the
 * texture image created, and {@link #setTexture2D(Texture2D)}, to set the
 * instance of Texture2D whose image data is to be replaced by the rendered
 * image of this viewer.
 * 
 * The implementation uses the class {@link JOGLFBO}, which hides the dirty
 * stuff from view.
 * 
 * For working example, see
 * <code>de.jreality.tutorial.viewer.JOGLFBOTextureExample</code> in the
 * jReality tutorial.
 * 
 * @author Charles Gunn
 * 
 */
public class JOGLFBOViewer extends JOGLViewer {

	JOGLFBO joglfbo;
	boolean init = false;

	public JOGLFBOViewer(SceneGraphComponent sgc) {
		this(null, sgc);

	}

	public JOGLFBOViewer(SceneGraphPath cameraPath,
			SceneGraphComponent sceneRoot) {
		super(cameraPath, sceneRoot);
		renderer = new JOGLRenderer(this);
		joglfbo = new JOGLFBO();
	}

	public void setSize(Dimension dim) {
		joglfbo.setSize(dim);
	}

	protected void render(GL2 gl) {
		if (!init) {
			renderer.init(gl);
			renderer.theFBO = joglfbo;
			init = true;
		}
		renderer.fboMode = true;
		renderer.display(gl);
		renderer.fboMode = false;
	}

	public void setTexture2D(Texture2D tex) {
		joglfbo.setTexture(tex);
	}

	public JOGLFBO getJoglfbo() {
		return joglfbo;
	}

}
