package de.jreality.tutorial.app;

import java.io.IOException;

import de.jreality.geometry.SphereUtility;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * A simple example of using an OpenGL shading language shader in a jReality scene graph.
 * @author Charles Gunn
 *
 */
public class GLShadingLangExample01NewBackend {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		
		world.setGeometry(SphereUtility.tessellatedIcosahedronSphere(3, true)); 
		
		//setting the file name of the custom GLSL shader by writing it into the Appearance
		String[] source = new String[2];
		source[0] = "custom/brick.vert";
		source[1] = "custom/brick.frag";
		ap.setAttribute("polygonShader.glsl330-source", source);
		
		//setting all uniform variables by writing them into the Appearance
		double[] brickSize = {.2, .25};
		double[] brickPct = {.5, .75};
		double[] mortarPct = new double[2];
		double[] lightPosition = {-2,0,4};
		mortarPct[0] = 1.0 - brickPct[0];
		mortarPct[1] = 1.0 - brickPct[1];
		ap.setAttribute("polygonShader.SpecularContribution", .5);
		ap.setAttribute("polygonShader.DiffuseContribution", 1.0);
		ap.setAttribute("polygonShader.BrickColor", new double[]{.2,0f,.8});
		ap.setAttribute("polygonShader.MortarColor", new double[] {.8,1f,.2});
		ap.setAttribute("polygonShader.BrickSize", brickSize);
		ap.setAttribute("polygonShader.BrickPct", brickPct);
		ap.setAttribute("polygonShader.MortarPct", mortarPct);
		ap.setAttribute("polygonShader.LightPosition", lightPosition);
		
		JRViewer.display(world);
	}
}
