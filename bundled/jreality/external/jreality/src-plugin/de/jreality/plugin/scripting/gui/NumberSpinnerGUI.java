package de.jreality.plugin.scripting.gui;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.Map;

import javax.swing.Icon;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.plugin.icon.ImageHook;
import de.jreality.plugin.scripting.AbstractPythonGUI;
import de.jreality.plugin.scripting.PythonGUI;
import de.jreality.plugin.scripting.PythonGUIPlugin;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class NumberSpinnerGUI extends PythonGUIPlugin<Number> {

	private static final Icon
		pluginIcon = ImageHook.getIcon("control_eject.png");
	private Map<Long, NumberGUI>
		guiMap = new HashMap<Long, NumberGUI>();
	
	
	private class NumberGUI extends AbstractPythonGUI<Number> {

		private NumberFrontendGUI
			frontend = new NumberFrontendGUI(this);
		private NumberBackendGUI
			backend = new NumberBackendGUI(this);
		
		public NumberGUI(long id) {
			super(id, NumberSpinnerGUI.class);
			setVariableName("d");
		}
		
		protected void setId(long newId) {
			this.id = newId;
		}
		
		@Override
		protected void fireValueChanged() {
			super.fireValueChanged();
		}
		
		@Override
		public void setVariableDisplay(String display) {
			super.setVariableDisplay(display);
			frontend.nameLabel.setText(display);
		}
		@Override
		public Number getVariableValue() {
			if (backend.isIntegerValue()) {
				return frontend.model.getNumber().intValue();
			} else {
				return frontend.model.getNumber();
			}
		}
		@Override
		public void setVariableValue(Number val) {
			frontend.model.setValue(val.doubleValue());
		}

		
		@Override
		public JPanel getFrontendGUI() {
			return frontend;
		}
		@Override
		public JPanel getBackendGUI() {
			return backend;
		}
		
		@Override
		public void storeProperties(Controller c) {
			super.storeProperties(c);
			c.storeProperty(NumberSpinnerGUI.class, "value" + getId(), getVariableValue());
			c.storeProperty(NumberSpinnerGUI.class, "maxValue" + getId(), backend.getMaxValue());
			c.storeProperty(NumberSpinnerGUI.class, "minValue" + getId(), backend.getMinValue());
			c.storeProperty(NumberSpinnerGUI.class, "stepSize" + getId(), backend.getStepSize());
			c.storeProperty(NumberSpinnerGUI.class, "isInteger" + getId(), backend.isIntegerValue());
			c.storeProperty(NumberSpinnerGUI.class, "showSlider" + getId(), backend.isIntegerValue());
		}

		@Override
		public void restoreProperties(Controller c) {
			super.restoreProperties(c);
			Number value = c.getProperty(NumberSpinnerGUI.class, "value" + getId(), 0.0);
			Number maxValue = c.getProperty(NumberSpinnerGUI.class, "maxValue" + getId(), 10000.0);
			Number minValue = c.getProperty(NumberSpinnerGUI.class, "minValue" + getId(), -10000.0);
			Number stepSize = c.getProperty(NumberSpinnerGUI.class, "stepSize" + getId(), 0.1);
			boolean isInteger = c.getProperty(NumberSpinnerGUI.class, "isInteger" + getId(), false);
			boolean showSlider = c.getProperty(NumberSpinnerGUI.class, "showSlider" + getId(), false);
			setVariableValue(value);
			
			backend.setListenersEnabled(false);
			try {
				backend.setMaxValue(maxValue.doubleValue());
				backend.setMinValue(minValue.doubleValue());
				backend.setStepSize(stepSize.doubleValue());
				backend.setIntegerValue(isInteger);
				backend.setInstantExecute(isInstant());
				backend.setShowSlider(showSlider);
				backend.updateFrontend();
			} finally {
				backend.setListenersEnabled(true);
			}
		}

		@Override
		public void deleteProperties(Controller c) {
			super.deleteProperties(c);
			c.deleteProperty(NumberSpinnerGUI.class, "value" + getId());
			c.deleteProperty(NumberSpinnerGUI.class, "maxValue" + getId());
			c.deleteProperty(NumberSpinnerGUI.class, "minValue" + getId());
			c.deleteProperty(NumberSpinnerGUI.class, "stepSize" + getId());
		}
		
	}
	

	private class NumberFrontendGUI extends JPanel implements ChangeListener {

		private static final long 
			serialVersionUID = 1L;
		private NumberGUI 
			gui = null;
		private JLabel
			nameLabel = new JLabel("Value");
		private SpinnerNumberModel
			model = new SpinnerNumberModel(0.0, -10000, 10000, 0.1);
		private JSpinner
			spinner = new JSpinner(model);
		private JSlider
			slider = new JSlider(0, 1000);
		private boolean
			listenersEnabled = true; 
		
		public NumberFrontendGUI(NumberGUI gui) {
			this.gui = gui;
			setLayout(new GridBagLayout());
			GridBagConstraints c = new GridBagConstraints();
			c.weightx = 1.0;
			c.weighty = 0.0;
			c.gridwidth = GridBagConstraints.RELATIVE;
			c.fill = GridBagConstraints.HORIZONTAL;
			nameLabel.setIcon(pluginIcon);
			add(nameLabel, c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(spinner, c);
			add(slider, c);
			spinner.addChangeListener(this);
			slider.addChangeListener(this);
			setShowSlider(false);
		}
		
		@Override
		public void stateChanged(ChangeEvent e) {
			if (!listenersEnabled) return;
			try {
				listenersEnabled = false;
				double maxD = ((Number)model.getMaximum()).doubleValue();
				double minD = ((Number)model.getMinimum()).doubleValue();
				double spanD = maxD - minD;
				int maxI = slider.getMaximum();
				int minI = slider.getMinimum();
				int spanI = maxI - minI;
				if (spinner == e.getSource()) {
					double val = model.getNumber().doubleValue();
					double relVal = (val - minD) / spanD;
					int relIntStep = (int)(relVal * spanI + minI);
					slider.setValue(relIntStep);
				}
				if (slider == e.getSource()) {
					double val = slider.getValue();
					double relVal = (val - minI) / spanI;
					double spinnerVal = relVal * spanD + minD;
					Number valNum = model.getNumber();
					if (valNum instanceof Integer) {
						model.setValue((int)spinnerVal);
					} else {
						model.setValue(spinnerVal);
					}
				}
				gui.fireValueChanged();
			} finally {
				listenersEnabled = true;
			}
		}
		
		public void setNumberModel(SpinnerNumberModel model) {
			this.model = model;
			spinner.setModel(model);
			stateChanged(new ChangeEvent(spinner));
		}
		
		public void setShowSlider(boolean showSlider) {
			slider.setVisible(showSlider);
		}
		
	}
	
	
	private class NumberBackendGUI extends JPanel implements ChangeListener, ActionListener {

		private static final long 
			serialVersionUID = 1L;
		private NumberGUI
			gui = null;
		private JCheckBox
			showSliderChecker = new JCheckBox("Show Slider"),
			integerValueChecker = new JCheckBox("Is Integer Value");
		private SpinnerNumberModel
			minValueModel = new SpinnerNumberModel(-10000.0, -100000, 100000, 1.0),
			maxValueModel = new SpinnerNumberModel(10000.0, -100000, 100000, 1.0),
			stepSizeModel = new SpinnerNumberModel(0.1, 0.01, 10.0, 0.01);
		private JSpinner
			minValueSpinner = new JSpinner(minValueModel),
			maxValueSpinner = new JSpinner(maxValueModel),
			stepSizeSpinner = new JSpinner(stepSizeModel);
		private JCheckBox
			instantChecker = new JCheckBox("Execute On Edit");
		private boolean
			listenersEnabled = true;
		
		public NumberBackendGUI(NumberGUI gui) {
			this.gui = gui;
			setLayout(new GridBagLayout());

			GridBagConstraints c = new GridBagConstraints();
			c.weightx = 1.0;
			c.fill = GridBagConstraints.HORIZONTAL;
			c.insets = new Insets(2, 2, 2, 2);
			
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(instantChecker, c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(integerValueChecker, c);
			add(showSliderChecker, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Min Value"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(minValueSpinner, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Max Value"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(maxValueSpinner, c);
			c.gridwidth = GridBagConstraints.RELATIVE;
			add(new JLabel("Step Size"), c);
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(stepSizeSpinner, c);
			
			minValueSpinner.addChangeListener(this);
			maxValueSpinner.addChangeListener(this);
			stepSizeModel.addChangeListener(this);
			instantChecker.addActionListener(this);
			integerValueChecker.addActionListener(this);
			showSliderChecker.addActionListener(this);
		}
		
		public void setListenersEnabled(boolean listenersEnabled) {
			this.listenersEnabled = listenersEnabled;
		}
		
		@Override
		public void stateChanged(ChangeEvent e) {
			if (!listenersEnabled) return;
			updateFrontend();
		}
		
		@Override
		public void actionPerformed(ActionEvent e) {
			if (!listenersEnabled) return;
			if (integerValueChecker == e.getSource()) {
				gui.fireValueChanged();
			}
			if (instantChecker == e.getSource()) {
				gui.setInstant(instantChecker.isSelected());
			}
			updateFrontend();
		}

		public void updateFrontend() {
			if (isIntegerValue()) {
				int min = minValueModel.getNumber().intValue();
				int max = maxValueModel.getNumber().intValue();
				int step = stepSizeModel.getNumber().intValue();
				int val = gui.frontend.model.getNumber().intValue();
				step = step == 0 ? 1 : step;
				gui.frontend.setNumberModel(new SpinnerNumberModel(val, min, max, step));
			} else {
				double min = minValueModel.getNumber().doubleValue();
				double max = maxValueModel.getNumber().doubleValue();
				double step = stepSizeModel.getNumber().doubleValue();
				double val = gui.frontend.model.getNumber().doubleValue();
				gui.frontend.setNumberModel(new SpinnerNumberModel(val, min, max, step));
			}
			gui.frontend.setShowSlider(isShowSlider());
		}

		public double getMaxValue() {
			return maxValueModel.getNumber().doubleValue();
		}
		public void setMaxValue(Number val) {
			maxValueModel.setValue(val);
		}
		public double getMinValue() {
			return minValueModel.getNumber().doubleValue();
		}
		public void setMinValue(Number val) {
			minValueModel.setValue(val);
		}
		public double getStepSize() {
			return stepSizeModel.getNumber().doubleValue();
		}
		public void setStepSize(Number val) {
			stepSizeModel.setValue(val);
		}
		public boolean isIntegerValue() {
			return integerValueChecker.isSelected();
		}
		public void setIntegerValue(boolean integerValue) {
			integerValueChecker.setSelected(integerValue);
		}
		public void setInstantExecute(boolean instant) {
			instantChecker.setSelected(instant);
		}
		public boolean isShowSlider() {
			return showSliderChecker.isSelected();
		}
		public void setShowSlider(boolean showSlider) {
			showSliderChecker.setSelected(showSlider);
		}
	}
	
	@Override
	public PythonGUI<Number> getGUI(long id) {
		if (guiMap.containsKey(id)) {
			return guiMap.get(id);
		}
		NumberGUI gui = new NumberGUI(id);
		guiMap.put(id, gui);
		return gui;
	}
	
	@Override
	public void setGUIId(long oldId, long newId) {
		NumberGUI gui = guiMap.remove(oldId);
		if (gui == null) return;
		gui.setId(newId);
		guiMap.put(newId, gui);
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = super.getPluginInfo();
		info.name = "Number Spinner";
		info.icon = pluginIcon;
		return info;
	}
	
}
