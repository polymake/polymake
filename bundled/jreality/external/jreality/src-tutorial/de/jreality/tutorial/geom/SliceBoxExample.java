package de.jreality.tutorial.geom;

import static de.jreality.geometry.SphereUtility.SPHERE_SUPERFINE;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS_DEFAULT;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static java.awt.Color.RED;
import static java.awt.Color.WHITE;

import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.SliceBoxFactory;
import de.jreality.geometry.SphereUtility;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.TwoSidePolygonShader;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.util.SceneGraphUtility;


/**
 * A tutorial demonstrating the use of {@link SliceBoxFactory}
 * 
 * @author Charles Gunn
 *
 */
public class SliceBoxExample{

	public static void main(String[] args) throws IOException {
		SliceBoxExample lcpe = new SliceBoxExample();
		SceneGraphComponent root = lcpe.makeExample();
		JRViewer.display(root);
	}
	  
	  SceneGraphComponent makeExample()	{
		  	SceneGraphComponent worldSGC = makeWorld();
		  	SliceBoxFactory sbf = new SliceBoxFactory(worldSGC);
		  	sbf.setSeparation(.4);
		  	sbf.update();
			SceneGraphComponent foo = sbf.getSliceBoxSGC();
			foo.addTool(new ClickWheelCameraZoomTool());
			return foo;
	  }
	  
	  /**
	   * This is just a sphere -- for real applications, replace this sgc with your own.
	   * @return	the scene graph component to be sliced.
	   */
	  protected SceneGraphComponent makeWorld()	{	
			SceneGraphComponent world;
			world = SceneGraphUtility.createFullSceneGraphComponent("container");
			world.setPickable( false);
			world.addChild(SphereUtility.tessellatedCubeSphere(SPHERE_SUPERFINE));
			world.getAppearance().setAttribute(POLYGON_SHADER,TwoSidePolygonShader.class);
			world.getAppearance().setAttribute(POLYGON_SHADER+".front."+DIFFUSE_COLOR, new Color(0,204,204));
			world.getAppearance().setAttribute(POLYGON_SHADER+".back."+DIFFUSE_COLOR, new Color(204,204,0));
			world.getAppearance().setAttribute(LINE_SHADER+"."+DIFFUSE_COLOR, WHITE);
			world.getAppearance().setAttribute(POINT_SHADER+"."+DIFFUSE_COLOR, RED);
			world.getAppearance().setAttribute(POINT_SHADER+"."+POINT_RADIUS, 2*POINT_RADIUS_DEFAULT);
			return world;
	  }
	  
}
