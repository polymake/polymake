package de.jreality.tutorial.app;

import java.io.IOException;

import de.jreality.geometry.SphereUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * An example using multiple OpenGL shading language shaders in a jReality scene graph.
 * @author Charles Gunn
 *
 */
public class GLShadingLangExample02 {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   		ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(true);
		dgs.setShowPoints(false);
		dgs.createPolygonShader("glsl");

		GlslProgram brickProg = null;		
		ap.setAttribute(CommonAttributes.EDGE_DRAW, false);
		for (int i = 0; i<6; ++i)	{
			double[] brickSize = new double[2];
			double[] brickPct = new double[2];
			double[] mortarPct = new double[2];
			double[] lightPosition = {0,0,4};
			SceneGraphComponent c = SceneGraphUtility.createFullSceneGraphComponent("sphere"+i);
			ap = c.getAppearance();
			c.setGeometry(SphereUtility.tessellatedIcosahedronSphere(3, true)); 
			double angle = (2 * Math.PI * i)/6.0;
			MatrixBuilder.euclidean().translate(3 * Math.cos(angle), 3*Math.sin(angle), 0.0).assignTo(c);
			float g = (float) (i/5.0);
			float r = (float) (i/6.0);
			float b = 1f - r;
			ap.setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, new java.awt.Color(g,g,g));
			brickSize[0] = .1+r*.5;
			brickSize[1] = .2+r*.3;
			brickPct[0] = .4 + .1*i;
			brickPct[1] = .7 + .05*i;
			mortarPct[0] = 1.0 - brickPct[0];
			mortarPct[1] = 1.0 - brickPct[1];
			lightPosition[0] = i-2.0;
			try {
				brickProg = new GlslProgram(ap, "polygonShader",   
						Input.getInput("de/jreality/jogl/shader/resources/brick.vert"),
						Input.getInput("de/jreality/jogl/shader/resources/brick.frag")
				    );
			} catch (IOException e) {
				e.printStackTrace();
			}
			brickProg.setUniform("SpecularContribution", (i*.15));
			brickProg.setUniform("DiffuseContribution", 1.0);
			brickProg.setUniform("BrickColor", new double[]{r,0f,b});
			brickProg.setUniform("MortarColor", new double[] {b,1f,r});
			brickProg.setUniform("BrickSize", brickSize);
			brickProg.setUniform("BrickPct", brickPct);
			brickProg.setUniform("MortarPct", mortarPct);
			brickProg.setUniform("LightPosition", lightPosition);			
			world.addChild(c);
		}
		JRViewer.display(world);
	}
}
