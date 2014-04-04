package de.jreality.backends.testingApps;

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

/**
 *
 * @author Benjamin Kutschan
 */
// converted this to openGL 3.3
// replaced triangle by squares with different colors
// TODO implement depth peeling
public class DepthPeeling implements GLEventListener{
	
	static float nonT[] = {
		0, -.1f, .9f, 1,
		.1f, .33f, .1f, 1,
		.9f, .33f, .1f, 1,
		.9f, .33f, .1f, 1,
		.8f, -.1f, .9f, 1,
		0, -.1f, .9f, 1
	};
	
	static float quadCoords[] = {
		-.2f, .2f, 0.4f, 1,
		.9f, .2f, 0.4f, 1,
		.8f, -.9f, 0.7f, 1,
		.8f, -.9f, 0.7f, 1,
		-.3f, -.9f, 0.7f, 1,
		-.2f, .2f, 0.4f, 1,
		
		-.3f, .8f, .75f, 1,
		.8f, .8f, .75f, 1,
		.8f, -.3f, .75f, 1,
		.8f, -.3f, .75f, 1,
		-.3f, -.3f, .75f, 1,
		-.3f, .8f, .75f, 1,
		
		-.8f, .3f, .5f, 1,
		.3f, .3f, .5f, 1,
		.3f, -.8f, .5f, 1,
		.3f, -.8f, .5f, 1,
		-.8f, -.8f, .5f, 1,
		-.8f, .3f, .5f, 1
		
		
	   };
	static final float alpha = 0.6f;
	static float quadColors[] = {
		1, 0, 0, alpha,
		1, 0, 0, alpha,
		1, 0, 0, alpha,
		1, 0, 0, alpha,
		1, 0, 0, alpha,
		1, 0, 0, alpha,
		
		0, 1, 0, alpha,
		0, 1, 0, alpha,
		0, 1, 0, alpha,
		0, 1, 0, alpha,
		0, 1, 0, alpha,
		0, 1, 0, alpha,
		
		0, 0, 1, alpha,
		0, 0, 1, alpha,
		0, 0, 1, alpha,
		0, 0, 1, alpha,
		0, 0, 1, alpha,
		0, 0, 1, alpha
		
		
	   };
	
	static float testQuadCoords[] = {
		-1f, 1f, 0.1f, 1,
		1f, 1f, 0.1f, 1,
		1f, -1f, 0.1f, 1,
		1f, -1f, 0.1f, 1,
		-1f, -1f, 0.1f, 1,
		-1f, 1f, 0.1f, 1
	};
	
	static float testTexCoords[] = {
		0,0,0,0,
		0,1,0,0,
		1,1,0,0,
		1,1,0,0,
		1,0,0,0,
		0,0,0,0
	};
	
	public static GLShader depth = new GLShader("testing/depth.v", "testing/depth.f");
	public static GLShader transp = new GLShader("testing/transparent.v", "testing/transparent.f");
	public static GLShader copy = new GLShader("testing/copy.v", "testing/copy.f");
	public static GLShader nonTS = new GLShader("testing/nonts.v", "testing/nonts.f");
	GLVBOFloat quadVerts, quadCols, copyCoords, copyTex, nonTr;
	@Override
    public void reshape(GLAutoDrawable dr, int x, int y, int width, int height){
    	System.out.println("Reshape");
    	this.width = width;
    	this.height = height;
    	resizeTexture(dr.getGL().getGL3());
    }
    
	
	private int[] texs = new int[3];
	
	
	private void resizeTexture(GL3 gl){
		//bind color texture to framebuffer object 1
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[2]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGBA8, width, height, 0, gl.GL_RGBA, gl.GL_UNSIGNED_BYTE, null);
    	
