package de.jreality.jogl3.light;

import java.awt.Color;

import de.jreality.jogl3.JOGLSceneGraph;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.event.LightEvent;
import de.jreality.scene.event.LightListener;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;

public abstract class JOGLLightEntity extends SceneGraphNodeEntity implements LightListener {

	JOGLSceneGraph sg;
	
	protected JOGLLightEntity(SceneGraphNode node, JOGLSceneGraph sg) {
		super(node);
		this.sg = sg;
		sg.lightsChanged = true;
		// TODO Auto-generated constructor stub
	}
	protected Color color;
	boolean dataUpToDate = false;
	boolean global = true;
	protected double intensity;
	
	public double getIntensity(){
		return intensity;
	}
	
	public float[] getColor(){
		return color.getComponents(new float[]{0, 0, 0, 0});
	}
	
	public void lightChanged(LightEvent ev) {
		System.out.println("JOGLPointSetEntity.lightChanged()");
		dataUpToDate = false;
		sg.lightsChanged = true;
	}
	
	public abstract void updateData();
	
	public boolean isGlobal() {
		// TODO Auto-generated method stub
		return global;
	}
}
