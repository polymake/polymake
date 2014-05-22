package de.jreality.tutorial.projects.darboux;

import java.util.LinkedList;
import java.util.List;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.Quat;
import de.jreality.math.Rn;
import de.jreality.tutorial.util.polygon.PointSequence;

public class DarbouxTransform implements ChangeListener, PointSequence {
	
	
	private PointSequence initialCurve;
	private double[][] rods;
	private double[] startPoint;
	
	private double r;
	
	public DarbouxTransform(PointSequence sequence, double[] startPoint) {
		initialCurve = sequence;
		initialCurve.addChangeListener(this);
		setStartPoint(startPoint);
		update();
	}
	
	public double getR() {
		return r;
	}

	public void setR(double r) {
		this.r = r;
		update();
	}
	
	public void setStartPoint(double[] startPoint) {
		this.startPoint = startPoint;
		update();
	}
	
	private void update() {
		if (startPoint != null && initialCurve != null) {
			rods = computeRods(initialCurve.getPoints(), startRod(), getR());
			fireChange();
		}
	}

	public double[][] getRods() {
		return rods;
	}
	
	public double[][] getPoints() {
		if (rods == null) update();
		double[][] curve = initialCurve.getPoints();
		int n = curve.length;
		int m = initialCurve.isClosed() ? n+1 : n;
		double[][] ret = new double[m][];
		for (int i=0; i<m; i++) {
			ret[i] = Rn.add(null, curve[i%n], rods[i]);
		}
		return ret;
	}
	
	private double[] startRod() {
		return Rn.subtract(null, startPoint, initialCurve.getPoints()[0]);
	}

	
	private static double[][] computeRods(double[][] curve, double[] lT_0, double r) {
		int n = curve.length;
		double[][] darboux = new double[n+1][];
		darboux[0]=lT_0.clone();
		for (int i=0; i<n; i++) {
			double[] lT = darboux[i];
			double[] S = Rn.subtract(null, curve[(i+1)%n], curve[i]);
			darboux[i+1] = darbouxStep(lT, S, r);
		}
		return darboux;
	}

	private static double[] darbouxStep(double[] lT, double[] S, double r) {
		double[] lT_S = Rn.subtract(null, lT, S); // 3-vec
		double[] rlTS = Quat.toQuat(null, -r, lT_S); // quat
		double[] lT_next = Quat.conjugateBy(null, Quat.toQuat(null, 0, lT), rlTS);
		return Quat.im(lT_next);
	}
	
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

	public double[][] getCurve() {
		return initialCurve.getPoints();
	}

	public void stateChanged(ChangeEvent e) {
		update();
	}

	public boolean isClosed() {
		return false;
	}
	
}
