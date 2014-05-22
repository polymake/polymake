package de.jreality.backends.testingApps;

import java.awt.Color;
import java.awt.Frame;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.media.opengl.GL3;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;

import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;

/**
 *
 * @author Benjamin Kutschan
 */
// converted this to openGL 3.3
// replaced triangle by squares with different colors
// TODO implement depth peeling
public class Texturing implements GLEventListener{
	
	static float quadCoords[] = {
		-.2f, .2f, 0.4f, 1,
		.9f, .2f, 0.4f, 1,
		.8f, -.9f, 0.7f, 1,
		.8f, -.9f, 0.7f, 1,
		-.3f, -.9f, 0.7f, 1,
		-.2f, .2f, 0.4f, 1,
	   };
	
	static float testTexCoords[] = {
		0,0,0,0,
		0,1,0,0,
		1,1,0,0,
		1,1,0,0,
		1,0,0,0,
		0,0,0,0
	};
	
	public static GLShader texture = new GLShader("testing/texture.v", "testing/texture.f");
	GLVBOFloat quadVerts, copyTex;
	@Override
    public void reshape(GLAutoDrawable dr, int x, int y, int width, int height){
    	System.out.println("Reshape");
    	this.width = width;
    	this.height = height;
    }
    
	//private int[] texs = new int[1];
    @Override
    public void init( GLAutoDrawable dr ) {
    	System.out.println("Init");
    	width = dr.getWidth();
    	height = dr.getHeight();
    	GL3 gl = dr.getGL().getGL3();
    	texture.init(gl);
    	
    	quadVerts = new GLVBOFloat(gl, quadCoords, "vertex_coordinates");
    	copyTex = new GLVBOFloat(gl, testTexCoords, "texture_coordinates");
    	
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	
    	Appearance a = new Appearance();
    	
    	ImageData imageData = null;
		double scale = 1;
		// get the image for the texture first
		SimpleTextureFactory stf = new SimpleTextureFactory();
		stf.setColor(0, new Color(0,0,0,0));	// gap color in weave pattern is totally transparent
		stf.setColor(1, new Color(255,0,100));
		stf.setColor(2, new Color(255,255,0));
		stf.update();
		imageData = stf.getImageData();
		scale = 10;
		tex = TextureUtility.createTexture(a, CommonAttributes.POLYGON_SHADER, imageData);
    	
    }
    
    private Texture2D tex;
    @Override
    public void display(GLAutoDrawable dr){
    	System.out.println("Display");
    	GL3 gl = dr.getGL().getGL3();
    	
    	//set up framebuffer and clear its depth
    	gl.glClearDepth(1);
    	gl.glClear(gl.GL_DEPTH_BUFFER_BIT);
    	gl.glClearColor(0.5f, 0.5f, 0.5f, 1);
    	gl.glClear(gl.GL_COLOR_BUFFER_BIT);
    	
    	gl.glViewport(0, 0, width, height);
    	
    	texture.useShader(gl);
    	
    	Texture2DLoader.load(gl, tex, gl.GL_TEXTURE0);
    	gl.glUniform1i(gl.glGetUniformLocation(texture.shaderprogram, "image"), 0);
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quadVerts.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(texture.shaderprogram, quadVerts.getName()), quadVerts.getElementSize(), quadVerts.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(texture.shaderprogram, quadVerts.getName()));
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, copyTex.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(texture.shaderprogram, copyTex.getName()), copyTex.getElementSize(), copyTex.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(texture.shaderprogram, copyTex.getName()));
    	
    	
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, quadVerts.getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(texture.shaderprogram, quadVerts.getName()));
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(texture.shaderprogram, copyTex.getName()));
    	
    	texture.dontUseShader(gl);
    }
    
    @Override
    public void dispose( GLAutoDrawable glautodrawable ) {
    	System.out.println("Dispose");
    }
    
    private int width, height;
    
    public static void main( String [] args ) {
        GLProfile glprofile = GLProfile.get("GL3");
        GLCapabilities glcapabilities = new GLCapabilities( glprofile );
        final GLCanvas glcanvas = new GLCanvas( glcapabilities );
        Texturing texturing = new Texturing();
        glcanvas.addGLEventListener(texturing);
        //glcanvas.addMouseMotionListener(depthPeeling);

        final Frame frame = new Frame( "One Triangle AWT" );
        frame.add( glcanvas );
        frame.addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent windowevent ) {
                frame.remove( glcanvas );
                frame.dispose();
                System.exit( 0 );
            }
        });

        frame.setSize( 640, 480 );
        frame.setVisible( true );
    }

}