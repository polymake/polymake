package de.jreality.renderman.shader;

import java.util.HashMap;
import java.util.Map;

import de.jreality.renderman.RIBVisitor;
import de.jreality.shader.EffectiveAppearance;

/**
 * This class is used as shader for renderman sky box (@see RendermanSkyBox).
 */ 
public class ConstantTexture implements RendermanShader {
	Map att = new HashMap();
	
	public ConstantTexture()	{
		super();
	}
	public void setFromEffectiveAppearance(RIBVisitor ribv,
		EffectiveAppearance ap, String name) {
	}

	public Map getAttributes() {
		return att;
	}

	public String getType() {
		return "Surface";
	}

	public String getName() {
		return "constantTexture";
	}

}
