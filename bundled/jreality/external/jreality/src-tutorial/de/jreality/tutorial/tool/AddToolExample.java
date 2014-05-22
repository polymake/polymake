package de.jreality.tutorial.tool;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.ScaleTool;

public class AddToolExample {

	public static void main(String[] args) {
		SceneGraphComponent cmp = new SceneGraphComponent();
		Appearance ap = new Appearance();
		cmp.setAppearance(ap);
		setupAppearance(ap);
		ScaleTool tool= new ScaleTool();
		cmp.addTool(tool);		
		cmp.setGeometry(Primitives.icosahedron());
	    JRViewer.display(cmp);
	}
	private static void setupAppearance(Appearance ap) {
		DefaultGeometryShader dgs;
		DefaultPolygonShader dps;
		DefaultLineShader dls;
		DefaultPointShader dpts;
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(true);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.blue);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.03);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.05);
	}
}
