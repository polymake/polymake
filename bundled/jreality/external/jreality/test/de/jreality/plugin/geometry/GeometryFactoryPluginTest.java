package de.jreality.plugin.geometry;

import static de.jreality.junitutils.Assert.assertDifferent;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.beans.PropertyDescriptor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.logging.Level;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import de.jreality.geometry.AbstractGeometryFactoryCustomizer;
import de.jreality.geometry.ParametricSurfaceFactory;
import de.jreality.geometry.ParametricSurfaceFactoryCustomizer;
import de.jreality.junitutils.GuiTestUtils.ComponentsFinder;
import de.jreality.junitutils.PropertyVault;
import de.jreality.ui.widgets.TextSlider;
import de.jtem.jrworkspace.plugin.simplecontroller.SimpleController;

public class GeometryFactoryPluginTest {

	private static List<Method>
		writeMethods = new ArrayList<Method>(),
		readMethods = new ArrayList<Method>();
	private static List<Object> allPropsBeforeMod = new ArrayList<Object>();
	private static List<Double> dblPropsBeforeMod = new ArrayList<Double>();
	private static final ComponentsFinder<ParametricSurfaceFactoryCustomizer> customizerFinder = new ComponentsFinder<ParametricSurfaceFactoryCustomizer>(ParametricSurfaceFactoryCustomizer.class);

	private List<TextSlider<? extends Number>> sliders = new ArrayList<TextSlider<? extends Number>>();
	private ParametricSurfaceFactory psf;
	private ParametricSurfaceFactoryPlugin psfPlugin;

	@BeforeClass
	public static void getCustomizerMethods() {
		AbstractGeometryFactoryCustomizer<ParametricSurfaceFactory> customizer =
			new ParametricSurfaceFactoryCustomizer(new ParametricSurfaceFactory());
		addMethodsTo(customizer.getIntegerSliders().keySet(), readMethods, writeMethods);
		addMethodsTo(customizer.getDoubleSliders().keySet(), readMethods, writeMethods);
		addMethodsTo(customizer.getJToggleButtons().keySet(), readMethods, writeMethods);
	}

	private static void addMethodsTo(Set<PropertyDescriptor> properties, List<Method> readMethods,
		List<Method> writeMethods) {
		for (PropertyDescriptor pd : properties) {
			readMethods.add(pd.getReadMethod());
			writeMethods.add(pd.getWriteMethod());
		}
	}

	@Before
	public void init() throws Exception {
		psf = new ParametricSurfaceFactory();
		psfPlugin = new ParametricSurfaceFactoryPlugin(psf);
		de.jtem.jrworkspace.logging.LoggingSystem.LOGGER.setLevel(Level.WARNING);
		SimpleController controller = new SimpleController();
		controller.registerPlugin(psfPlugin);
		controller.startupLocal();
		
		sliders.addAll(customizerFinder.getComponents(psfPlugin.getShrinkPanel()).get(0).getIntegerSliders().values());
		sliders.addAll(customizerFinder.getComponents(psfPlugin.getShrinkPanel()).get(0).getDoubleSliders().values());	

	}
	
	@After
	public void clearLists() {
		sliders.clear();
		allPropsBeforeMod.clear();
		dblPropsBeforeMod.clear();
	}

	@Test
	public void testShrinkPanelContent() {
		assertEquals(1, customizerFinder.getComponents(psfPlugin.getShrinkPanel()).size());
	}

	@Test
	public void testStateStoringOfAllCutomizableMethods() throws Exception {
		PropertyVault properties = new PropertyVault();
		psfPlugin.storeStates(properties);

		modifyAllProperties();
		
		assertAllPropertiesModified();

		psfPlugin.restoreStates(properties);

		assertAllPropertiesRestored();
	}
	
