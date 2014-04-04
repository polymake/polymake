package de.jreality.toolsystem.raw;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTable;

import de.jreality.devicedriver.TrackdJNI;

public class Trackd implements ActionListener, Runnable {

	private final static long TIMESTEP = 1;

	private Thread anim;

	private static TrackdJNI trackdJNI = null;

	int numSensors;
	
	int sensor;

	private JRadioButton[] rbSensors = null;

	private JTable jtSensors = null;

	
	private int numButtons;
	private JRadioButton[] rbButtons = null;
	
	private int numValuators;
	private JTable jtValuators = null;

	private void init() {

		try {
			trackdJNI = new TrackdJNI(4126, 4127);
		} catch (IOException e) {
			System.out.println(e);
			System.exit(1);
		}
		numSensors = trackdJNI.getNumSensors();

		JFrame.setDefaultLookAndFeelDecorated(true);
		JFrame frame = new JFrame("Trackd");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		JPanel panel = new JPanel();
		panel.setLayout(new BoxLayout(panel, BoxLayout.PAGE_AXIS));
		panel.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
		frame.getContentPane().add(panel);

		JPanel pSensors = new JPanel(new GridLayout(1, 0));
		pSensors.setBorder(BorderFactory.createCompoundBorder(BorderFactory
				.createTitledBorder("Sensor"), BorderFactory.createEmptyBorder(
				5, 5, 5, 5)));
		rbSensors = new JRadioButton[numSensors];
		ButtonGroup group = new ButtonGroup();
		for (int i = 0; i < numSensors; i++) {
			rbSensors[i] = new JRadioButton();
			group.add(rbSensors[i]);
			rbSensors[i].addActionListener(this);
			pSensors.add(rbSensors[i]);
		}
		panel.add(pSensors);

		JPanel pMatrix = new JPanel();
		pMatrix.setBorder(BorderFactory.createCompoundBorder(BorderFactory
				.createTitledBorder("Data"), BorderFactory.createEmptyBorder(0,
				0, 0, 0)));
		panel.add(pMatrix);

		jtSensors = new JTable(4, 4);
		jtSensors.setEnabled(false);
		pMatrix.add(jtSensors);

		sensor = 0;
		rbSensors[sensor].setSelected(true);
		
		numButtons = trackdJNI.getNumButtons();
		rbButtons = new JRadioButton[numButtons];
		JPanel pButtons = new JPanel(new GridLayout(1, 0));
		pButtons.setBorder(BorderFactory.createCompoundBorder(BorderFactory
				.createTitledBorder("Buttons"), BorderFactory.createEmptyBorder(
				5, 5, 5, 5)));
		for (int i = 0; i < numButtons; i++) {
			rbButtons[i] = new JRadioButton();
			rbButtons[i].setEnabled(false);
			pButtons.add(rbButtons[i]);
		}
		
		panel.add(pButtons);

		numValuators = trackdJNI.getNumValuators();
		JPanel pValuators = new JPanel();
		pValuators.setBorder(BorderFactory.createCompoundBorder(BorderFactory
				.createTitledBorder("Valuators"), BorderFactory.createEmptyBorder(0,
				0, 0, 0)));
		panel.add(pValuators);

		jtValuators = new JTable(1, numValuators);
		jtValuators.setEnabled(false);
		pValuators.add(jtValuators);
		
		update();
		
		frame.pack();
		frame.setVisible(true);
		
		System.out.println("numSensors="+numSensors);
		System.out.println("numButtons="+numButtons);
		System.out.println("numValuators="+numValuators);
		
	}

	public void run() {
		Thread.currentThread().setPriority(Thread.MIN_PRIORITY);
		while (Thread.currentThread() == anim) {
			try {
				update();
				Thread.sleep(TIMESTEP);
			} catch (InterruptedException e) {
				break;
			}
		}
	}

	public void actionPerformed(ActionEvent event) {
		for (int i = 0; i < numSensors; i++)
			if (rbSensors[i].isSelected()) {
				sensor = i;
				break;
			}
	}

	public void update() {
		float[] matrix = new float[16];
		trackdJNI.getMatrix(matrix, sensor);
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				jtSensors.setValueAt(new Float(matrix[4 * i + j]), i, j);
		for (int i=0; i<numButtons; i++) {
			boolean pressed = trackdJNI.getButton(i) != 0;
			rbButtons[i].setSelected(pressed);
		}
		for (int i=0; i<numValuators; i++) {
			double val = trackdJNI.getValuator(i);
			jtValuators.setValueAt(val, 0, i);
		}
	}

	public void start() {
		if (anim == null)
			anim = new Thread(this);
		anim.start();
	}

	public void stop() {
		anim = null;
	}

	public static void main(String[] args) {
		Trackd me = new Trackd();
		me.init();
		me.start();
	}

}
