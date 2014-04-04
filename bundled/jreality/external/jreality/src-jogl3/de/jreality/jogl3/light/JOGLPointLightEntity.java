package de.jreality.jogl3.light;

import de.jreality.jogl3.JOGLSceneGraph;
import de.jreality.scene.PointLight;

public class JOGLPointLightEntity extends JOGLLightEntity {
	
	public boolean isShadowmap;
	public String shadowMap;
	public float A0, A1, A2;
	
	public JOGLPointLightEntity(PointLight node, JOGLSceneGraph sg) {
		super(node, sg);
	}

	@Override
	public void updateData() {
		if (!dataUpToDate) {
			PointLight l = (PointLight) getNode();
			color = l.getColor();
			intensity = l.getIntensity();
			shadowMap = l.getShadowMap();
			isShadowmap = l.isUseShadowMap();
			global = l.isGlobal();
			A0 = (float)l.getFalloffA0();
			A1 = (float)l.getFalloffA1();
			A2 = (float)l.getFalloffA2();
			
			dataUpToDate = true;
		}
	}
}
