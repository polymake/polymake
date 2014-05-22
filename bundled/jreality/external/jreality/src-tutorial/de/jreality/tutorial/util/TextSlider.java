package de.jreality.tutorial.util;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.JTextField;
import javax.swing.border.EmptyBorder;
import javax.swing.event.CaretEvent;
import javax.swing.event.CaretListener;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;


/** @deprecated use {@link de.jreality.ui.widgets.TextSlider}
 * 
 */
public abstract class TextSlider<T extends Number> extends JPanel  {
	private static final long serialVersionUID = 1L;

	/** An <code>enum</code> indicating the composition of the <code>TextSlider</code>. 
	 */
	public static enum SliderComposition {SliderOnly, SliderAndTextField,  SliderTextFieldAndMaxMinButtons}
	public static SliderComposition DEFAULT_SLIDER_COMPOSITION = SliderComposition.SliderTextFieldAndMaxMinButtons;

	private static final int TEXT_FIELD_COLUMNS = 6;
	private static final int PREFERRED_HEIGHT = 40;
	private static final int EXTRA_WIDTH = -45;
	
	private final JSlider slider;
	private final JLabel label;
	private final JTextField textField;
	private String textContents;
	private T min, max;

	public T getMin() {
		return min;
	}

	public T getMax() {
		return max;
	}

	public void setMin(T min) {
		this.min=min;
	    Vector<ActionListener> remember = listeners;
	    listeners = null;
	    textField.postActionEvent();
	    listeners=remember;
	}

	public void setMax(T max) {
		this.max=max;
	    Vector<ActionListener> remember = listeners;
	    listeners = null;
	    textField.postActionEvent();
	    listeners=remember;
	}
	
	abstract void adjustSliderMinMax();
	abstract T sliderToText();
	abstract int textToSlider();
	abstract int numberToSlider(T n);
	abstract String getFormattedValue(T n);

	public T getValue()	{
		return sliderToText();
	}

	public void setValue(T n)	{
	    Vector<ActionListener> remember = listeners;
	    listeners = null;
		slider.setValue(numberToSlider(n));
	    textField.setText(getFormattedValue(n));
	    textField.postActionEvent();
	    listeners = remember;
	}


	Vector<ActionListener> listeners;
	
	public void addActionListener(ActionListener l)	{
		if (listeners == null)	listeners = new Vector<ActionListener>();
		if (listeners.contains(l)) return;
		listeners.add(l);
	}
	
	public void removeActionListener(ActionListener l)	{
		if (listeners == null)	return;
		listeners.remove(l);
	}

	public Vector<ActionListener> getActionListeners() {
		return listeners;
	}

	
	public void fireActionPerformed()	{
		if (listeners == null) return;
		if (!listeners.isEmpty())	{
			ActionEvent ae = new ActionEvent(this, 0, "");
			for (int i = 0; i<listeners.size(); ++i)	{
				ActionListener l = (ActionListener) listeners.get(i);
				l.actionPerformed(ae);
			}
		}
	}

	
	private TextSlider(String label, int orientation, T min, T max, T value, int sliderMin, int sliderMax, int sliderValue, 
			SliderComposition sliderComp)	{
		super();
		setName(label);
		if (sliderMax < sliderValue) sliderMax = sliderValue;
		this.label  = new JLabel(label, JLabel.LEFT);
	    this.min=min; this.max=max; 

		slider = initSlider(orientation, sliderMin, sliderMax, sliderValue);
		textField = initTextField();
	    initListeners();
		
		final JButton minButton = initMinButton();
		final JButton maxButton = initMaxButton();
	
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(1, 1, 1, 1);
		c.gridx = 0;
		c.gridwidth = 200;
		c.anchor = GridBagConstraints.WEST;
		c.weightx = 1;
		add(this.label, c);
		c.gridwidth = 1;
		c.gridx = GridBagConstraints.RELATIVE;
		c.anchor = GridBagConstraints.EAST;
		c.weightx = .5;
		add(Box.createHorizontalGlue(), c);
		c.weightx = 0;
		add(minButton, c);
		add(Box.createHorizontalStrut(2), c);
		c.gridwidth = GridBagConstraints.REMAINDER;
		add(maxButton, c);
		c.gridx = 0;
		c.gridwidth = 1;
		c.anchor = GridBagConstraints.WEST;
		add(textField, c);
		c.gridx = 1;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.anchor = GridBagConstraints.EAST;
		add(slider, c);

		switch (sliderComp) {
		case SliderOnly: 
			textField.setVisible(false); 
		case SliderAndTextField: 
			maxButton.setVisible(false); 
			minButton.setVisible(false);
		}

		setPreferredSize(new Dimension(getPreferredSize().width + EXTRA_WIDTH, PREFERRED_HEIGHT));
		setMinimumSize(getPreferredSize());
		setMaximumSize(new Dimension(10000, PREFERRED_HEIGHT));
	}


