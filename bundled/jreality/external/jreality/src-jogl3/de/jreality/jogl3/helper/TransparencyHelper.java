package de.jreality.jogl3.helper;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.List;

import javax.media.opengl.GL3;

//import com.jogamp.opengl.util.awt.ImageUtil;

import de.jreality.jogl.plugin.InfoOverlay;
import de.jreality.jogl3.InfoOverlayData;
import de.jreality.jogl3.JOGLSceneGraphComponentInstance.RenderableObject;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.optimization.RenderableUnitCollection;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.LabelShader;
import de.jreality.util.ImageUtility;

public class TransparencyHelper {
	public static void setSupersample(int ss){
		TransparencyHelper.supersample = ss;
	}
	public static int getSupersample(){
		return TransparencyHelper.supersample;
	}
	private static int supersample = 2;
	
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
	private static int[] queries = new int[1];
	
	private static int[] texs = new int[3];
	
	private static int[] fbos = new int[2];
    
    private static int[] queryresavail = new int[1];
    private static int[] queryres = new int[1];
	
	public static void resizeFramebufferTextures(GL3 gl, int width, int height){
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
    	//System.out.println(" " + gl.GL_FRAMEBUFFER_COMPLETE + " " + gl.glCheckFramebufferStatus(gl.GL_FRAMEBUFFER));
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
	}
	private static void initTextureFramebuffer(GL3 gl, int width, int height){
		gl.glGenTextures(3, texs, 0);
		gl.glGenFramebuffers(2, fbos, 0);
    	resizeFramebufferTextures(gl, width, height);
	}
	
	public static void initTransparency(GL3 gl, int width, int height){
		depth.init(gl);
    	transp.init(gl);
    	transpSphere.init(gl);
    	copy.init(gl);
    	copyCoords = new GLVBOFloat(gl, testQuadCoords, "vertex_coordinates");
    	copyTex = new GLVBOFloat(gl, testTexCoords, "texture_coordinates");
    	gl.glGenQueries(1, queries, 0);
    	
    	gl.glEnable(gl.GL_DEPTH_TEST);
    	initTextureFramebuffer(gl, width, height);
	}
	
	private static void startQuery(GL3 gl){
		gl.glBeginQuery(gl.GL_SAMPLES_PASSED, queries[0]);
	}
	private static int endQuery(GL3 gl){
		gl.glEndQuery(gl.GL_SAMPLES_PASSED);
    	int counter = 0;
    	do{
    		counter++;
    		gl.glGetQueryObjectuiv(queries[0],gl.GL_QUERY_RESULT_AVAILABLE, queryresavail, 0);
    		//System.out.println("not true yet: " + counter);
    		if(counter == 1000000000)
    			System.out.println("reached max no of iterations in query loop");
    	}while(queryresavail[0] != gl.GL_TRUE && counter < 1000000000);
    	if(queryresavail[0] == gl.GL_TRUE){
    		gl.glGetQueryObjectuiv(queries[0] ,gl.GL_QUERY_RESULT, queryres, 0);
//    		System.out.println("query result after " + counter + " waits is " + queryres[0]);
    		return queryres[0];
    		//System.out.println("Query result is " + queryres[0]);
    	}else{
    		System.err.println("Oups, we waited for the query result for longer than a billion iterations! Returning 0, to make partial render possible");
    		return 0;
    	}
	}
	
