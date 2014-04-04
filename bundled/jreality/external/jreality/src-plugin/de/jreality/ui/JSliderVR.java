package de.jreality.ui;

import java.awt.HeadlessException;
import java.awt.Point;

import javax.swing.BoundedRangeModel;
import javax.swing.JSlider;

public class JSliderVR extends JSlider {

	private static final long 
		serialVersionUID = 1L;
	
	public JSliderVR() {
		super();
	}

	public JSliderVR(BoundedRangeModel brm) {
		super(brm);
	}

	public JSliderVR(int orientation, int min, int max, int value) {
		super(orientation, min, max, value);
	}

	public JSliderVR(int min, int max, int value) {
		super(min, max, value);
	}

	public JSliderVR(int min, int max) {
		super(min, max);
	}

	public JSliderVR(int orientation) {
		super(orientation);
	}

	@Override
	public Point getMousePosition() throws HeadlessException {
		return new Point();
	}
	
}
