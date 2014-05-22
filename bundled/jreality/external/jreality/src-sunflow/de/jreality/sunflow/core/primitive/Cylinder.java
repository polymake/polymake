package de.jreality.sunflow.core.primitive;

import org.sunflow.SunflowAPI;
import org.sunflow.core.Instance;
import org.sunflow.core.IntersectionState;
import org.sunflow.core.ParameterList;
import org.sunflow.core.PrimitiveList;
import org.sunflow.core.Ray;
import org.sunflow.core.ShadingState;
import org.sunflow.math.BoundingBox;
import org.sunflow.math.Matrix4;
import org.sunflow.math.OrthoNormalBasis;
import org.sunflow.math.Point3;
import org.sunflow.math.Solvers;
import org.sunflow.math.Vector3;

public class Cylinder implements PrimitiveList {
    public boolean update(ParameterList pl, SunflowAPI api) {
        return true;
    }

    public BoundingBox getWorldBounds(Matrix4 o2w) {
        BoundingBox bounds = new BoundingBox(1);
        if (o2w != null)
            bounds = o2w.transform(bounds);
        return bounds;
    }

    public float getPrimitiveBound(int primID, int i) {
        return (i & 1) == 0 ? -1 : 1;
    }

    public int getNumPrimitives() {
        return 1;
    }

    public void prepareShadingState(ShadingState state) {
        state.init();
        state.getRay().getPoint(state.getPoint());
        Instance parent = state.getInstance();
        Point3 localPoint = parent.transformWorldToObject(state.getPoint());
        state.getNormal().set(localPoint.x, localPoint.y, 0);
        state.getNormal().normalize();

        float phi = (float) Math.atan2(state.getNormal().y, state.getNormal().x);
        if (phi < 0)
            phi += 2 * Math.PI;
        float theta = (float) Math.acos(state.getNormal().z);
        state.getUV().y = theta / (float) Math.PI;
        state.getUV().x = state.getPoint().z;
        Vector3 v = new Vector3();
        v.x = -2 * (float) Math.PI * state.getNormal().y;
        v.y = 2 * (float) Math.PI * state.getNormal().x;
        v.z = 0;
        state.setShader(parent.getShader(0));
        // into world space
        Vector3 worldNormal = parent.transformNormalObjectToWorld(state.getNormal());
        v = parent.transformVectorObjectToWorld(v);
        state.getNormal().set(worldNormal);
        state.getNormal().normalize();
        state.getGeoNormal().set(state.getNormal());
        // compute basis in world space
        state.setBasis(OrthoNormalBasis.makeFromWV(state.getNormal(), v));

    }

    public void intersectPrimitive(Ray r, int primID, IntersectionState state) {
        // intersect in local space
        float qa = r.dx * r.dx + r.dy * r.dy;
        float qb = 2 * ((r.dx * r.ox) + (r.dy * r.oy));
        float qc = ((r.ox * r.ox) + (r.oy * r.oy)) - 1;
        double[] t = Solvers.solveQuadric(qa, qb, qc);
        if (t != null) {
            double z=r.oz+t[0]*r.dz;
            if (z<=1 && z>=-1 && t[0] > r.getMin() && t[0] < r.getMax())
                r.setMax((float) t[0]);
            else {
                z=r.oz+t[1]*r.dz;
                if (z<=1 && z>=-1 && t[1] > r.getMin() && t[1] < r.getMax()) r.setMax((float) t[1]);
                else return;
            }
            state.setIntersection(0, 0, 0);
        }
    }

	public PrimitiveList getBakingPrimitives() {
		return null;
	}
}