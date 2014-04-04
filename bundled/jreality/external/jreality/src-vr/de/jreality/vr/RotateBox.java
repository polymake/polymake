package de.jreality.vr;

import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.net.URL;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.List;

import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.util.Secure;

@SuppressWarnings("serial")
public class RotateBox extends JPanel {

	private static final double PI2 = Math.PI / 2;
	
	private List<ChangeListener> changeListeners = new ArrayList<ChangeListener>();
	private Matrix rotation=new Matrix();
	
	public RotateBox() {
		super(new GridLayout(3, 3));
		
		ImageIcon rotateLeft = Secure.doPrivileged(new PrivilegedAction<ImageIcon>() {
			public ImageIcon run() {
				URL imgURL = ViewerVR.class.getResource("rotleft.gif");
				return new ImageIcon(imgURL);
			}
		});

		ImageIcon rotateRight = Secure.doPrivileged(new PrivilegedAction<ImageIcon>() {
			public ImageIcon run() {
				URL imgURL = ViewerVR.class.getResource("rotright.gif");
				return new ImageIcon(imgURL);
			}
		});


		setBorder(new EmptyBorder(20, 0, 20, 0));

		JButton xRotateLeft = new JButton(rotateLeft);
		xRotateLeft.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateX(
						-PI2).getMatrix());
				fireChange();
			}
		});
		Insets insets = new Insets(0, 0, 0, 0);
		Dimension dim = new Dimension(25, 22);
		xRotateLeft.setMargin(insets);
		xRotateLeft.setMaximumSize(dim);
		add(xRotateLeft);
		JLabel label = new JLabel("x");
		label.setHorizontalAlignment(SwingConstants.CENTER);
		add(label);
		JButton xRotateRight = new JButton(rotateRight);
		xRotateRight.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateX(
						PI2).getMatrix());
				fireChange();
			}
		});
		xRotateRight.setMargin(insets);
		xRotateRight.setMaximumSize(dim);
		add(xRotateRight);

		JButton yRotateLeft = new JButton(rotateLeft);
		yRotateLeft.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateY(
						-PI2).getMatrix());
				fireChange();
			}
		});
		yRotateLeft.setMargin(insets);
		yRotateLeft.setMaximumSize(dim);
		add(yRotateLeft);
		label = new JLabel("y");
		label.setHorizontalAlignment(SwingConstants.CENTER);
		add(label);
		JButton yRotateRight = new JButton(rotateRight);
		yRotateRight.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateY(
						PI2).getMatrix());
				fireChange();
			}
		});
		yRotateRight.setMargin(insets);
		yRotateRight.setMaximumSize(dim);
		add(yRotateRight);

		JButton zRotateLeft = new JButton(rotateLeft);
		zRotateLeft.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateZ(
						-PI2).getMatrix());
				fireChange();
			}
		});
		zRotateLeft.setMargin(insets);
		zRotateLeft.setMaximumSize(dim);
		add(zRotateLeft);
		label = new JLabel("z");
		label.setHorizontalAlignment(SwingConstants.CENTER);
		add(label);
		JButton zRotateRight = new JButton(rotateRight);
		zRotateRight.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				rotation.multiplyOnLeft(MatrixBuilder.euclidean().rotateZ(
						PI2).getMatrix());
				fireChange();
			}
		});
		zRotateRight.setMargin(insets);
		zRotateRight.setMaximumSize(dim);
		add(zRotateRight);

	}

	protected void fireChange() {
		synchronized (changeListeners) {
			ChangeEvent e = new ChangeEvent(this);
			for (ChangeListener cl : changeListeners)
				cl.stateChanged(e);
		}
	}
	
	public void addChangeListener(ChangeListener cl) {
		synchronized (changeListeners) {
			changeListeners.add(cl);
		}
	}

	public void removeChangeListener(ChangeListener cl) {
		synchronized (changeListeners) {
			changeListeners.remove(cl);
		}
	}

	public Matrix getMatrix() {
		return new Matrix(rotation);
	}
}