	private JSlider initSlider(int orientation, int sliderMin, int sliderMax, int sliderValue) {
		JSlider slider = new JSlider(orientation, sliderMin, sliderMax, sliderValue);
		slider.setBounds(1, 1, 1, 1);
		slider.setMinimumSize(slider.getPreferredSize());
		slider.setMaximumSize(new Dimension(10000, slider.getPreferredSize().height));
	    return slider;
	}

	private JTextField initTextField() {
		JTextField textField = new JTextField();
		textField.setText(getFormattedValue(sliderToText()));
	    textContents = textField.getText();
	    textField.setColumns(TEXT_FIELD_COLUMNS);
	    textField.setEditable(true);
		Font  f = new Font("Helvetica", Font.PLAIN, 9);
	    textField.setFont(f);	    
	    
	    Dimension d = textField.getPreferredSize();
	    textField.setPreferredSize(new Dimension(d.width, d.height - 2));
	    textField.setMinimumSize(textField.getPreferredSize());    
	    textField.setMaximumSize(textField.getPreferredSize());   
	    textField.setBorder(new EmptyBorder(1, 1, 1, 1));
	    return textField;
	}

	private void initListeners() {
		textField.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				adjustSliderMinMax();
				slider.setValue(textToSlider());
				textContents = textField.getText();
				fireActionPerformed();
				textField.setForeground(Color.black);
			}
 	    	
 	    });
	    textField.addCaretListener(new CaretListener() {
			public void caretUpdate(CaretEvent e) {
				if (textField.getText().compareTo(textContents) != 0)
					textField.setForeground(Color.RED);
			}	    	
	    });
		slider.addChangeListener( new ChangeListener()	{
		    public void stateChanged(ChangeEvent e) {
			    textField.setText(getFormattedValue(sliderToText()));
		        fireActionPerformed();
		    }
		});
	}

	private JButton initMinButton() {
		final JButton minButton = initButton("min");
		minButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setMin(getValue());
			}
		});		
		return minButton;
	}

	private JButton initMaxButton() {
		final JButton maxButton = initButton("max");
		maxButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				setMax(getValue());
			}
		});
		return maxButton;
	}
	
	private JButton initButton(String label) {
		final JButton button=new JButton(label);
		button.setBorder(new EmptyBorder(1, 2, 1, 2));
		button.setMaximumSize(button.getPreferredSize());
		button.setMinimumSize(button.getPreferredSize());
		button.setToolTipText("Set the current value as " + label + " value of the slider.");
		return button;
	}

	
	public static class Integer extends TextSlider<java.lang.Integer>	{

		private static final long serialVersionUID = 1L;

		public Integer(String l, int orientation, int min, int max, int value)	{
			this(l, orientation, min, max, value, DEFAULT_SLIDER_COMPOSITION);
		}

		public Integer(String l, int orientation, int min, int max, int value, SliderComposition sliderComp) {
			this(l, orientation, min, max, value, min, max, value, sliderComp);
		}
		
		public Integer(String l, int orientation, 
			int min, int max, int value, 
			int sliderMin, int sliderMax, int sliderValue, 
			SliderComposition sliderComp) {
			super(l, orientation, min, max, value, sliderMin, sliderMax, sliderValue, sliderComp);
		}
		java.lang.Integer sliderToText() {
			return super.slider.getValue();
		}
			
		int textToSlider()	{
			return java.lang.Integer.valueOf(super.textField.getText());
		}
		
		int numberToSlider(java.lang.Integer val)	{
			return val;	
		}
		
		void adjustSliderMinMax() {
			int foo = textToSlider();
			if (foo > super.slider.getMaximum()) setMax(foo); 
			if (foo < super.slider.getMinimum()) setMin(foo); 
		}
		@Override
		String getFormattedValue(java.lang.Integer n) {
			return String.format("%d",n);
		}
		@Override
		public void setMax(java.lang.Integer max) {
		    Vector<ActionListener> remember = listeners;
		    listeners = null;
			String text=super.textField.getText();
			super.slider.setMaximum(max);
			super.textField.setText(text);
		    listeners=remember;
			super.setMax(max); 
		}
		@Override
		public void setMin(java.lang.Integer min) {
		    Vector<ActionListener> remember = listeners;
		    listeners = null;
			String text=super.textField.getText();
			super.slider.setMinimum(min);
			super.textField.setText(text);
		    listeners=remember;
			super.setMin(min); 
		}

		@Override
		public void setValue(java.lang.Integer n) {
			if (n > super.slider.getMaximum()) setMax(n);
			if (n < super.slider.getMinimum()) setMin(n); 		
			super.setValue(n);
		}
	}
	
	private static double scaler = 10E8;
	/** @deprecated use {@link de.jreality.ui.widgets.TextSlider.Double}
	 * 
	 */
	public static class Double extends TextSlider<java.lang.Double>	{
		private static final long serialVersionUID = 1L;

		public Double(String l, int orientation, double min, double max, double value)	{
			this(l, orientation, min, max, value, DEFAULT_SLIDER_COMPOSITION);
		}
		public Double(String l, int orientation, double min, double max, double value, SliderComposition sliderComp)	{
			super(l, orientation, min, max, value, 
					0, (int) scaler,  ((int) (scaler*(value-min)/(max-min))), sliderComp);
		}
		
		@Override
		java.lang.Double sliderToText() {
			return sliderToDouble(super.slider.getValue());
		}
		
		@Override
		int textToSlider()	{
			return numberToSlider(java.lang.Double.valueOf(super.textField.getText()));
		}
		
		@Override
		int numberToSlider(java.lang.Double val)	{
			return ((int) (scaler * (val-super.min)/(super.max-super.min)));			
		}
		
		double sliderToDouble(int val) { 
			return (super.min + (super.max-super.min)*(val/scaler)); 
		}

		
		@Override
		void adjustSliderMinMax() {
			double val= java.lang.Double.valueOf(super.textField.getText());
			if (val > super.max) {
				setMax(val);
			}
			if (val < super.min) {
				setMin(val); 
			}
		}
						
		String getFormattedValue(java.lang.Double n) {
			return String.format("%8.4g",n);
		}
	}
	
	/** @deprecated use {@link de.jreality.ui.widgets.TextSlider.DoubleLog}
	 * 
	 */
	public static class DoubleLog extends TextSlider.Double	{
		private static final long serialVersionUID = 1L;

		public DoubleLog(String l, int orientation, double min, double max, double value)	{
			this(l, orientation, min, max,value,DEFAULT_SLIDER_COMPOSITION);
		}
		public DoubleLog(String l, int orientation, double min, double max, double value, SliderComposition sliderComp)	{
			super(l, orientation, min, max, (max-min)*(Math.log(value/min)/Math.log(max/min)),sliderComp);
			if (min < 0 || max < 0)
				throw new IllegalArgumentException("DoubleLog slider only accepts positive limits");
		}
		
		@Override
		int numberToSlider(java.lang.Double val)	{
			double f = Math.log(val.doubleValue()/getMin())/Math.log(getMax()/getMin());
			int ret = ((int) (scaler * f));
			return ret;		
		}
		
		@Override
		double sliderToDouble(int val)		{ 
			double f = val/scaler;
			double a = Math.pow(getMin(), 1.0-f);
			double b = Math.pow(getMax(), f);
			double ret = a*b; 
			return ret;
		}

				
	}

	/** @deprecated use {@link de.jreality.ui.widgets.TextSlider.IntegerLog}
	 * 
	 */
	public static class IntegerLog extends TextSlider.DoubleLog	{
		private static final long serialVersionUID = 1L;

		public IntegerLog(String l, int orientation, double min, double max, double value) {
			super(l, orientation, min, max, value);
		}

		@Override
		double sliderToDouble(int val)		{ 
			double f = val/scaler;
			double a = Math.pow(getMin(), 1.0-f);
			double b = Math.pow(getMax(), f);
			// have to correct some round-off errors due to integer conversion + log distortions
			int iret = (int) (a*b+.5);
			double ret = (double) iret;
			return ret;
		}

		@Override
		String getFormattedValue(java.lang.Double n) {
			System.err.println("integerlog format = "+n);
			return String.format("%8d",((int)(n+.001)));
		}			
	}

}
