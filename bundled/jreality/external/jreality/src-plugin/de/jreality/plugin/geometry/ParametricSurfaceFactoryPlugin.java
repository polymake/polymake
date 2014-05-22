package de.jreality.plugin.geometry;

import java.awt.GridLayout;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import de.jreality.geometry.AbstractGeometryFactoryCustomizer;
import de.jreality.geometry.ParametricSurfaceFactory;
import de.jreality.geometry.ParametricSurfaceFactoryCustomizer;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.scene.SceneShrinkPanel;
import de.jreality.ui.widgets.TextSlider;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

/** Wraps a {@link ParametricSurfaceFactoryCustomizer} into a plugin. This takes care of showing up somewhere 
 * (see {@link SceneShrinkPanel}) in {@link JRViewer} and storing its properties by overriding the corresponding methods of {@link Plugin}.
 * 
 * @author G. Paul Peters, 16.06.2010
 */
public class ParametricSurfaceFactoryPlugin extends SceneShrinkPanel {
	
	
	private final AbstractGeometryFactoryCustomizer<ParametricSurfaceFactory> psfCustomizer;
	private final ParametricSurfaceFactory psf;
	private final ArrayList<PropertyDescriptor> properties = new ArrayList<PropertyDescriptor>();
	private final List<TextSlider<Integer>> intSliders = new ArrayList<TextSlider<Integer>>();
	private final List<TextSlider<Double>> dblSliders = new ArrayList<TextSlider<Double>>();

	public ParametricSurfaceFactoryPlugin(ParametricSurfaceFactory psf) {
		this.psf = psf;
		psfCustomizer = new ParametricSurfaceFactoryCustomizer(psf);
		psfCustomizer.updateGuiFromFactory();
		initPanel();
		readPropertiesAndSliders();
	}

	private void initPanel() {
		getShrinkPanel().setLayout(new GridLayout());
		getShrinkPanel().add(psfCustomizer);
	}
	
	private void readPropertiesAndSliders() {
		final Map<PropertyDescriptor, TextSlider<Integer>> integerSliders = psfCustomizer.getIntegerSliders();
		final Map<PropertyDescriptor, TextSlider<Double>> doubleSliders = psfCustomizer.getDoubleSliders();
		properties.addAll(integerSliders.keySet());
		properties.addAll(doubleSliders.keySet());
		properties.addAll(psfCustomizer.getJToggleButtons().keySet());
		intSliders.addAll(integerSliders.values());
		dblSliders.addAll(doubleSliders.values());
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Parameter Domain Settings");
		info.vendorName = "G. Paul Peters";
		info.email = "peters@math.tu-berlin.de";
		return info;
	}

	@Override
	public void restoreStates(Controller c) throws Exception {
		super.restoreStates(c);

		for (PropertyDescriptor property : properties) {
			String name = property.getName();
			Method readMethod = property.getReadMethod();
			Method writeMethod = property.getWriteMethod();
			writeMethod.invoke(psf, c.getProperty(getClass(), name, readMethod.invoke(psf)));
		}
		psfCustomizer.updateGuiFromFactory();
		
		for (TextSlider<Integer> slider: intSliders) {
			slider.setMax(c.getProperty(getClass(), slider.getName() + "Max", slider.getMax()));
			slider.setMin(c.getProperty(getClass(), slider.getName() + "Min", slider.getMin()));
		}
		for (TextSlider<Double> slider: dblSliders) {
			slider.setMax(c.getProperty(getClass(), slider.getName() + "Max", slider.getMax()));
			slider.setMin(c.getProperty(getClass(), slider.getName() + "Min", slider.getMin()));
		}
	}

	@Override
	public void storeStates(Controller c) throws Exception {
		super.storeStates(c);
		
		for (TextSlider<Integer> slider: intSliders) {
			c.storeProperty(getClass(), slider.getName() + "Max", slider.getMax());
			c.storeProperty(getClass(), slider.getName() + "Min", slider.getMin());
		}
		for (TextSlider<Double> slider: dblSliders) {
			c.storeProperty(getClass(), slider.getName() + "Max", slider.getMax());
			c.storeProperty(getClass(), slider.getName() + "Min", slider.getMin());
		}
		for (PropertyDescriptor property : properties) {
			String name = property.getName();
			Method readMethod = property.getReadMethod();
			c.storeProperty(this.getClass(), name, readMethod.invoke(psf));
		}
	}
	
	
	
	
	

}
