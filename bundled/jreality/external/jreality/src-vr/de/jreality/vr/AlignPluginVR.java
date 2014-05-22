package de.jreality.vr;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.prefs.Preferences;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.SwingConstants;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

public class AlignPluginVR extends AbstractPluginVR {

	private static final double MAX_OFFSET = 5;
	
	// maximal radius of tubes or points compared to content size
	public static final double MAX_RADIUS = 0.1;
	
	// ratio of maximal versus minimal value for logarithmic sliders
	public static final int LOGARITHMIC_RANGE = 200;
	
	// maximal horizontal diameter of content in meters
	private static final double MAX_CONTENT_SIZE = 100;

	// defaults for align panel
	private static final double DEFAULT_SIZE = 22;
	private static final double DEFAULT_OFFSET = .3;
	
	// align panel
	private JPanel alignPanel;
	private JSlider sizeSlider;
	private JSlider groundSlider;

	public AlignPluginVR() {
		super("align");
		makeAlignTab();
	}
	
	@Override
	public JPanel getPanel() {
		return alignPanel;
	}
	
	private void makeAlignTab() {
		alignPanel = new JPanel(new BorderLayout());
		alignPanel.setBorder(new EmptyBorder(5, 5, 5, 5));
		Box placementBox = new Box(BoxLayout.X_AXIS);
		Box sizeBox = new Box(BoxLayout.Y_AXIS);
		sizeBox.setBorder(new EmptyBorder(10, 5, 0, 5));
		JLabel sizeLabel = new JLabel("size");
		sizeSlider = new JSlider(SwingConstants.VERTICAL, 0, 100, 0);
		sizeSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setSize(getSize());
			}
		});
		sizeBox.add(sizeLabel);
		sizeBox.add(sizeSlider);
		Box groundBox = new Box(BoxLayout.Y_AXIS);
		groundBox.setBorder(new EmptyBorder(10, 5, 0, 5));
		JLabel groundLabel = new JLabel("level");
		groundSlider = new JSlider(SwingConstants.VERTICAL, -25, 75, 0);
		groundSlider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent arg0) {
				setOffset(getOffset());
			}
		});
		groundBox.add(groundLabel);
		groundBox.add(groundSlider);

		final RotateBox rotateBox = new RotateBox();
		rotateBox.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				getViewerVR().setContentMatrix(rotateBox.getMatrix());
			}
		});
		
		JPanel p = new JPanel(new BorderLayout());
		p.setBorder(new EmptyBorder(5, 30, 5, 20));
		p.add("Center", rotateBox);
		JButton alignButton = new JButton("align");
		alignButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				getViewerVR().alignContent();
			}
		});
		p.add("South", alignButton);
		placementBox.add(sizeBox);
		placementBox.add(groundBox);
		placementBox.add(p);
		alignPanel.add(placementBox);
	}

	public double getSize() {
		double sliderDiam = 0.01 * sizeSlider.getValue();
		return Math.exp(Math.log(LOGARITHMIC_RANGE)*sliderDiam)/LOGARITHMIC_RANGE * MAX_CONTENT_SIZE;
	}

	public void setSize(double d) {
		double sliderDiam = Math.log(d*LOGARITHMIC_RANGE/MAX_CONTENT_SIZE)/Math.log(LOGARITHMIC_RANGE);
		sizeSlider.setValue((int) (sliderDiam * 100));
		getViewerVR().setContentSize(d);
	}

	public double getOffset() {
		return .01 * groundSlider.getValue() * MAX_OFFSET;
	}

	public void setOffset(double offset) {
		groundSlider.setValue((int) (offset/MAX_OFFSET * 100));
		getViewerVR().setContentOffset(offset);
	}

	@Override
	public void restorePreferences(Preferences prefs) {
		// defaults for align panel
		setSize(prefs.getDouble("size", DEFAULT_SIZE));
		setOffset(prefs.getDouble("offset", DEFAULT_OFFSET));
	}
	
	@Override
	public void restoreDefaults() {
		// defaults for align panel
		setSize(DEFAULT_SIZE);
		setOffset(DEFAULT_OFFSET);
	}
	
	@Override
	public void storePreferences(Preferences prefs) {
		// defaults for align panel
		prefs.putDouble("size", getSize());
		prefs.putDouble("offset", getOffset());
	}
}
