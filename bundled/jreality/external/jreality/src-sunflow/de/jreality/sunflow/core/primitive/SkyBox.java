package de.jreality.sunflow.core.primitive;

import org.sunflow.SunflowAPI;
import org.sunflow.core.IntersectionState;
import org.sunflow.core.ParameterList;
import org.sunflow.core.PrimitiveList;
import org.sunflow.core.Ray;
import org.sunflow.core.Shader;
import org.sunflow.core.ShadingState;
import org.sunflow.image.Color;
import org.sunflow.math.BoundingBox;
import org.sunflow.math.Matrix4;
import org.sunflow.math.OrthoNormalBasis;
import org.sunflow.math.Vector3;

import de.jreality.backends.texture.EnvironmentTexture;
import de.jreality.shader.CubeMap;

public class SkyBox implements PrimitiveList, Shader {
	
	private OrthoNormalBasis basis;
	private EnvironmentTexture envTex;
	
	public SkyBox(CubeMap cm) {
		updateBasis(new Vector3(0, 0, -1), new Vector3(0, 1, 0));
		envTex = new EnvironmentTexture(cm, null);
	}
	
	public int getNumPrimitives() {
		return 1;
	}

	public float getPrimitiveBound(int primID, int i) {
		return 0;
	}

	public BoundingBox getWorldBounds(Matrix4 o2w) {
		return null;
	}

	public void intersectPrimitive(Ray r, int primID, IntersectionState state) {
		if (r.getMax() == Float.POSITIVE_INFINITY)
            state.setIntersection(0, 0, 0);
	}

	public void prepareShadingState(ShadingState state) {
		if (state.includeLights())
            state.setShader(this);
	}

	public boolean update(ParameterList pl, SunflowAPI api) {
		updateBasis(pl.getVector("center", null), pl.getVector("up", null));
		return true;
	}
	
	public void init(String name, SunflowAPI api) {
		// register this object with the api properly
		api.geometry(name, this);
		api.shader(name + ".shader", this);
		api.parameter("shaders", name + ".shader");
		api.instance(name + ".instance", name);
	}
	public Color getRadiance(ShadingState state) {
		// lookup texture based on ray direction
        return state.includeLights() ? getColor(basis.untransform(state.getRay().getDirection(), new Vector3())) : Color.BLACK;
		//return getColor(basis.untransform(state.getRay().getDirection()));
	}

	public void scatterPhoton(ShadingState state, Color power) {
	}
	
	private Color getColor(Vector3 dir) {
		final double[] color = new double[4];
		envTex.getColor(0, 0, dir.x, dir.y, dir.z, 0, 0, color);
		return new Color((float)(color[0]/255), (float)(color[1]/255), (float)(color[2]/255)).toLinear().mul(3.14f);
	}
	
    private void updateBasis(Vector3 center, Vector3 up) {
        if (center != null && up != null) {
            basis = OrthoNormalBasis.makeFromWV(center, up);
            basis.swapWU();
            basis.flipV();
        }
    }

	public PrimitiveList getBakingPrimitives() {
		return null;
	}
}
