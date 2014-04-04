package de.jreality.jogl3.light;

import de.jreality.scene.SceneGraphNode;

public class JOGLSpotLightInstance extends JOGLPointLightInstance {

	public JOGLSpotLightInstance(SceneGraphNode node) {
		super(node);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	public void addToList(JOGLLightCollection parentCollection) {
		parentCollection.spotLights.add(this);
	}
	
}
