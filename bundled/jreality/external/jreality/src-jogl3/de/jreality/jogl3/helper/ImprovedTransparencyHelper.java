package de.jreality.jogl3.helper;

import java.util.List;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLSceneGraphComponentInstance.RenderableObject;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.GLVBOFloat;

public class ImprovedTransparencyHelper {
	public static int supersample = 2;
	
	//DONT FORGET TO INITIALIZE SHADERS WITH .init(GL3 gl)
	public static GLShader depth = new GLShader("transp/polygonDepth.v", "transp/depth.f");
	public static GLShader transp = new GLShader("nontransp/polygon.v", "transp/polygonTransp.f");
	public static GLShader transpSphere = new GLShader("nontransp/sphere.v", "transp/sphereTransp.f");
	public static GLShader copy = new GLShader("testing/copy.v", "testing/copy.f");
	static float testQuadCoords[] = {
		-1f, 1f, 0.1f, 1,
		1f, 1f, 0.1f, 1,
		1f, -1f, 0.1f, 1,
		1f, -1f, 0.1f, 1,
		-1f, -1f, 0.1f, 1,
		-1f, 1f, 0.1f, 1
	};
	
	static float testTexCoords[] = {
		0,1,0,0,
		1,1,0,0,
		1,0,0,0,
		1,0,0,0,
		0,0,0,0,
		0,1,0,0
	};
	public static GLVBOFloat copyCoords, copyTex;
	private static int[] queries = new int[2];
	
	private static int[] texs = new int[3];
	
	private static int[] fbos = new int[2];
    
    private static int[] queryresavail = new int[2];
    private static int[] queryres = new int[2];
	
	public static void resizeTexture(GL3 gl, int width, int height){
		//bind color texture to framebuffer object 1
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[2]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGBA8, supersample*width, supersample*height, 0, gl.GL_RGBA, gl.GL_UNSIGNED_BYTE, null);
    	
