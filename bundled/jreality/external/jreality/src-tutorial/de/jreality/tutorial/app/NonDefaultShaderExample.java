package de.jreality.tutorial.app;

import java.awt.Color;

import de.jreality.geometry.SphereUtility;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImplodePolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.TwoSidePolygonShader;
import de.jreality.util.SceneGraphUtility;

public class NonDefaultShaderExample {

	public static void main(String[] args)	{
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		world.setGeometry(SphereUtility.sphericalPatch(0, 0.0, 180.0, 90.0, 40, 40, 1.0));
		Appearance ap = world.getAppearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) 
   			ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(true);
		dgs.setShowPoints(false);
		// A non-default shader has to be set using the following method call
		TwoSidePolygonShader tsps = (TwoSidePolygonShader) dgs.createPolygonShader("twoSide");
		// We use another non-default shader for the front surface of the two-sided shader
		ImplodePolygonShader dps = (ImplodePolygonShader) tsps.createFront("implode");
		DefaultPolygonShader dps2 = (DefaultPolygonShader) tsps.createBack("default");
		// but the attributes can be set directly using the Appearance.setAttribute() method
		ap.setAttribute("polygonShader.implodeFactor", .5);
		dps.setDiffuseColor(new Color(0,204,204));
		dps2.setDiffuseColor(new Color(204,204,0));
		DefaultLineShader dls = (DefaultLineShader) dgs.getLineShader();
		dls.setTubeDraw(true);
		dls.setTubeRadius(.005);
		dls.setDiffuseColor(Color.red);

		JRViewer.display(world);
	}

}
