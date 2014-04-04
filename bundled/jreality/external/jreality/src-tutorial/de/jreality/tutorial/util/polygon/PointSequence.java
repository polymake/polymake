package de.jreality.tutorial.util.polygon;

import javax.swing.event.ChangeListener;

public interface PointSequence {
	double[][] getPoints();
	boolean isClosed();
	void addChangeListener(ChangeListener cl);
}