	public static void render(InfoOverlayData infoData, GL3 gl, RenderableUnitCollection ruc, List<RenderableObject> transp, int width, int height, BackgroundHelper backgroundHelper){
		
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
    	
    	//TODO is this correct??
    	gl.glDisable(gl.GL_BLEND);
    	ruc.render(gl, width, height);
    	
    	if(transp.size() != 0){
    		for(RenderableObject o : transp){
        		o.render(width, height);
        	}
        	
        	//TODO TODO TODO
        	//if transp.size == 0 then don't do anything of this!
        	//Except for drawing labels nicely on top of each other with correct AA.
        	
        	int quer = 1;
        	//with this loop it draws as many layers as neccessary to complete the scene
        	//you can experiment by drawing only one layer or two and then view the result (see the comment after the loop)
        	
        	//draw transparent objects into FBO with reverse depth values
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
        	startQuery(gl);
        	peelDepth(gl, transp, supersample*width, supersample*height);
        	quer = endQuery(gl);
        	
        	int counter = 0;
        	while(quer!=0 && counter < 20){
        		counter++;
            	//draw on the SCREEN
            	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
            	addOneLayer(gl, transp, supersample*width, supersample*height);
            	//draw transparent objects into FBO with reverse depth values
            	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
            	startQuery(gl);
            	peelDepth(gl, transp, supersample*width, supersample*height);
            	quer = endQuery(gl);
        	}
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
        	gl.glDisable(gl.GL_DEPTH_TEST);
    	}
//    	
    	if(infoData.activated)
    		LabelShader.renderOverlay("Framerate = " + infoData.framerate + "\nClockrate = " + infoData.clockrate + "\nPolygonCount = " + infoData.polygoncount + "\n" + InfoOverlay.getMemoryUsage(), gl);
    	
    	
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
		gl.glDisable(gl.GL_BLEND);
		gl.glViewport(0, 0, width, height);
    	
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	gl.glActiveTexture(gl.GL_TEXTURE0);
    	gl.glBindTexture(gl.GL_TEXTURE_2D, texs[tex]);
    	
    	
    	gl.glDisable(gl.GL_DEPTH_TEST);
    	
    	copy.useShader(gl);
    	
    	gl.glUniform1i(gl.glGetUniformLocation(copy.shaderprogram, "image"), 0);
    	
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
	
	public static BufferedImage renderOffscreen(double AA, BufferedImage dst, GL3 gl, RenderableUnitCollection ruc, List<RenderableObject> transp, int width, int height, BackgroundHelper backgroundHelper){
		System.out.println("supersample is " + supersample);
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
    	ruc.render(gl, width, height);
    	for(RenderableObject o : transp){
    		o.render(width, height);
    	}
    	int quer = 1;
    	//with this loop it draws as many layers as neccessary to complete the scene
    	//you can experiment by drawing only one layer or two and then view the result (see the comment after the loop)
    	
    	//draw transparent objects into FBO with reverse depth values
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
    	startQuery(gl);
    	peelDepth(gl, transp, supersample*width, supersample*height);
    	quer = endQuery(gl);
    	
    	int counter = 0;
    	while(quer!=0 && counter < 20){
    		counter++;
        	//draw on the SCREEN
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[1]);
        	addOneLayer(gl, transp, supersample*width, supersample*height);
        	//draw transparent objects into FBO with reverse depth values
        	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbos[0]);
        	startQuery(gl);
        	peelDepth(gl, transp, supersample*width, supersample*height);
        	quer = endQuery(gl);
    	}
    	
    	gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0);
    	//you can change the number here:
    	//0 means the current depth layer generated by peelDepth()
    	//1 the depth layer of the final image generated by addOneLayer()
    	//2 the color layer of the final image generated by addOneLayer()
    	copyFBO2FB(gl, 2, width, height);
    	
    	System.out.println("before dst=new...");
    	dst = new BufferedImage(width, height,
				BufferedImage.TYPE_4BYTE_ABGR); // TYPE_3BYTE_BGR); //
    	
    	Buffer buffer = ByteBuffer.wrap(((DataBufferByte) dst.getRaster()
				.getDataBuffer()).getData());
		
    	System.out.println("before ReadPixels");
    	int err = gl.glGetError();
    	System.out.println(err);
    	gl.glReadPixels(0, 0, width, height, GL3.GL_RGBA,
				GL3.GL_UNSIGNED_BYTE, buffer);
    	err = gl.glGetError();
    	System.out.println("ss" + supersample);
    	System.out.println(err);
		System.err.println("reading pixels");
		
		dst = ImageUtility.rearrangeChannels(null, dst);
		//TODO revert here
//		ImageUtil.flipImageVertically(dst);
		return dst;
	}
}
