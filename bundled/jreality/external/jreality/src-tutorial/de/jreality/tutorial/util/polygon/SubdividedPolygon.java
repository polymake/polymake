package de.jreality.tutorial.util.polygon;

import java.util.LinkedList;
import java.util.List;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.Rn;

/**
 * A subdivider that uses 4-point subdivision. For closed or open point sequences.
 * It will be updated whenever the control point sequence changes.
 * 
 * @author Steffen Weissmann
 */
public class SubdividedPolygon implements ChangeListener, PointSequence {

	int subdivisionSteps = 2;
	
	// the PointSequence we will subdivide...
	private PointSequence controlPoints;
	
	// the resulting points
	private double[][] pts;
	
	/**
	 * Create a SubdividedPolygon for a set of control points.
	 * @param controlPoints the control point sequence used for subdivision.
	 */
	public SubdividedPolygon(PointSequence controlPoints) {
		this.controlPoints=controlPoints;
		// we attach this class as a change listener to the control points.
		controlPoints.addChangeListener(this);
		update();
	}

	/**
	 * Set the number of subdivision steps.
	 * @param n number of steps
	 */
	public void setSubdivisionLevel(int n) {
		this.subdivisionSteps=n;
		update();
	}

	// compute the subdivided points
	private double[][] computeSpline() {
		double[][] cur = controlPoints.getPoints();
		for (int i=0; i<subdivisionSteps; i++) {
			int n = isClosed() ? cur.length : cur.length-1;
			double[][] sub = new double[cur.length+n][];
			for (int j=0; j<n; j++) {
				sub[2*j] = cur[j];
				sub[2*j+1] = subdivide(
						point(cur, j-1),
						point(cur, j),
						point(cur, j+1),
						point(cur, j+2));
			}
			if (!isClosed()) {
				sub[2*n]=cur[n];
			}
			cur = sub;
		}
		return cur;
	}
	
	private double[] point(double[][] pts, int j) {
		int n=pts.length;
		if (j>=0 && j<n) return pts[j];
		if (controlPoints.isClosed()) return pts[(j+n)%n];
		double[] p0=null, p1=null;
		if (j==-1) {
			p0 = pts[0];
			p1 = pts[1];
		}
		if (j==n) {
			p1 = pts[n-2];
			p0 = pts[n-1];
		}
		double[] ret = Rn.linearCombination(null, 2, p0, -1, p1);
		return ret;
	}

	private static double[] subdivide(double[] v1, double[] v2, double[] v3, double[] v4) {
		double[] ret = new double[3];
    	for (int j=0; j<3; j++) ret[j] = (9.0*(v2[j]+v3[j])-v1[j]-v4[j])/16.0;
    	return ret;
	}

	// recompute subdivision
	private void update() {
		pts = computeSpline();
		fireChange();
	}
	
	/**
	 * returns the subdivided point sequence.
	 */
	public double[][] getPoints() {
		return pts;
	}

	/**
	 * this is called from the control point sequence.
	 */
	public void stateChanged(ChangeEvent e) {
		update();
	}

	public boolean isClosed() {
		return controlPoints.isClosed();
	}
	
	/******** listener code ********/
	
	private List<ChangeListener> listeners = new LinkedList<ChangeListener>();
	
	private void fireChange() {
		final ChangeEvent ev = new ChangeEvent(this);
		synchronized (listeners ) {
			for (ChangeListener cl : listeners) cl.stateChanged(ev);
		}
	}

	public void addChangeListener(ChangeListener cl) {
		synchronized (listeners) {
			listeners.add(cl);
		}
	}
	
	public void removeChangeListener(ChangeListener cl) {
		synchronized (listeners) {
			listeners.remove(cl);
		}
	}
	
}
