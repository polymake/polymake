/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common.ui;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Iterator;
import java.util.Vector;

import javax.swing.Box;
import javax.swing.JLabel;
import javax.swing.JSlider;
import javax.swing.JTextField;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

/**
 * @author wotzlaw
 * 
 * TODO To change the template for this generated type comment go to Window -
 * Preferences - Java - Code Style - Code Templates
 */
public class PolymakeSlider {

	protected Box sliderBox = Box.createHorizontalBox();

	protected JLabel label;

	protected JTextField valueField = new JTextField(3);

	protected JSlider slider = new JSlider(0, 100);

	protected Vector sliderListeners = new Vector();

	private double min = 0;

	private double max = 1;

	private double value = 0.0;

	public PolymakeSlider(String labelText, double m_min, double m_max,	double defaultValue) {

		min = m_min;
		max = m_max;

		label = new JLabel(labelText);

		valueField.setText(Double.toString(defaultValue));
		valueField.setPreferredSize(new Dimension(40, 20));
		valueField.setMaximumSize(new Dimension(40, 20));
		valueField.setMinimumSize(new Dimension(40, 20));
		valueField.setActionCommand("valueField");
		valueField.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				try {
					double newValue = Double.parseDouble(valueField.getText());
					if (value != newValue) {
						setDoubleValue(newValue);
						notifyListeners();
					}
				} catch (NumberFormatException nfe) {
					// TODO: Care about stupid input!
				}
			}
		});

		slider.setValue((int) Math.round(100 * (defaultValue - min)
				/ (max - min)));
		slider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				if (slider.getValueIsAdjusting() && value != slider.getValue()) {
					setDoubleValue(min + slider.getValue() * (max - min)
							/ 100.0);
					notifyListeners();
				}
			}
		});

		label.setBorder(new javax.swing.border.EmptyBorder(0, 4, 0, 2));
		label.setPreferredSize(new Dimension(100, 20));

		sliderBox.add(label);
		sliderBox.add(Box.createHorizontalGlue());
		sliderBox.add(valueField);
		sliderBox.add(slider);
	}

	public Component getComponent() {
		return sliderBox;
	}

	public double getDoubleValue() {
		synchronized (valueField) {
			value = Double.parseDouble(valueField.getText());
		}
		return value;
	}

	public void setDoubleValue(double newValue) {
		if (newValue < min)
			newValue = min;
		if (newValue > max)
			newValue = max;
		value = newValue;
		synchronized (valueField) {
			valueField.setText(Double.toString(newValue));
			slider.setValue((int) Math.round(100 * (newValue - min)
					/ (max - min)));
		}
	}

	public String getLabel() {
		return label.getText();
	}

	public void addSliderListener(SliderListener sl) {
		sliderListeners.add(sl);
	}

	private void notifyListeners() {
		for (Iterator sli = sliderListeners.iterator(); sli.hasNext();) {
			SliderListener sl = (SliderListener) sli.next();
			sl.sliderValueChanged(new SliderEvent(this,
					(int) SliderEvent.SLIDER_EVENT));
		}
	}
	
	public void setEnabled(boolean t) {
		slider.setEnabled(t);
		valueField.setEnabled(t);
	}
}
