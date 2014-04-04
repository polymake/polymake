package de.jreality.sunflow.core.shader;

import org.sunflow.SunflowAPI;
import org.sunflow.core.ParameterList;
import org.sunflow.core.Ray;
import org.sunflow.core.Shader;
import org.sunflow.core.ShadingState;
import org.sunflow.image.Color;
import org.sunflow.math.OrthoNormalBasis;
import org.sunflow.math.Point2;
import org.sunflow.math.Vector3;

import de.jreality.backends.texture.SimpleTexture;
import de.jreality.shader.CubeMap;
import de.jreality.shader.RenderingHintsShader;

public class DefaultPolygonShader implements Shader {

	private de.jreality.shader.DefaultPolygonShader dps;
	private SimpleTexture tex;

	private RenderingHintsShader rhs;
	private CubeMap cm;
	
	public DefaultPolygonShader(
			de.jreality.shader.DefaultPolygonShader dps,
			RenderingHintsShader rhs,
			boolean hasTextureCoordinates
	) {
		this.dps=dps;
		if (dps.getTexture2d() != null && hasTextureCoordinates) {
			tex = new SimpleTexture(dps.getTexture2d());
		}
		this.rhs=rhs;
		this.cm = dps.getReflectionMap();
	}
	
	public Color getRadiance(ShadingState state) {
        // make sure we are on the right side of the material
        state.faceforward();
        // setup lighting
        state.initLightSamples();
        state.initCausticSamples();
        
        double[] diff = getDiffuse(state);
        
        if (diff[3] == 0) {
        	return state.traceRefraction(new Ray(state.getPoint(), state.getRay().getDirection()), 0); 
        	//return state.traceTransparency();
        }
        
        Color d = new Color((float)diff[0], (float)diff[1], (float)diff[2]).toLinear();
        
        if (!rhs.getLightingEnabled()) return d;

        Color ret = state.diffuse(d);
        
        if (state.includeSpecular()) {
        	Color spec = state.specularPhong(convert(dps.getSpecularColor(), dps.getSpecularCoefficient()), dps.getSpecularExponent().floatValue(), 0);
        	ret.add(spec);
        }
        
        if (cm != null) {
            float cos = state.getCosND();
            float dn = 2 * cos;
            Vector3 refDir = new Vector3();
            refDir.x = (dn * state.getNormal().x) + state.getRay().getDirection().x;
            refDir.y = (dn * state.getNormal().y) + state.getRay().getDirection().y;
            refDir.z = (dn * state.getNormal().z) + state.getRay().getDirection().z;
            Ray refRay = new Ray(state.getPoint(), refDir);
            
//            // compute Fresnel term
//            cos = 1 - cos;
//            float cos2 = cos * cos;
//            float cos5 = cos2 * cos2 * cos;
//
//            Color ret = Color.white();
//            Color r = d.copy().mul(convert(dps.getSpecularColor(), dps.getSpecularCoefficient()));
//            ret.sub(r);
//            ret.mul(cos5);
//            ret.add(r);

            
        	Color ref = state.traceReflection(refRay, 0);
        	float l = cm.getBlendColor().getAlpha()/255f;
        	ret.mul(1-l).madd(l, ref);
        }             
        if (diff[3] != 1) {
        	float t=(float) diff[3];
        	Color refl = state.traceRefraction(new Ray(state.getPoint(), state.getRay().getDirection()), 0); 
        	//Color refl = state.traceTransparency();
        	ret.mul(t).madd(1-t, refl);
        }
        return ret;
	}

	private double[] getDiffuse(ShadingState state) {
		double[] color=new double[4];
		getColor(color, dps.getDiffuseColor(), dps.getDiffuseCoefficient());
		if (rhs.getTransparencyEnabled()) {
			color[3] = 1-dps.getTransparency();
		}
		else if (dps.getTransparency() == 1) color[3] = 0;
		if (tex != null) {
			Point2 uv = state.getUV();
			tex.getColor(uv.x, uv.y, 0, 0, 0, 0, 0, color);
		}
		if (!rhs.getTransparencyEnabled() && color[3] > 0) {
			color[3]=1;
		}
		return color;
	}

	private void getColor(double[] d, java.awt.Color c, double f) {
		d[0] = c.getRed()*f/255;
		d[1] = c.getGreen()*f/255;
		d[2] = c.getBlue()*f/255;
		d[3] = c.getAlpha()/255;
	}

	private Color convert(java.awt.Color c, double f) {
		float ff = (float) (f/255);
		return new Color(c.getRed()*ff, c.getGreen()*ff, c.getBlue()*ff);
	}

	public void scatterPhoton(ShadingState state, Color power) {
		Color diffuse;
		float refl = cm != null ? (float)cm.getBlendColor().getAlpha()/255f : 0;
        // make sure we are on the right side of the material
        state.faceforward();
        double[] diff = getDiffuse(state);
        diffuse = new Color((float)diff[0],(float)diff[1],(float)diff[2]);
        //System.out.println("storing photon");
        state.storePhoton(state.getRay().getDirection(), power, diffuse);
        float d = diffuse.getAverage();
        float r = d * refl;
        double rnd = state.getRandom(0, 0, 1);
        if (rnd < d) {
            // photon is scattered
            power.mul(diffuse).mul(1.0f / d);
            OrthoNormalBasis onb = state.getBasis();
            double u = 2 * Math.PI * rnd / d;
            double v = state.getRandom(0, 1, 1);
            float s = (float) Math.sqrt(v);
            float s1 = (float) Math.sqrt(1.0 - v);
            Vector3 w = new Vector3((float) Math.cos(u) * s, (float) Math.sin(u) * s, s1);
            w = onb.transform(w, new Vector3());
            state.traceDiffusePhoton(new Ray(state.getPoint(), w), power);
        } else if (rnd < d + r) {
            float cos = -Vector3.dot(state.getNormal(), state.getRay().getDirection());
            power.mul(diffuse).mul(1.0f / d);
            // photon is reflected
            float dn = 2 * cos;
            Vector3 dir = new Vector3();
            dir.x = (dn * state.getNormal().x) + state.getRay().getDirection().x;
            dir.y = (dn * state.getNormal().y) + state.getRay().getDirection().y;
            dir.z = (dn * state.getNormal().z) + state.getRay().getDirection().z;
            state.traceReflectionPhoton(new Ray(state.getPoint(), dir), power);
        }
	}

	public boolean update(ParameterList pl, SunflowAPI api) {
		return true;
	}

}