	private void modifyAllProperties() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
		for (int i = 0; i < readMethods.size(); i++) {
			Object value = readMethods.get(i).invoke(psf);
			allPropsBeforeMod.add(value);
			if (value instanceof Integer) {
				writeMethods.get(i).invoke(psf, (Integer) value + 1);
			} else if (value instanceof Double) {
				writeMethods.get(i).invoke(psf, (Double) value + .1);
			} else if (value instanceof Boolean) {
				writeMethods.get(i).invoke(psf, !(Boolean) value);
			} else
				fail("This test only supports properties of type Integer, Boolean, or Double, but "
					+ value.getClass() + " found.");
		}

	}

	private void assertAllPropertiesModified() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
		for (int i = 0; i < readMethods.size(); i++) {
			if (readMethods.get(i).invoke(psf).equals(allPropsBeforeMod.get(i)))
				fail("Property " + readMethods.get(i).getName() + " was not modified.");
		}
	}

	private void assertAllPropertiesRestored() throws IllegalArgumentException, IllegalAccessException,
		InvocationTargetException {
		for (int i = 0; i < readMethods.size(); i++) {
			Object actual = readMethods.get(i).invoke(psf);
			Object expected = allPropsBeforeMod.get(i);
			if (!expected.equals(actual))
				fail("Property " + readMethods.get(i).getName() + " was not restored: expected " + expected
					+ ", but was " + actual);
		}
	}

	@Test
	public void testMaxMinPropertiesStoring() throws Exception {
		PropertyVault properties = new PropertyVault();
		psfPlugin.storeStates(properties);

		changeSliders();

		assertAllSlidersModified();

		psfPlugin.restoreStates(properties);

		assertAllSlidersRestored();
	}

	@SuppressWarnings("unchecked")
	private void changeSliders() {
		for (TextSlider<? extends Number> slider : sliders) {
			double max = slider.getMax().doubleValue();
			double min = slider.getMin().doubleValue();
			double value = slider.getValue().doubleValue();
			dblPropsBeforeMod.add(max);
			dblPropsBeforeMod.add(min);
			dblPropsBeforeMod.add(value);
			if (slider.getMax() instanceof Integer) { 
				((TextSlider<Integer>)slider).setMax((int) max + 1);
				((TextSlider<Integer>)slider).setMin((int) min - 1);
				((TextSlider<Integer>)slider).setValue((int) value + 1);
			} else {
				((TextSlider<Double>)slider).setMax(max + 1.17);
				((TextSlider<Double>)slider).setMin(min - 1.17);
				((TextSlider<Double>)slider).setValue(value + 1.17); //not necessary, as DblSlider values slightly change, when max min values change			
			}
		}
	}

	private void assertAllSlidersModified() {
		int propertyIndex = 0;
		for (TextSlider<? extends Number> slider : sliders) {
			double max = dblPropsBeforeMod.get(propertyIndex++);
			double min = dblPropsBeforeMod.get(propertyIndex++);
			double value = dblPropsBeforeMod.get(propertyIndex++);
			assertDifferent("Slider <" + slider.getName() + "> not modified: ", max, slider.getMax().doubleValue(), 1e-44);
			assertDifferent("Slider <" + slider.getName() + "> not modified: ", min, slider.getMin().doubleValue(), 1e-4);
			assertDifferent("Slider <" + slider.getName() + "> not modified: ", value, slider.getValue().doubleValue(), 1e-4);
		}
	}

	private void assertAllSlidersRestored() {
		int propertyIndex = 0;
		for (TextSlider<? extends Number> slider : sliders) {
			double max = dblPropsBeforeMod.get(propertyIndex++);
			double min = dblPropsBeforeMod.get(propertyIndex++);
			double value = dblPropsBeforeMod.get(propertyIndex++);
			assertEquals("Max of Slider <" + slider.getName() + "> not restored", max, slider.getMax().doubleValue(), 1e-4);
			assertEquals("Min of Slider <" + slider.getName() + "> not restored", min, slider.getMin().doubleValue(), 1e-4);
			assertEquals("Value of Slider <" + slider.getName() + "> not restored", value, slider.getValue().doubleValue(), 1e-4);
		}
	}

}
