package de.jreality.tutorial.gui;

import java.awt.GridBagLayout;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.ParametricSurfaceFactory;
import de.jreality.geometry.ParametricSurfaceFactory.Immersion;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tutorial.geom.ParametricSurfaceExample;
import de.jreality.ui.JSliderVR;
import de.jtem.jrworkspace.plugin.PluginInfo;

/** 
 * Extends {@link ParametricSurfaceExample}. Visualizes the associated family of 
 * the catenoid and helicoid. Adds a slider for the parameter alpha of the associated
 * family
 *
 * @author G. Paul Peters, 22.07.2009
 *
 */
public class SliderExample {
	
	/** This class implements the associated family of the Helicoid and Catenoid
	 *  
 	 * x = cos alpha sinh v sin u + sin alpha cosh v cos u
	 * y = -cos alpha sinh v cos u + sin alpha cosh v sin u 
	 * z = u cos alpha + v sin alpha
	 * 
	 * u \in [-Pi,Pi[
	 * v \in \R
	 * 
	 * alpha = 0 helicoid
	 * alpha = Pi/2 catenoid
	 * 
	 */
	public static class HelicoidCatenoid implements Immersion {
		
		//add a parameter to the surface
		private double alpha;

		//get and set methods to read and write the parameter
		public double getAlpha() {
			return alpha;
		}
		public void setAlpha(double alpha) {
			this.alpha = alpha;
		}
		
		//implementation of the formula
		public void evaluate(double u, double v, double[] xyz, int index) {
			xyz[index]= Math.cos(alpha) * Math.sinh(v) * Math.sin(u) + Math.sin(alpha) * Math.cosh(v) * Math.cos(u);
			xyz[index+2]= -Math.cos(alpha) * Math.sinh(v) * Math.cos(u) + Math.sin(alpha) * Math.cosh(v) * Math.sin(u);
			xyz[index+1]= u * Math.cos(alpha) + v * Math.sin(alpha);
		}
		//the values of that map are elements of 3-space
		public int getDimensionOfAmbientSpace() { return 3;	}
		//the surface is mutable, because the extra alpha parameter changes the behavior of the evaluate method 
		public boolean isImmutable() { return false; }
	};
	
	public static void main(String[] args) {
		//The immersion and the factory need to be final, because we want to access them from
		//the listener of the factory.
		final HelicoidCatenoid helicoidCatenoid = new HelicoidCatenoid();
		final ParametricSurfaceFactory psf = new ParametricSurfaceFactory(helicoidCatenoid);
		//parameters of the factory
		psf.setUMin(-Math.PI);psf.setUMax(Math.PI);psf.setVMin(-1);psf.setVMax(1);
		psf.setULineCount(31);psf.setVLineCount(10);
		psf.setGenerateEdgesFromFaces(true);
		psf.setGenerateVertexNormals(true);
		psf.update();
		
		//put the geometry into a scene graph component
		SceneGraphComponent sgc = new SceneGraphComponent("Helicoid-Catenoid");
		sgc.setGeometry(psf.getIndexedFaceSet());
		
		//build a JRViewerVR
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentTools());
		v.setContent(sgc);

		//create a slider for the parameter alpha 
		final int steps=60;
		//also the slider is made final, so that we can access it from the listener
		final JSliderVR slider=new JSliderVR(0,steps,0);
		//now we add an anonymous class as listener to the slider by implementing the
		//interface ChangeListener 
		slider.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				helicoidCatenoid.setAlpha( 2*Math.PI * slider.getValue()/ steps );
				psf.update();
			}
		});
		
		//wrap  the slider in a plugin in order to add it to the viewer
		ViewShrinkPanelPlugin plugin = new ViewShrinkPanelPlugin() {
			@Override
			public PluginInfo getPluginInfo() {
				return new PluginInfo("alpha");
			}
		};
		//a layout manager is needed, so that the slider is stretched
		//to fill the available horizontal space
		plugin.getShrinkPanel().setLayout(new GridBagLayout());
		plugin.getShrinkPanel().add(slider);
		v.registerPlugin(plugin);
		
		//Start the viewer
		v.startup();
	}

    
}