    	//bind depth texture to framebuffer object 1
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[1]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_DEPTH_COMPONENT, width, height, 0, gl.GL_DEPTH_COMPONENT, gl.GL_FLOAT, null);
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    	
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_COLOR_ATTACHMENT0, gl.GL_TEXTURE_2D, texs[2], 0);
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_DEPTH_ATTACHMENT, gl.GL_TEXTURE_2D, texs[1], 0);
    	
		//bind depth texture to framebuffer object 0
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[0]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_DEPTH_COMPONENT, width, height, 0, gl.GL_DEPTH_COMPONENT, gl.GL_FLOAT, null);
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
    	
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_DEPTH_ATTACHMENT, gl.GL_TEXTURE_2D, texs[0], 0);
    	System.out.println(" " + gl.GL_FRAMEBUFFER_COMPLETE + " " + gl.glCheckFramebufferStatus(gl.GL_FRAMEBUFFER));
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);

	}
	private void initTextureFramebuffer(GL3 gl){
		gl.glGenTextures(3, texs, 0);
		gl.glGenFramebuffers(2, fbos, 0);
    	resizeTexture(gl);
	}
	
	
	private int[] queries = new int[1];
	private void startQuery(GL3 gl){
		gl.glBeginQuery(gl.GL_SAMPLES_PASSED, queries[0]);
	}
	private int endQuery(GL3 gl){
		gl.glEndQuery(gl.GL_SAMPLES_PASSED);
    	int counter = 0;
    	do{
    		counter++;
    		gl.glGetQueryObjectuiv(queries[0],gl.GL_QUERY_RESULT_AVAILABLE, queryresavail, 0);
    		System.out.println("not true yet: " + counter);
    	}while(queryresavail[0] != gl.GL_TRUE);
    	gl.glGetQueryObjectuiv(queries[0] ,gl.GL_QUERY_RESULT, queryres, 0);
    	return queryres[0];
    	//System.out.println("Query result is " + queryres[0]);
	}
	
    @Override
    public void init( GLAutoDrawable dr ) {
    	System.out.println("Init");
    	width = dr.getWidth();
    	height = dr.getHeight();
    	GL3 gl = dr.getGL().getGL3();
    	depth.init(gl);
    	transp.init(gl);
    	copy.init(gl);
    	nonTS.init(gl);
    	
    	quadVerts = new GLVBOFloat(gl, quadCoords, "vertex_coordinates");
    	quadCols = new GLVBOFloat(gl, quadColors, "vertex_colors");
    	copyCoords = new GLVBOFloat(gl, testQuadCoords, "vertex_coordinates");
    	copyTex = new GLVBOFloat(gl, testTexCoords, "texture_coordinates");
    	nonTr = new GLVBOFloat(gl, nonT, "vertex_coordinates");
    	
    	gl.glGenQueries(1, queries, 0);
    	
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	initTextureFramebuffer(gl);
    	
    }
    @Override
    public void display(GLAutoDrawable dr){
    	System.out.println("Display");
    	GL3 gl = dr.getGL().getGL3();
    	
    	//set up framebuffer and clear its depth
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    	gl.glClearDepth(1);
    	gl.glClear(gl.GL_DEPTH_BUFFER_BIT);
    	gl.glClearColor(0.5f, 0.5f, 0.5f, 1);
    	gl.glClear(gl.GL_COLOR_BUFFER_BIT);
    	//draw nontransparent objects into framebuffer
    	drawNonTransparent(gl);
    	int quer = 1;
    	//with this loop it draws as many layers as neccessary to complete the scene
    	//you can experiment by drawing only one layer or two and then view the result (see the comment after the loop)
    	do{
    		//draw transparent objects into FBO with reverse depth values
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
        	startQuery(gl);
        	peelDepth(gl);
        	quer = endQuery(gl);
        	//draw on the SCREEN
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    		addOneLayer(gl);
    	}while(quer != 0);
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
    	//you can change the number here:
    	//0 means the current depth layer generated by peelDepth()
    	//1 the depth layer of the final image generated by addOneLayer()
    	//2 the color layer of the final image generated by addOneLayer()
    	this.copyFBO2FB(gl, 2);
    	
    }
    
    @Override
    public void dispose( GLAutoDrawable glautodrawable ) {
    	System.out.println("Dispose");
    }
    
    @SuppressWarnings("static-access")
	public void peelDepth(GL3 gl){
    	
    	gl.glViewport(0, 0, width, height);
    	
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	gl.glActiveTexture(gl.GL_TEXTURE0);
    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[1]);
    	depth.useShader(gl);
    	
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "image"), 0);
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "width"), width);
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "height"), height);
    	
    	gl.glClearColor(0, 0, 0, 1);
    	gl.glClear(gl.GL_COLOR_BUFFER_BIT);
    	gl.glClearDepth(1);
    	gl.glClear(gl.GL_DEPTH_BUFFER_BIT);
    	
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quadVerts.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(depth.shaderprogram, quadVerts.getName()), quadVerts.getElementSize(), quadVerts.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(depth.shaderprogram, quadVerts.getName()));
    	
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, quadVerts.getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(depth.shaderprogram, quadVerts.getName()));
    	
    	depth.dontUseShader(gl);
    }
    
    private void addOneLayer(GL3 gl){
    	gl.glViewport(0, 0, width, height);
    	
    	gl.glEnable(gl.GL_BLEND);
    	gl.glBlendEquation(gl.GL_FUNC_ADD);
    	gl.glBlendFunc(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA);
    	
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	gl.glActiveTexture(gl.GL_TEXTURE0);
    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[0]);
    	transp.useShader(gl);
    	
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "image"), 0);
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "width"), width);
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "height"), height);
    	
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quadVerts.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(transp.shaderprogram, quadVerts.getName()), quadVerts.getElementSize(), quadVerts.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(transp.shaderprogram, quadVerts.getName()));
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quadCols.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(transp.shaderprogram, quadCols.getName()), quadCols.getElementSize(), quadCols.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(transp.shaderprogram, quadCols.getName()));
    	
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, quadVerts.getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(transp.shaderprogram, quadVerts.getName()));
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(transp.shaderprogram, quadCols.getName()));
    	
    	transp.dontUseShader(gl);
    	
    	gl.glDisable(gl.GL_BLEND);
    }
    
    private void drawNonTransparent(GL3 gl){
    	gl.glViewport(0, 0, width, height);
    	
    	gl.glDisable(gl.GL_BLEND);
    	nonTS.useShader(gl);
    	
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, nonTr.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(nonTS.shaderprogram, nonTr.getName()), nonTr.getElementSize(), nonTr.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(nonTS.shaderprogram, nonTr.getName()));
    	
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, nonTr.getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(nonTS.shaderprogram, nonTr.getName()));
    	
    	nonTS.dontUseShader(gl);
    }
    
    private void copyFBO2FB(GL3 gl, int tex){
    	gl.glViewport(0, 0, width, height);
    	
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	gl.glActiveTexture(gl.GL_TEXTURE0);
    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[tex]);
    	gl.glUniform1i(gl.glGetUniformLocation(transp.shaderprogram, "image"), 0);
    	
    	gl.glDisable(gl.GL_DEPTH_TEST);
    	
    	copy.useShader(gl);
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, copyCoords.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(copy.shaderprogram, copyCoords.getName()), copyCoords.getElementSize(), copyCoords.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(copy.shaderprogram, copyCoords.getName()));
    	
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, copyTex.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(copy.shaderprogram, copyTex.getName()), copyTex.getElementSize(), copyTex.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(copy.shaderprogram, copyTex.getName()));
    	
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, copyCoords.getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(copy.shaderprogram, copyCoords.getName()));
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(copy.shaderprogram, copyTex.getName()));
    	
    	copy.dontUseShader(gl);
    }
    
    private int[] fbos = new int[2];
    
    private int width, height;
    
    private int[] queryresavail = new int[1];
    private int[] queryres = new int[1];
 
	
    public static void main( String [] args ) {
        GLProfile glprofile = GLProfile.get("GL3");
        GLCapabilities glcapabilities = new GLCapabilities( glprofile );
        final GLCanvas glcanvas = new GLCanvas( glcapabilities );
        DepthPeeling depthPeeling = new DepthPeeling();
        glcanvas.addGLEventListener(depthPeeling);
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