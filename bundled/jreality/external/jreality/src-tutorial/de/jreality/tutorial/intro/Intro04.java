package de.jreality.tutorial.intro;

import java.awt.Color;
import java.io.IOException;
import java.net.URL;

import de.jreality.plugin.JRViewer;
import de.jreality.reader.Readers;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.PickShowTool;
import de.jreality.util.Input;

/**
 * This class contains the fourth in a series of 8 simple introductory examples which mimic the
 * functionality of the 
 * <a href="http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/User_Tutorial"> jReality User Tutorial 
 *</a>.  
 *
 * Same as {@link Intro02} but material properties changed using shader interfaces.
 * 
 * @author Charles Gunn
 *
 */
public class Intro04 {

	private static DefaultGeometryShader dgs;
	private static DefaultLineShader dls;
	private static DefaultPointShader dpts;
	private static RenderingHintsShader rhs;
	private static DefaultPolygonShader dps;

	public static void main(String[] args)	{
		SceneGraphComponent dodecSGC = readDodec();
		dodecSGC.addTool(new PickShowTool());
		Appearance ap = dodecSGC.getAppearance();
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(true);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.05);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.1);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setSmoothShading(false);
		rhs = ShaderUtility.createDefaultRenderingHintsShader(ap, true);
		rhs.setTransparencyEnabled(true);
		rhs.setOpaqueTubesAndSpheres(true);
		dps.setTransparency(.5);
		JRViewer.display(dodecSGC);
	}

	private static SceneGraphComponent readDodec() {
		URL url = Intro04.class.getResource("dodec.off");
		SceneGraphComponent scp = null;
		try {
			scp = Readers.read(Input.getInput(url));
// alternative to access the file as a URL
//			scp = Readers.read(Input.getInput("http://www3.math.tu-berlin.de/jreality/download/data/dodec.off"));
			scp.setName("Dodecahedron");
		} catch (IOException e) {
			e.printStackTrace();
		}
		return scp;
	}

}
