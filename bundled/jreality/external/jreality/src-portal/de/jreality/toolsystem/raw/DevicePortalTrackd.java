package de.jreality.toolsystem.raw;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Map;

import javax.swing.ButtonGroup;
import javax.swing.JFrame;
import javax.swing.JRadioButton;

import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;

public class DevicePortalTrackd extends DeviceTrackd {
	
	private int headSensorID=1;
	
	public DevicePortalTrackd() {
		JFrame f = new JFrame("Head tracking:");
		ButtonGroup bg = new ButtonGroup();
		JRadioButton b1 = new JRadioButton("enabled");
		b1.setSelected(true);
		b1.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				freeHead();
			}
		});
		JRadioButton b2 = new JRadioButton("disabled");
		b2.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				fixHead();
			}
		});
		bg.add(b1);
		bg.add(b2);
		f.getContentPane().setLayout(new GridLayout(1, 2));
		f.getContentPane().add(b1);
		f.getContentPane().add(b2);
		f.pack();
		f.setVisible(true);
	}
	
	protected void fixHead() {
		disableSensor(headSensorID);
	}

	protected void freeHead() {
		enableSensor(headSensorID);
	}

	@Override
	protected void calibrate(double[] sensorMatrix, int index) {
		
		Matrix m = new Matrix(sensorMatrix);

		double x = m.getEntry(0, 3);
		double y = m.getEntry(1, 3);
		double z = m.getEntry(2, 3);
		m.setEntry(0, 3, x*0.254);
		m.setEntry(1, 3, y*0.254+0.25);
		m.setEntry(2, 3, z*0.254);
	}
	
	@Override
	public void initialize(Viewer viewer, Map<String, Object> config) {
		super.initialize(viewer, config);
		if (config.containsKey("head_sensor")) {
			Object hid = config.get("head_sensor");
			if (hid instanceof Integer) {
				headSensorID = (Integer) hid;
			}
		}
	}
	
	@Override
	public String getName() {
		return "PORTAL: "+super.getName();
	}
}
