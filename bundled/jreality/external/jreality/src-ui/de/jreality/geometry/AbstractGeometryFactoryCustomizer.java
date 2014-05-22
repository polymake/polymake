package de.jreality.geometry;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.Customizer;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.swing.Box;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.JToggleButton;
import javax.swing.border.EmptyBorder;

import de.jreality.ui.widgets.TextSlider;

abstract public class AbstractGeometryFactoryCustomizer<FACTORY> extends JPanel implements Customizer {
	private static final long serialVersionUID = 1L;
	
	FACTORY factory;
	final Method updateMethod = getUpdateMethod();
	final List<SliderProperty<?>> sliderProperties = new LinkedList<SliderProperty<?>>();
	final List<ToggleProperty> toggleProperties = new LinkedList<ToggleProperty>();

	boolean alwaysCallUpdateMethdod = true;

	abstract Class<FACTORY> getAcceptableClass();
	
	void initSliderProperties() {
		sliderProperties.clear();
	}
	void initToggleProperties() {
		toggleProperties.clear();
	}
	
	Method getUpdateMethod() {
		try {
			return getAcceptableClass().getMethod("update");
		} catch (Exception e) {
			throw new IllegalStateException(e);
		} 
	}
	
	@SuppressWarnings("unchecked")
	public void setObject(Object bean) {
		if (!getAcceptableClass().isInstance(bean)) {
				throw new IllegalArgumentException(this.getClass() + " can only inspect objects of type "
					+ getAcceptableClass().getCanonicalName());
		}
		factory = (FACTORY)bean;
		initSliderProperties();
		initToggleProperties();
		initPanel();
		revalidate();
	}
	
	public boolean isAlwaysCallUpdateMethdod() {
		return this.alwaysCallUpdateMethdod;
	}
	public void setAlwaysCallUpdateMethdod(boolean alwaysCallUpdateMethdod) {
		this.alwaysCallUpdateMethdod = alwaysCallUpdateMethdod;
	}
	
	public Map<PropertyDescriptor, JToggleButton> getJToggleButtons() {
		Map<PropertyDescriptor, JToggleButton> toggleButtons = new HashMap<PropertyDescriptor, JToggleButton>();
		for (ToggleProperty toggleProperty : toggleProperties) {
			toggleButtons.put(toggleProperty.getPropertyDescriptor(), toggleProperty.getButton());
		}
		return toggleButtons ;
	}
	public Map<PropertyDescriptor, TextSlider<Double>> getDoubleSliders() {
		Map<PropertyDescriptor, TextSlider<Double>> integerSliders = new HashMap<PropertyDescriptor, TextSlider<Double>>();
		for (SliderProperty<? extends Number> sliderProperty : sliderProperties) {
			if (DoubleSliderProperty.class.isInstance(sliderProperty)) {
				integerSliders.put(sliderProperty.getPropertyDescriptor(), ((DoubleSliderProperty) sliderProperty).getSlider());
			}
		}
		return integerSliders ;
	}
	public Map<PropertyDescriptor, TextSlider<Integer>> getIntegerSliders() {
		Map<PropertyDescriptor, TextSlider<Integer>> integerSliders = new HashMap<PropertyDescriptor, TextSlider<Integer>>();
		for (SliderProperty<? extends Number> sliderProperty : sliderProperties) {
			if (IntegerSliderProperty.class.isInstance(sliderProperty)) {
				integerSliders.put(sliderProperty.getPropertyDescriptor(), ((IntegerSliderProperty) sliderProperty).getSlider());
			}
		}
		return integerSliders ;
	}
	public void updateGuiFromFactory() {
		List<Property> properties = new LinkedList<Property>();
		properties.addAll(sliderProperties);
		properties.addAll(toggleProperties);
		for (Property property : properties) {
			property.updateGuiFromFactory();
		}
	}
	protected void initPanel() {
		removeAll();
		setLayout(new GridBagLayout());
		addToggleButtons();
		addSlider();
	}
	private void addSlider() {
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(0, 5, 0, 5);
		c.gridwidth = GridBagConstraints.REMAINDER;
		for (SliderProperty<?> sliderProperty : sliderProperties) {
			add(sliderProperty.getSlider(), c);
		}
	}
	private void addToggleButtons() {
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(2, 2, 2, 2);
		c.fill = GridBagConstraints.BOTH;
		c.gridwidth = 1;
		c.weightx = .3;
		int toggleButtons = 1;
		for (ToggleProperty toggleProperty : toggleProperties) {
			add(toggleProperty.getButton(), c);
			toggleButtons++;
			if (0 == toggleButtons % 3) {
				c.gridwidth = GridBagConstraints.REMAINDER;
			} else {
				c.gridwidth = 1;
			}
		}
		if (1 != toggleButtons % 3) {
			c.gridwidth = GridBagConstraints.REMAINDER;
			add(Box.createHorizontalGlue(), c);
		}
	}
	/** Subclasses need to implement a null constructor, otherwise bean reflection instantiation will fail. 
	 */
	public AbstractGeometryFactoryCustomizer() {
		this(null);
	}

