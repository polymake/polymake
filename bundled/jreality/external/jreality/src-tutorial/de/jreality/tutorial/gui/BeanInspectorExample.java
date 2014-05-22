package de.jreality.tutorial.gui;


import de.jreality.geometry.ParametricSurfaceFactory;
import de.jreality.geometry.ParametricSurfaceFactory.Immersion;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.Shell;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tutorial.geom.ParametricSurfaceExample;
import de.jtem.beans.InspectorPanel;
import de.jtem.jrworkspace.plugin.PluginInfo;

/** Extends {@link ParametricSurfaceExample} by an inspector plugin panel.
 * 
 * @author G. Paul Peters, 22.07.2009
 *
 */
public class BeanInspectorExample {
	public static class Swallowtail implements Immersion {
		public void evaluate(double u, double v, double[] xyz, int index) {
			xyz[index]= 10*(u-v*v);
			xyz[index+1]= 10*u*v;
			xyz[index+2]= 10*(u*u-4*u*v*v);
		}
		public int getDimensionOfAmbientSpace() { return 3;	}
		public boolean isImmutable() { return true; }
	};
	
	public static void main(String[] args) {
		ParametricSurfaceFactory psf = new ParametricSurfaceFactory(new Swallowtail());
		psf.setUMin(-.3);psf.setUMax(.3);psf.setVMin(-.4);psf.setVMax(.4);
		psf.setULineCount(20);psf.setVLineCount(20);
		psf.setGenerateEdgesFromFaces(true);
		psf.setGenerateVertexNormals(true);
		psf.update();
		
		SceneGraphComponent sgc = new SceneGraphComponent("Swallowtail");
		sgc.setGeometry(psf.getIndexedFaceSet());
		
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentTools());
		v.setContent(sgc);

		//create an Inspector for the domain and tell it about the method "update", which it needs to call if
		//changes should have an effect.
		InspectorPanel inspector = new InspectorPanel();
		inspector.setObject(psf, "update");
		
		//add the inspector to the viewer. 
		ViewShrinkPanelPlugin plugin = new ViewShrinkPanelPlugin() {
			@Override
			public PluginInfo getPluginInfo() {
				return new PluginInfo("Domain");
			}
		};
		plugin.getShrinkPanel().add(inspector);
		v.registerPlugin(plugin);
		
		v.setShowPanelSlots(true, true, true, true);
		v.getPlugin(Shell.class).getShrinkPanel().setShrinked(false);
		
		//Start the viewer
		v.startup();
		
	}	
}
