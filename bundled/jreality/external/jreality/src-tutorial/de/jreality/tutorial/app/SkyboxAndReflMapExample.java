package de.jreality.tutorial.app;

import java.awt.Color;
import java.io.IOException;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.shader.CubeMap;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.TextureUtility;
import de.jreality.util.SceneGraphUtility;

/**
 * Shows how to add a skybox to a jReality scene, and also use it as the source for a reflection map
 * 
 * @author Charles Gunn
 *
 */
public class SkyboxAndReflMapExample {

	public static void main(String[] args)	{
		SceneGraphComponent worldSGC = SceneGraphUtility.createFullSceneGraphComponent("SkyboxExample");
		worldSGC.setGeometry(new CatenoidHelicoid(40));
		Appearance ap = worldSGC.getAppearance();
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(true);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		ap.setAttribute(
				"polygonShader.reflectionMap:blendColor",
				new Color(1f, 1f, 1f, .5f));
		// basic class needed for both reflection maps and skyboxes is a CubeMap
		// we load this over the net since 
		CubeMap rm = null;
		try {
			rm = TextureUtility.createReflectionMap(ap,"polygonShader",
			  	"http://www3.math.tu-berlin.de/jreality/download/data/reflectionMap/desert_",
			     new String[]{"rt","lf","up", "dn","bk","ft"},
			     ".jpg");
			TextureUtility.createReflectionMap(ap,"lineShader.polygonShader",
				  	"http://www3.math.tu-berlin.de/jreality/download/data/reflectionMap/desert_",
				     new String[]{"rt","lf","up", "dn","bk","ft"},
				     ".jpg");
		} catch (IOException e) {
			e.printStackTrace();
		}
		Viewer v = JRViewer.display(worldSGC);
		ImageData[] sides = TextureUtility.getCubeMapImages(rm);
		// attach a skybox to the scene root
		TextureUtility.createSkyBox(v.getSceneRoot().getAppearance(), sides);
	}
}