    	//bind depth texture to framebuffer object 1
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[1]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_DEPTH_COMPONENT, supersample*width, supersample*height, 0, gl.GL_DEPTH_COMPONENT, gl.GL_FLOAT, null);
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    	
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_COLOR_ATTACHMENT0, gl.GL_TEXTURE_2D, texs[2], 0);
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_DEPTH_ATTACHMENT, gl.GL_TEXTURE_2D, texs[1], 0);
    	
		//bind depth texture to framebuffer object 0
		gl.glBindTexture(gl.GL_TEXTURE_2D, texs[0]);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST);
    	
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, gl.GL_CLAMP_TO_EDGE);
    	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, gl.GL_CLAMP_TO_EDGE);

    	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_DEPTH_COMPONENT, supersample*width, supersample*height, 0, gl.GL_DEPTH_COMPONENT, gl.GL_FLOAT, null);
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
    	
    	gl.glFramebufferTexture2D(gl.GL_FRAMEBUFFER, gl.GL_DEPTH_ATTACHMENT, gl.GL_TEXTURE_2D, texs[0], 0);
    	System.out.println(" " + gl.GL_FRAMEBUFFER_COMPLETE + " " + gl.glCheckFramebufferStatus(gl.GL_FRAMEBUFFER));
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
	}
	private static void initTextureFramebuffer(GL3 gl, int width, int height){
		gl.glGenTextures(3, texs, 0);
		gl.glGenFramebuffers(2, fbos, 0);
    	resizeTexture(gl, width, height);
	}
	
	public static void initTransparency(GL3 gl, int width, int height){
		depth.init(gl);
    	transp.init(gl);
    	transpSphere.init(gl);
    	copy.init(gl);
    	copyCoords = new GLVBOFloat(gl, testQuadCoords, "vertex_coordinates");
    	copyTex = new GLVBOFloat(gl, testTexCoords, "texture_coordinates");
    	gl.glGenQueries(2, queries, 0);
    	
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	initTextureFramebuffer(gl, width, height);
	}
	
	private static int queryNumber = 0;
	private static void startQuery(GL3 gl){
		queryNumber++;
		queryNumber %= 2;
		System.out.println("query number is " + queryNumber);
		gl.glBeginQuery(gl.GL_SAMPLES_PASSED, queries[1-queryNumber]);
	}
	private static void endQuery(GL3 gl){
		gl.glEndQuery(gl.GL_SAMPLES_PASSED);
	}
	private static int waitForQueryResult(GL3 gl){
    	int counter = 0;
    	do{
    		counter++;
    		gl.glGetQueryObjectuiv(queries[queryNumber],gl.GL_QUERY_RESULT_AVAILABLE, queryresavail, queryNumber);
    		//System.out.println("not true yet: " + counter);
    	}while(queryresavail[queryNumber] != gl.GL_TRUE && counter <= 100000);
    	if(queryresavail[queryNumber] == gl.GL_TRUE){
    		gl.glGetQueryObjectuiv(queries[queryNumber] ,gl.GL_QUERY_RESULT, queryres, queryNumber);
    		return queryres[queryNumber];
    	}else{
    		System.err.println("no occlusion query result after 100000 loop iterations.\n" +
    				"Forcing query result to 0. See jogl3.helper.TransparencyHelper.java");
    		return 0;
    	}
	}
	private static int numLoops = 2;
	private static boolean veryFirstEntry = true;
	public static void render(GL3 gl, List<RenderableObject> nonTransp, List<RenderableObject> transp, int width, int height, BackgroundHelper backgroundHelper){
		
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    	gl.glViewport(0, 0, supersample*width, supersample*height);
    	gl.glClearColor(0.5f, 0.5f, 0.5f, 1);
    	gl.glClear(gl.GL_COLOR_BUFFER_BIT);
    	//draw background here
    	backgroundHelper.draw(gl);
    	SkyboxHelper.render(gl);
    	//draw nontransparent objects into framebuffer
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	gl.glClearDepth(1);
    	gl.glClear(gl.GL_DEPTH_BUFFER_BIT);
    	
    	gl.glDisable(gl.GL_BLEND);
    	for(RenderableObject o : nonTransp){
    		o.render(width, height);
    	}
    	for(RenderableObject o : transp){
    		o.render(width, height);
    	}
    	//with this loop it draws as many layers as neccessary to complete the scene
    	//you can experiment by drawing only one layer or two and then view the result (see the comment after the loop)
    	
    	
    	
    	int counter = 0;
    	boolean lastLoopVisited = false;
    	System.out.println("numLoops " + numLoops);
    	while(counter < 2*numLoops){
    		System.out.println("entering loop");
    		//when we are in the last loop, get the query result from the previous render pass
    		if(counter >= numLoops-2 && !lastLoopVisited && !veryFirstEntry){
    			int res = waitForQueryResult(gl);
    			System.out.println("query result is " + res);
    			System.out.println("numLoops " + numLoops);
    			//if there were fragments left to draw, then increase the number of loops, else decrease it
    			if(res != 0)
    				numLoops+=1;
    			else if(numLoops > 1)
    				numLoops--;
    			lastLoopVisited = true;
    		}
    		veryFirstEntry = false;
    		
    		counter+=2;
        	
    		//draw transparent objects into FBO with reverse depth values
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
        	peelDepth(gl, transp, supersample*width, supersample*height);
        	//draw on the SCREEN
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
        	addOneLayer(gl, transp, supersample*width, supersample*height);
    	}
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
    	startQuery(gl);
    	peelDepth(gl, transp, supersample*width, supersample*height);
    	endQuery(gl);
    	//draw on the SCREEN
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
    	addOneLayer(gl, transp, supersample*width, supersample*height);
//    	//draw transparent objects into FBO with reverse depth values
//    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
//    	peelDepth(gl, transp, supersample*width, supersample*height);
//    	//draw on the SCREEN
//    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
//    	addOneLayer(gl, transp, supersample*width, supersample*height);
//    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
    	//you can change the number here:
    	//0 means the current depth layer generated by peelDepth()
    	//1 the depth layer of the final image generated by addOneLayer()
    	//2 the color layer of the final image generated by addOneLayer()
    	copyFBO2FB(gl, 2, width, height);
	}
	
	private static void addOneLayer(GL3 gl, List<RenderableObject> transp, int width, int height) {
		gl.glViewport(0, 0, width, height);
    	
    	gl.glEnable(gl.GL_BLEND);
    	gl.glBlendEquation(gl.GL_FUNC_ADD);
    	gl.glBlendFunc(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA);
    	
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	
		
		for(RenderableObject o : transp){
			gl.glActiveTexture(gl.GL_TEXTURE9);
	    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[0]);
    		o.addOneLayer(width, height);
    	}
		
    	gl.glDisable(gl.GL_BLEND);
	}
	
	private static void peelDepth(GL3 gl, List<RenderableObject> transp, int width, int height) {
		gl.glViewport(0, 0, width, height);
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	
    	gl.glClearColor(0, 0, 0, 1);
    	gl.glClear(gl.GL_COLOR_BUFFER_BIT);
    	gl.glClearDepth(1);
    	gl.glClear(gl.GL_DEPTH_BUFFER_BIT);
    	
    	
		for(RenderableObject o : transp){
			gl.glActiveTexture(gl.GL_TEXTURE0);
	    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[1]);
    		o.renderDepth(width, height);
    	}
		
	}
	private static void copyFBO2FB(GL3 gl, int tex, int width, int height){
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
}
