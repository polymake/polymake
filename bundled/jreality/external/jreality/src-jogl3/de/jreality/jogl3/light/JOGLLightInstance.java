package de.jreality.jogl3.light;

import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.proxy.tree.SceneTreeNode;

public abstract class JOGLLightInstance extends SceneTreeNode{
	
	public double[] trafo;
	
	protected JOGLLightInstance(SceneGraphNode node) {
		super(node);
	}
	
	public abstract void addToList(JOGLLightCollection parentCollection);

}
