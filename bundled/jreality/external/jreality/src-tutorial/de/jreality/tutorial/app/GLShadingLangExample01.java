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
public class GLShadingLangExample01 {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		dgs.createPolygonShader("glsl");
		GlslProgram brickProg = null;		
		world.setGeometry(SphereUtility.tessellatedIcosahedronSphere(3, true)); 
		try {
			brickProg = new GlslProgram(ap, "polygonShader",   
					Input.getInput("de/jreality/jogl/shader/resources/brick.vert"),
					Input.getInput("de/jreality/jogl/shader/resources/brick.frag")
			    );
		} catch (IOException e) {
			e.printStackTrace();
		}
		double[] brickSize = {.2, .25};
		double[] brickPct = {.5, .75};
		double[] mortarPct = new double[2];
		double[] lightPosition = {-2,0,4};
		mortarPct[0] = 1.0 - brickPct[0];
		mortarPct[1] = 1.0 - brickPct[1];
		brickProg.setUniform("SpecularContribution", .5);
		brickProg.setUniform("DiffuseContribution", 1.0);
		brickProg.setUniform("BrickColor", new double[]{.2,0f,.8});
		brickProg.setUniform("MortarColor", new double[] {.8,1f,.2});
		brickProg.setUniform("BrickSize", brickSize);
		brickProg.setUniform("BrickPct", brickPct);
		brickProg.setUniform("MortarPct", mortarPct);
		brickProg.setUniform("LightPosition", lightPosition);			
		JRViewer.display(world);
	}
}
