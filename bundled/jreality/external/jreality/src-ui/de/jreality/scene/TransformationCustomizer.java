package de.jreality.scene;

import java.awt.Component;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Customizer;
import java.text.NumberFormat;

import javax.swing.Box;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.text.NumberFormatter;

import de.jreality.math.FactoredMatrix;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;


/**
 * Transformation inspector.
 * 
 * @author msommer
 */
public class TransformationCustomizer  extends JPanel implements Customizer, TransformationListener {

	private Transformation transformation;
	private FactoredMatrix matrix;
	
	private static final int X=0, Z=2, ANGLE=3;  //Y=1
	private static final int TRANSLATION=0, ROTATION=1, SCALE=2;
	
	private double[] translation, rotation, scale;
	private double angle;
	private Entry[] tEntries=new Entry[3], rEntries=new Entry[4], sEntries=new Entry[3];
	private String[] labels = {"X", "Y", "Z", "\u03B1"};
	private JTextField name;
	private JButton chain;
	private boolean chained;
	

	public TransformationCustomizer() {
		this(null);
	}

	public TransformationCustomizer(Transformation t) {
		
		GridBagLayout gbl = new GridBagLayout();
		setLayout(gbl);
		//setBorder(new EmptyBorder(10,10,10,0));
		
		GridBagConstraints labelConstraint = new GridBagConstraints();
		labelConstraint.gridwidth = GridBagConstraints.REMAINDER; //new line after labels
		labelConstraint.anchor = GridBagConstraints.WEST;  //horizontal alignment

		
		GridBagConstraints labelRightConstraint = (GridBagConstraints)labelConstraint.clone();
		labelRightConstraint.anchor = GridBagConstraints.WEST;  //horizontal alignment
		labelRightConstraint.weightx=1;
		labelRightConstraint.insets=new Insets(10, 0, 0, 0);
		labelRightConstraint.gridx=2;
			
		GridBagConstraints editorConstraint = new GridBagConstraints();
		editorConstraint.anchor = GridBagConstraints.WEST;  //horizontal alignment
		
		GridBagConstraints lastColumnConstraint=(GridBagConstraints)editorConstraint.clone();
		lastColumnConstraint.anchor = GridBagConstraints.WEST;
		lastColumnConstraint.weightx=1;
		lastColumnConstraint.gridwidth=GridBagConstraints.REMAINDER;

		//NAME
//		Font f = new Font("Helvetica", Font.BOLD, 12);
		JLabel label = new JLabel("Name", JLabel.LEFT);
//		label.setFont(f);
		//label.setToolTipText("Name of the Transformation");
		gbl.setConstraints(label, labelConstraint);
		add(label);
		name = new JTextField(18);
		//name.setFont(new Font("Courier", Font.PLAIN, 12));
		name.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				transformation.setName(name.getText());
			}
		});
		GridBagConstraints c = (GridBagConstraints)editorConstraint.clone();
		c.insets=new Insets(0, 5, 0, 0);
		c.gridwidth=3;
		gbl.setConstraints(name, c);
		add(name);
		Component lastColumn = Box.createHorizontalGlue();
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);

		//TRANSLATE
		label = new JLabel("Translation", JLabel.LEFT);
//		label.setFont(f);
		labelConstraint.insets=new Insets(10, 0, 0, 0);
		gbl.setConstraints(label, labelConstraint);
		add(label);
		for (int i=X; i<=Z; i++) {
			tEntries[i] = new Entry(labels[i], 0, TRANSLATION);
			gbl.setConstraints(tEntries[i], editorConstraint);
			add(tEntries[i]);
		}
		lastColumn = Box.createHorizontalGlue();
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);
		
		//ROTATE
		label = new JLabel("Rotation", JLabel.LEFT);
//		label.setFont(f);
		gbl.setConstraints(label, labelConstraint);
		add(label);
		for (int i=X; i<=Z; i++) {
			rEntries[i] = new Entry(labels[i], 0, ROTATION);
			gbl.setConstraints(rEntries[i],  editorConstraint);
			add(rEntries[i]);
		}
		lastColumn = Box.createHorizontalGlue();
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);
		rEntries[ANGLE] = new Entry(labels[ANGLE], angle, ROTATION);
		gbl.setConstraints(rEntries[ANGLE], lastColumnConstraint);
		add(rEntries[ANGLE]);
		lastColumn = Box.createHorizontalGlue();
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);
		
		//SCALE
		label = new JLabel("Scale", JLabel.LEFT);
//		label.setFont(f);
		gbl.setConstraints(label, labelConstraint);
		add(label);
		for (int i=X; i<=Z; i++) {
			sEntries[i] = new Entry(labels[i], 0, SCALE);
			gbl.setConstraints(sEntries[i], editorConstraint);
			add(sEntries[i]);
		}
		lastColumn = Box.createHorizontalGlue();
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);
		chain = new JButton();
		chain.setToolTipText("Toggle scalar scale");
		chained = false;
		final ImageIcon openChain = new ImageIcon(TransformationCustomizer.class.getResource("openChain.gif"));
		final ImageIcon closedChain = new ImageIcon(TransformationCustomizer.class.getResource("closedChain.gif"));
		chain.setIcon(chained ? closedChain : openChain);
		chain.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				chained = !chained;
				chain.setIcon(chained ? closedChain : openChain);
			}
		});
		chain.setMargin(new Insets(0, 0, 0, 0));
		editorConstraint.insets=new Insets(0,19, 5, 5);
		editorConstraint.gridwidth=3;
		editorConstraint.fill=GridBagConstraints.HORIZONTAL;
		editorConstraint.anchor = GridBagConstraints.NORTH;
		gbl.setConstraints(chain, editorConstraint);
		add(chain);
		
		lastColumn = Box.createHorizontalGlue();
		lastColumnConstraint.weighty=1;
		gbl.setConstraints(lastColumn, lastColumnConstraint);
		add(lastColumn);
		
	
