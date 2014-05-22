package de.jreality.geometry;

import static org.junit.Assert.fail;

import java.awt.Component;
import java.beans.PropertyDescriptor;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.swing.JToggleButton;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;

import de.jreality.junitutils.GuiTestUtils.ComponentsFinder;
import de.jreality.junitutils.GuiTestUtils.Matcher;
import de.jreality.ui.widgets.TextSlider;
import de.jtem.beans.InspectorPanel;

public abstract class AbstractGeometryFactoryCustomizerAndBeanInfoTest
	<FACTORY, CUSTOMIZER extends AbstractGeometryFactoryCustomizer<FACTORY>> {

	abstract FACTORY getFactory();

	abstract CUSTOMIZER getACustomizer();

	abstract int nbOfIntSliders();

	abstract int nbOfDblSliders();

	abstract int nbOfToggleButtons();

	InspectorPanel inspector;
	FACTORY factory;
	CUSTOMIZER customizer;

	private static final ComponentsFinder<TextSlider.Integer> integerSliderFinder =
		new ComponentsFinder<TextSlider.Integer>(TextSlider.Integer.class);
	private static final ComponentsFinder<TextSlider.Double> doubleSliderFinder =
		new ComponentsFinder<TextSlider.Double>(TextSlider.Double.class);
	private static final ComponentsFinder<JToggleButton> visibleJToggleButtonFinder =
		new ComponentsFinder<JToggleButton>(new Matcher<JToggleButton>() {
			public JToggleButton castWhenMatches(Component cp) {
				if (cp instanceof JToggleButton && cp.isVisible()) {
					return (JToggleButton) cp;
				}
				else {
					return null;
				}
			}
		});

	@Before
	public void init() {
		factory = getFactory();
		customizer = getACustomizer();
		inspector = new InspectorPanel();
		inspector.setObject(factory);
		inspector.setAutoRefresh(false);
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Test
	public void testNbOfIntSlidersViaIntrospection() {
		Assert.assertEquals(nbOfIntSliders(), integerSliderFinder.getComponentsCount(inspector));
	}

	@Test
	public void testNbOfDblSlidersViaIntrospection() {
		Assert.assertEquals(nbOfDblSliders(), doubleSliderFinder.getComponentsCount(inspector));
	}

	@Test
	public void testNbOfTButtonsViaIntrospection() {
		Assert.assertEquals(nbOfToggleButtons(), visibleJToggleButtonFinder.getComponentsCount(inspector));
	}

	@Test(expected = IllegalArgumentException.class)
	public void testSetWrongObjectType() {
		customizer.setObject(new Integer(1));
	}

	@Test
	public void compareReturnedSlidersAndActualGuiComponents() {
		Set<TextSlider<? extends Number>> sliders = new HashSet<TextSlider<? extends Number>>();
		sliders.addAll(customizer.getIntegerSliders().values());
		sliders.addAll(customizer.getDoubleSliders().values());
	
		Set<TextSlider<? extends Number>> guiSliders = new HashSet<TextSlider<? extends Number>>();
		guiSliders.addAll(integerSliderFinder.getComponents(customizer));
		guiSliders.addAll(doubleSliderFinder.getComponents(customizer));
	
		Assert.assertTrue(
			"Customizer returns different set of sliders from the set contained in the JPanel container.", sliders
				.equals(guiSliders));
	}

	@Test
	public void compareReturnedButtonsAndActualGuiButtons() {
		Set<JToggleButton> sliders = new HashSet<JToggleButton>();
		sliders.addAll(customizer.getJToggleButtons().values());
	
		HashSet<JToggleButton> guiSliders = new HashSet<JToggleButton>();
		guiSliders.addAll(visibleJToggleButtonFinder.getComponents(customizer));
	
		Assert.assertTrue(
			"Customizer returns different set of sliders from the set contained in the JPanel container.", sliders
				.equals(guiSliders));
	}

	@Test
	public void testIntSliderFunctionality() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
			Map<PropertyDescriptor, TextSlider<Integer>> sliders = customizer.getIntegerSliders();
			assertIntSlidersEqualFactoryProperties(sliders);
			
			for (PropertyDescriptor pd : sliders.keySet()) {
				int expected = 13 + (Integer) pd.getReadMethod().invoke(factory);
				sliders.get(pd).setValue(expected);
				sliders.get(pd).fireActionPerformed();
				int actual = (Integer) pd.getReadMethod().invoke(factory);
				Assert.assertEquals("Slider for " + factory.getClass() + "." + pd.getName() + " does not work.", expected, actual);
			}
		}

	private void assertIntSlidersEqualFactoryProperties(Map<PropertyDescriptor, TextSlider<Integer>> sliders) throws IllegalAccessException,
		InvocationTargetException {
			for (PropertyDescriptor pd : sliders.keySet()) {
				int expected = (Integer) pd.getReadMethod().invoke(factory);
				int actual = sliders.get(pd).getValue();
				Assert.assertEquals("Slider for " + factory.getClass() + "." + pd.getName() + ": ", expected, actual);
			}
		}

	@Test
	public void testDblSliderFunctionality() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
			Map<PropertyDescriptor, TextSlider<Double>> sliders = customizer.getDoubleSliders();
			assertDoubleSlidersEqualFactoryProperties(sliders);
			
			for (PropertyDescriptor pd : sliders.keySet()) {
				double expected = 13.17 + (Double) pd.getReadMethod().invoke(factory);
				sliders.get(pd).setValue(expected);
				sliders.get(pd).fireActionPerformed();
				double actual = (Double) pd.getReadMethod().invoke(factory);
				Assert.assertEquals("Slider for " + factory.getClass() + "." + pd.getName() + " does not work.", expected, actual, 1E5);
			}
		}

	private void assertDoubleSlidersEqualFactoryProperties(Map<PropertyDescriptor, TextSlider<Double>> sliders) throws IllegalAccessException,
		InvocationTargetException {
			for (PropertyDescriptor pd : sliders.keySet()) {
				double expected = (Double) pd.getReadMethod().invoke(factory);
				double actual = sliders.get(pd).getValue();
				Assert.assertEquals("Slider for " + factory.getClass() + "." + pd.getName() + ": ", expected, actual, 1E5);
			}
		}

	@Test
	public void testTButtonFunctionality() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
			Map<PropertyDescriptor,JToggleButton> buttons = customizer.getJToggleButtons();
			assertButtonStatesEqualFactoryProperties(buttons);
			
			for (PropertyDescriptor pd : buttons.keySet()) {
				boolean expected = (Boolean) pd.getReadMethod().invoke(factory);
				buttons.get(pd).setSelected(expected);
				expected = !expected;
				buttons.get(pd).doClick();
				boolean actual = (Boolean) pd.getReadMethod().invoke(factory);
				Assert.assertEquals("JToggleButton for " + factory.getClass() + "." + pd.getName() + " does not work.", expected, actual);
			}
		}

	private void assertButtonStatesEqualFactoryProperties(Map<PropertyDescriptor, JToggleButton> buttons) throws IllegalAccessException,
		InvocationTargetException {
			for (PropertyDescriptor pd : buttons.keySet()) {
				boolean expected = (Boolean) pd.getReadMethod().invoke(factory);
				boolean actual = buttons.get(pd).isSelected();
				Assert.assertEquals("JToggleButton for " + factory.getClass() + "." + pd.getName() +": ", expected, actual);
			}
		}

	@Test
	public void testRereadFactory() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
			List<PropertyDescriptor> propertyDescriptors = new ArrayList<PropertyDescriptor>();
			propertyDescriptors.addAll(customizer.getIntegerSliders().keySet());
			propertyDescriptors.addAll(customizer.getDoubleSliders().keySet());
			propertyDescriptors.addAll(customizer.getJToggleButtons().keySet());
			for (PropertyDescriptor pd : propertyDescriptors) {
				Object value = pd.getReadMethod().invoke(factory);
				if (value instanceof Integer)
					pd.getWriteMethod().invoke(factory, (Integer)value + 1);
				else if (value instanceof Double)
					pd.getWriteMethod().invoke(factory, (Double)value - .1);
				else if (value instanceof Boolean)
					pd.getWriteMethod().invoke(factory, ! (Boolean)value);
				else
					fail("Unsupported property type: " + value.getClass());					
			}
			
			customizer.updateGuiFromFactory();
			
			assertButtonStatesEqualFactoryProperties(customizer.getJToggleButtons());
			assertIntSlidersEqualFactoryProperties(customizer.getIntegerSliders());
			assertDoubleSlidersEqualFactoryProperties(customizer.getDoubleSliders());
		}

}
