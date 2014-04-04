package de.jreality.jogl3.light;

import de.jreality.scene.SceneGraphNode;

public class JOGLPointLightInstance extends JOGLLightInstance {

	public JOGLPointLightInstance(SceneGraphNode node) {
		super(node);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void addToList(JOGLLightCollection parentCollection) {
		parentCollection.pointLights.add(this);
	}

}
