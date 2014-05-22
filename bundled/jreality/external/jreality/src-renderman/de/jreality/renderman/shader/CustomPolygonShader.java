package de.jreality.renderman.shader;

import de.jreality.renderman.RIBVisitor;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;

public class CustomPolygonShader extends AbstractRendermanShader {
	SLShader orig = new SLShader("orig");
	@Override
	public void setFromEffectiveAppearance(RIBVisitor ribv,
			EffectiveAppearance eap, String name) {
       Object foo = eap.getAttribute(name+"."+CommonAttributes.RMAN_SURFACE_SHADER,orig);
       if (foo == Appearance.DEFAULT || !(foo instanceof SLShader) || foo == orig)
    	   throw new IllegalStateException("CustomPolygonShader called without SLShader");
       SLShader sls = (SLShader) foo;
       map = sls.getParameters();
       shaderName = sls.getName();
       type = "Surface";
	}


}
