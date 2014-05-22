package de.jreality.geometry;

import java.awt.Color;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;

public class ThickenedSurfaceFactoryTest {
	SceneGraphComponent sgc1, root;
	IndexedFaceSet surface, thickSurface;
	int refineLevel = 1, steps = 4;
	double holeSize = 1.0, thickness = .1;
	boolean makeHoles = true;
	double[][] profile = new double[][]{{0,0},{.02, .25}, {.333,.5},{.666, .5},{.98, .25},{1,0}};
	ThickenedSurfaceFactory tsf;
	
	public  SceneGraphComponent makeWorld() {
		sgc1 = SceneGraphUtility.createFullSceneGraphComponent();
		surface = SphereUtility.tessellatedIcosahedronSphere(refineLevel); //Primitives.cube(); // DiscreteGroupUtility.archimedeanSolid("3.4.3.4");////
		tsf = new ThickenedSurfaceFactory(surface);
		tsf.setThickness(thickness);
		tsf.setMakeHoles(makeHoles);
		tsf.setHoleFactor(holeSize);
		tsf.setStepsPerEdge(steps);
		tsf.setKeepFaceColors(true);
		tsf.setProfileCurve(profile);
		updateGeometry();
		root = SceneGraphUtility.createFullSceneGraphComponent();
		root.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR,	Color.yellow);
		root.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
		root.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
		root.addChild(sgc1);
		return root;
	}

	protected void updateGeometry() {
		tsf.update();
		thickSurface = tsf.getThickenedSurface();
		//thickSurface = GeometryUtilityOverflow.thicken(surface, thickness, true, holeSize, steps, );
		sgc1.setGeometry(thickSurface);
	}
	
	protected void display()	{
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(makeWorld());
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}

	  public static void main(String[] args) {
		  ThickenedSurfaceFactoryTest tsft = new ThickenedSurfaceFactoryTest();
		  tsft.display();
	  }

}