//		addComponentListener(new ComponentAdapter() {
//			public void componentShown(ComponentEvent e) {
//				System.out.println(".componentShown()");
//			}
//		});
		
		if (t!=null) setObject(t);
	}

	
	public void setObject(Object t) {
		if (transformation==t) return;
		
		if (transformation!=null) transformation.removeTransformationListener(this);
		transformation = (Transformation) t;
		transformation.addTransformationListener(this);
		
		update();
	}
	
	
	private void update() {
		
		if (!name.hasFocus()) name.setText(transformation.getName());
		matrix = new FactoredMatrix(transformation.getMatrix());
		
		translation = matrix.getTranslation();
		rotation = matrix.getRotationAxis();
		angle = matrix.getRotationAngle();
		scale = matrix.getStretch();
		//update entries
		for (int i=X; i<=Z; i++) {
			tEntries[i].setValue(translation[i]);
			rEntries[i].setValue(rotation[i]);
			sEntries[i].setValue(scale[i]);
		}
		rEntries[ANGLE].setValue(angle);
		
		//repaint();
	}
	
	
	private void updateTransformation(int type) {
		switch (type) {
		case TRANSLATION:
			for (int i=X; i<=Z; i++) translation[i] = tEntries[i].getValue();
			matrix.setTranslation(translation);
			break;
		case ROTATION:
			for (int i=X; i<=Z; i++) rotation[i] = rEntries[i].getValue();
			angle = rEntries[ANGLE].getValue();
			matrix.setRotation(angle, rotation);
			break;
		case SCALE:
			for (int i=X; i<=Z; i++) scale[i] = sEntries[i].getValue();
			matrix.setStretch(scale);
			break;
		}
		transformation.setMatrix(matrix.getArray());
	}
	
	
	public void transformationMatrixChanged(TransformationEvent ev) {
//		if (isShowing()) 
		update();  //TODO: even updates if JPanel is "not shown" (object other than transformation selected)
	}
	

//----------------------------------------------------------------------
	//testing layout
	public static void main(String[] args) {
		JFrame frame = new JFrame("TransformationCustomizer");
		frame.getContentPane().add(new TransformationCustomizer(new Transformation("transformation")));
		
		frame.pack();
		frame.setVisible(true);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}
	
	
//----------------------------------------------------------------------
	private class Entry extends JPanel implements ActionListener {
		private JLabel label;
		private JFormattedTextField textField;
		private NumberFormatter formatter = null;
		private NumberFormat numberFormat = null;
		private double value;
		private int type;

		public Entry(String label, double d, int type) {
			super();
			value = d;
			this.type = type;
			this.label = new JLabel(label + ":", JLabel.LEFT);
			this.label.setFont(new Font("Helvetica", Font.PLAIN, 12));

			numberFormat = NumberFormat.getNumberInstance();
			formatter = new NumberFormatter(numberFormat);
			formatter.setValueClass(Double.class);
			textField = new JFormattedTextField(formatter);
			textField.setFont(new Font("Courier", Font.PLAIN, 12));
			textField.setValue(new Double(d));
			textField.setColumns(6); // get some space
			textField.addActionListener(this);
			Box box = Box.createHorizontalBox();
//			box.setBorder(BorderFactory.createEtchedBorder());
			box.add(this.label);
			box.add(Box.createHorizontalStrut(2));
			box.add(textField);
			add(box);
		}

		public void actionPerformed(ActionEvent ev) {
			updateValues();
			updateTransformation(type);
		}

		private void updateValues() {
			switch (type) {
			case SCALE:
				if (chained) {
					updateValue();
					for (int i=X; i<=Z; i++) sEntries[i].setValue(getValue());
				}
				else for (int i=X; i<=Z; i++) sEntries[i].updateValue();  //update scale values
				break;
			case ROTATION:
				for (int i=X; i<=Z; i++) rEntries[i].updateValue();
				rEntries[ANGLE].updateValue();
				break;
			case TRANSLATION:
				for (int i=X; i<=Z; i++) tEntries[i].updateValue();
			}
		}
		
		public void updateValue() {
			if (!textField.isEditValid()) {  //text is invalid.
				Toolkit.getDefaultToolkit().beep();
				textField.selectAll();
			} else
				try { //text is valid
					textField.commitEdit();  //so use text
					java.lang.Double dd = (java.lang.Double) textField.getValue();
					value = dd.doubleValue();
				} catch (java.text.ParseException exc) {
					exc.printStackTrace();
				}
		}

		public void setValue(double d) {
			value = d;
			if (!textField.hasFocus()) textField.setValue(new Double(d));
		}

		public double getValue() {
			return value;
		}
		
	}//end of class Entry

}
