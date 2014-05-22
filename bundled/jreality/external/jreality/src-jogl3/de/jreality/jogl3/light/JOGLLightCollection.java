package de.jreality.jogl3.light;

import java.util.LinkedList;

public class JOGLLightCollection {
	//the matrix for transforming world to camera coordinates
	public JOGLLightCollection(double[] ViewMatrix){
		viewMatrix = ViewMatrix;
	}
	
	public double[] getViewMatrix() {
		return viewMatrix;
	}
	//is the matrix that transforms world coordinates to camera space
	private double[] viewMatrix;
	
	public LinkedList<JOGLPointLightInstance> pointLights = new LinkedList<JOGLPointLightInstance>();
	public LinkedList<JOGLSpotLightInstance> spotLights = new LinkedList<JOGLSpotLightInstance>();
	public LinkedList<JOGLDirectionalLightInstance> directionalLights = new LinkedList<JOGLDirectionalLightInstance>();
}
