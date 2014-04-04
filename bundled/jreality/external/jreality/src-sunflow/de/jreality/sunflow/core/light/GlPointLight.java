package de.jreality.sunflow.core.light;

import org.sunflow.SunflowAPI;
import org.sunflow.core.LightSample;
import org.sunflow.core.LightSource;
import org.sunflow.core.ParameterList;
import org.sunflow.core.Ray;
import org.sunflow.core.ShadingState;
import org.sunflow.image.Color;
import org.sunflow.math.Point3;
import org.sunflow.math.Vector3;

public class GlPointLight implements LightSource {
    private Point3 lightPoint;
    private Color power;
    private float fallOffA0;
    private float fallOffA1;
    private float fallOffA2;

    public GlPointLight() {
        lightPoint = new Point3(0, 0, 0);
        power = Color.WHITE;
        fallOffA0 = 0;
        fallOffA1 = 0;
        fallOffA2 = 1;
    }

    public boolean update(ParameterList pl, SunflowAPI api) {
        lightPoint = pl.getPoint("center", lightPoint);
        power = pl.getColor("power", power);
        fallOffA0 = pl.getFloat("fallOffA0", 0);
        fallOffA1 = pl.getFloat("fallOffA1", 0);
        fallOffA2 = pl.getFloat("fallOffA2", 1);
        return true;
    }

    public int getNumSamples() {
        return 1;
    }

	public int getLowSamples() {
		return 1;
	}

    public boolean isVisible(ShadingState state) {
        Point3 p = state.getPoint();
        Vector3 n = state.getNormal();
        return (Vector3.dot(Point3.sub(lightPoint, p, new Vector3()), n) > 0.0);
    }

    public void getSamples(ShadingState state) {
    	LightSample dest = new LightSample();
        dest.setShadowRay(new Ray(state.getPoint(), lightPoint));
        float dist = lightPoint.distanceTo(state.getPoint());
        float fallOff = fallOffA0 + dist * fallOffA1 + dist * dist * fallOffA2;
        float scale = 1.0f / (float) (4 * Math.PI * fallOff);
        dest.setRadiance(power, power);
        dest.getDiffuseRadiance().mul(scale);
        dest.getSpecularRadiance().mul(scale);
        dest.traceShadow(state);
        state.addSample(dest);
    }

    public void getPhoton(double randX1, double randY1, double randX2, double randY2, Point3 p, Vector3 dir, Color power) {
        p.set(lightPoint);
        float phi = (float) (2 * Math.PI * randX1);
        float s = (float) Math.sqrt(randY1 * (1.0f - randY1));
        dir.x = (float) Math.cos(phi) * s;
        dir.y = (float) Math.sin(phi) * s;
        dir.z = (float) (1 - 2 * randY1);
        power.set(this.power);
    }

    public boolean isAdaptive() {
        return false;
    }

    public float getPower() {
        return power.getLuminance();
    }
}