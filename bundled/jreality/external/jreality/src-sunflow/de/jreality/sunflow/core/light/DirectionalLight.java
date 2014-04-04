package de.jreality.sunflow.core.light;

import org.sunflow.SunflowAPI;
import org.sunflow.core.LightSample;
import org.sunflow.core.LightSource;
import org.sunflow.core.ParameterList;
import org.sunflow.core.Ray;
import org.sunflow.core.ShadingState;
import org.sunflow.image.Color;
import org.sunflow.math.OrthoNormalBasis;
import org.sunflow.math.Point3;
import org.sunflow.math.Vector3;

public class DirectionalLight implements LightSource {
	private Vector3 dir;
	private Point3 sceneCenter;
	private Color power;
	private Color photonPower = new Color();
	
	private double distortion = 0.000089f;
	private int samples = 8;
	private float sceneRadius = 320;

    OrthoNormalBasis onb;

	public DirectionalLight() {
		sceneCenter = new Point3(0,0,0);
		dir = new Vector3(0, 1, 0);
		power = Color.WHITE;
		onb = OrthoNormalBasis.makeFromW(dir);
	}

	public boolean update(ParameterList pl, SunflowAPI api) {
		dir = pl.getVector("dir", dir);
		samples = pl.getInt("samples", 8);
		float radius = pl.getFloat("radius", (float)(.542*Math.PI/180));
		distortion = 1-Math.cos(radius);
		//System.out.println("distortion "+distortion);
		dir.normalize();
		onb = OrthoNormalBasis.makeFromW(dir);
		power = pl.getColor("power", power);
		Color.mul(200, power,photonPower);
		return true;
	}

	public int getNumSamples() {
		return samples;
	}

	public boolean isVisible(ShadingState state) {
		Vector3 n = state.getNormal();
		return (Vector3.dot(dir, n) > 0.0);
	}

	public void getSamples(ShadingState state) {
		Vector3 lightDir=dir;
		int samples = state.getDiffuseDepth() > 0 ? 1 : getNumSamples();
		for (int i = 0; i < samples; i++) {
		if (distortion != 0) {
	        double h = 1-state.getRandom(i, 0, samples)*distortion;
	        double theta = 2*state.getRandom(i, 1, samples)*Math.PI;
	        double us=Math.sqrt(1-h*h);
	        
	        float l1 = (float)(us*Math.cos(theta));
	        float l2 = (float)(us*Math.sin(theta));
	        float l3 = (float)h;
	
	        lightDir = new Vector3(l1, l2, l3);
	                
	        onb.transform(lightDir);
		}
		LightSample dest = new LightSample();
		dest.setShadowRay(new Ray(state.getPoint(), lightDir));
        dest.getShadowRay().setMax(Float.MAX_VALUE);
		dest.setRadiance(power, power);
		dest.traceShadow(state);
		}
	}

	public void getPhoton(double randX1, double randY1, double randX2, double randY2, Point3 p, Vector3 dir, Color power) {
		Vector3 lightDir=dir;
		if (distortion != 0) {
	        double h = 1-randX1*distortion;
	        double theta = 2*randY1*Math.PI;
	        double us=Math.sqrt(1-h*h);
	        
	        float l1 = (float)(us*Math.cos(theta));
	        float l2 = (float)(us*Math.sin(theta));
	        float l3 = -(float)h;
	
	        lightDir = new Vector3(l1, l2, l3);
	        onb.transform(lightDir);
		}
		dir.set(lightDir);
		
		double phi = randX2 * 2 * Math.PI;
		double r = sceneRadius * Math.sqrt(randY2);
		float p1 = (float) (r * Math.cos(phi));
		float p2 = (float) (r * Math.sin(phi));
		Vector3 position = new Vector3(p1, p2, sceneRadius);
		onb.transform(position);
		Point3.add(sceneCenter, position, p);
		
		power.set(photonPower);
		//System.out.println("photon from "+position+" in direction "+lightDir);
	}

	public boolean isAdaptive() {
		return false;
	}

	public float getPower() {
		return power.getLuminance();
	}

	public int getLowSamples() {
		return 1;
	}
}
