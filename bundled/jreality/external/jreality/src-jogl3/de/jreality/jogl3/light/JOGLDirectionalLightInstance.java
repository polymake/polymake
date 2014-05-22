package de.jreality.jogl3.light;

import de.jreality.scene.SceneGraphNode;

public class JOGLDirectionalLightInstance extends JOGLLightInstance {

	public JOGLDirectionalLightInstance(SceneGraphNode node) {
		super(node);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void addToList(JOGLLightCollection parentCollection) {
		parentCollection.directionalLights.add(this);
	}

}