	public AbstractGeometryFactoryCustomizer(FACTORY factory) {
		if (factory != null) setObject(factory);
	}
	
	abstract class Property {
		final PropertyDescriptor propertyDescriptor;

		Property(String propertyName) {
			try {
				propertyDescriptor = new PropertyDescriptor(propertyName, getAcceptableClass());
			} catch (Exception e) {
				throw new IllegalStateException(e);
			}
		}
		
		public PropertyDescriptor getPropertyDescriptor() {
			return propertyDescriptor;
		}
		
		abstract public void updateGuiFromFactory();  
	}
	

	abstract class SliderProperty<T extends Number> extends Property {
		private final TextSlider<T> slider;

		SliderProperty(String propertyName, String propertyLabel, T min, T max) {
			super(propertyName);
			slider = initSlider(propertyLabel, min, max, readValueFromFactory());
			addListener();
		}

		abstract TextSlider<T> initSlider(String propertyLabel, T min, T max, T value) ;

		public TextSlider<T> getSlider() {
			return this.slider;
		}

		void addListener() {
			slider.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					writeSliderValueToFactory();
				}
			});
		}

		void writeSliderValueToFactory() {
			try {
				propertyDescriptor.getWriteMethod().invoke(factory, slider.getValue());
				if (alwaysCallUpdateMethdod) updateMethod.invoke(factory);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		@SuppressWarnings("unchecked")
		T readValueFromFactory() {
			try {
				return (T) propertyDescriptor.getReadMethod().invoke(factory);
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}

		@Override
		public void updateGuiFromFactory() {
			try {
				slider.setValue(readValueFromFactory());
			} catch (Exception e) {
				e.printStackTrace();
			}	
		}
}
	

	class IntegerSliderProperty extends SliderProperty<Integer> {
		IntegerSliderProperty(String propertyName, Integer min, Integer max) {
			this(propertyName, propertyName, min, max);
		}

		IntegerSliderProperty(String propertyName, String propertyLabel, Integer min, Integer max) {
			super(propertyName, propertyLabel, min, max);
		}

		@Override
		TextSlider<Integer> initSlider(String propertyLabel, Integer min, Integer max, Integer value) {
			TextSlider<Integer> slider = new TextSlider.Integer(propertyLabel, JSlider.HORIZONTAL, min, max, value);
			return slider;
		}
	}

	
	class DoubleSliderProperty extends SliderProperty<Double> {
		DoubleSliderProperty(String propertyName, Double min, Double max) {
			this(propertyName, propertyName, min, max);
		}

		DoubleSliderProperty(String propertyName, String propertyLabel, Double min, Double max) {
			super(propertyName, propertyLabel, min, max);
		}

		@Override
		TextSlider<Double> initSlider(String propertyLabel, Double min, Double max, Double value) {
			TextSlider<Double> slider = new TextSlider.Double(propertyLabel, JSlider.HORIZONTAL, min, max, value);
			return slider;
		}
	}
	
	
	class ToggleProperty extends Property {
		private final JToggleButton button;
		
		ToggleProperty(String propertyName) {
			this(propertyName, propertyName);
		}
		
		ToggleProperty(String propertyName, String propertyLabel) {
			super(propertyName);
			button = new JToggleButton(propertyLabel, readStateFromFactory());
			button.setBorder(new EmptyBorder(2, 2, 2, 2));
			button.setMinimumSize(button.getPreferredSize());
			button.setToolTipText(propertyName);
			addListener();
		}
		
		public JToggleButton getButton() {
			return button;
		}
		
		void addListener() {
			button.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					writeButtonStateToFactory();
				}
			});
		}

		void writeButtonStateToFactory() {
			try {
				propertyDescriptor.getWriteMethod().invoke(factory, button.isSelected());
				if (alwaysCallUpdateMethdod) updateMethod.invoke(factory);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		boolean readStateFromFactory() {
			try {
				return (Boolean) propertyDescriptor.getReadMethod().invoke(factory);
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}

		@Override
		public void updateGuiFromFactory() {
			try {
				button.setSelected(readStateFromFactory());
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
}
