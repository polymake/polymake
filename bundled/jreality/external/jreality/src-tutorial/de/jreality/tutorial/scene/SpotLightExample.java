/*
 * Author	gunn
 * Created on Oct 12, 2005
 *
 */
package de.jreality.tutorial.scene;

import java.awt.Color;

import de.jreality.geometry.BezierPatchMesh;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.RotateTool;
import de.jreality.util.SceneGraphUtility;


/**
 * @author gunn
 *
 */
public class SpotLightExample  {

	static double[][][] square = new double[2][2][3];
	public static void main(String[] args) {
	 	SceneGraphComponent lightIcon;
		SceneGraphComponent spot;
		SceneGraphComponent theShape;
		SceneGraphComponent theWorld;
		theWorld = SceneGraphUtility.createFullSceneGraphComponent("world");
		theWorld.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, false);
		theWorld.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
		theWorld.getAppearance().setAttribute(CommonAttributes.SMOOTH_SHADING, true);
		
		theShape = SceneGraphUtility.createFullSceneGraphComponent("shape");
		theShape.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.WHITE);
		// we need a highly refined flat patch to shine the spotlight on.
		for (int i =0; i<2; ++i)	
			for (int j = 0; j<2; ++j)	{
				square[i][j][0] = 9*(i-.5);
				square[i][j][1] = 9*(j-.5);
			}
		BezierPatchMesh bpm = new BezierPatchMesh(1, 1, square);
	   	for (int j =0; j<=7; ++j)	bpm.refine();
	   	IndexedFaceSet qmpatch = BezierPatchMesh.representBezierPatchMeshAsQuadMesh(bpm);	 
	   	theShape.setGeometry(qmpatch);
	   	theWorld.addChild(theShape);
	   	// create a spotlight and an icon to represent it in the scene
		spot = SceneGraphUtility.createFullSceneGraphComponent("Spot");
		SpotLight sl = new SpotLight();
 		sl.setColor(Color.YELLOW);
		sl.setConeAngle(Math.PI/6.0 );
		sl.setConeDeltaAngle(Math.PI/12.0);  // seems to have no effect in OpenGL
 		sl.setDistribution(10.0);
 		sl.setIntensity(1.0);
  		sl.setFalloff(1, 0, .1);
 		MatrixBuilder.euclidean().translate(0,0,2).rotateX(Math.PI).assignTo(spot);
 		lightIcon = SceneGraphUtility.createFullSceneGraphComponent("Light icon");
 		lightIcon.setGeometry(Primitives.cylinder(10));
 		lightIcon.getTransformation().setMatrix(P3.makeStretchMatrix(null, new double[] {.2, .2, .8}));
		lightIcon.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.RED);
		lightIcon.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, true);
		lightIcon.getAppearance().setAttribute(CommonAttributes.TUBES_DRAW, false);
		spot.addChild(lightIcon);
 		spot.getAppearance().setAttribute(CommonAttributes.LIGHTING_ENABLED, false);
 		// put a rotate tool in the node to allow rotation of the spotlight icon
 		spot.addTool(new RotateTool());
	   	theWorld.addChild(spot);
	   	// turn the world so the cylinder is visible and "rotate-able"
	   	MatrixBuilder.euclidean().rotateY(Math.PI/4).assignTo(theWorld);
		Viewer viewer = JRViewer.display(theWorld); 
		// remove the default lights 
		SceneGraphUtility.removeLights(viewer);
 		spot.setLight(sl);
	}
  }
