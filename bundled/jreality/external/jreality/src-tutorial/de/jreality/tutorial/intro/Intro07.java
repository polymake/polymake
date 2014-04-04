package de.jreality.tutorial.intro;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D;

import java.awt.Color;
import java.io.IOException;
import java.net.URL;

import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.Input;
import de.jreality.util.SceneGraphUtility;

/**
 * This class contains the seventh in a series of 8 simple introductory examples which mimic the
 * functionality of the 
 * <a href="http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/User_Tutorial"> jReality User Tutorial 
 *</a>.  
 *
 * Same a {@link Intro06} but  with texture-mapped cylinder.
 * 
 * @author Charles Gunn
 *
 */
public class Intro07 {

	private static DefaultGeometryShader dgs;
	private static DefaultLineShader dls;
	private static DefaultPointShader dpts;
	private static RenderingHintsShader rhs;
	private static DefaultPolygonShader dps;
	private final static String	textureFileURL = "http://www3.math.tu-berlin.de/jreality/download/data/gridSmall.jpg";

	public static void main(String[] args)	{
			SceneGraphComponent myscene = SceneGraphUtility.createFullSceneGraphComponent("myscene");
			myscene.setGeometry(Primitives.cylinder(50));
			Appearance ap = myscene.getAppearance();
			dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
			dgs.setShowLines(false);
			dgs.setShowPoints(false);
			dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
			dps.setDiffuseColor(Color.white);
			// following code shows 2 different ways to create texture, one based on URL and 
			// the other based on file associated to the java package.
			// If the first fails, try the second.  
			Texture2D tex2d = null;
// following code mysteriously fails.  comment out until can be clarified
//			try {
//				tex2d = TextureUtility.createTexture(ap, POLYGON_SHADER,textureFileURL);
//			} catch (IOException e) {
//			}
			tex2d = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, 
					POLYGON_SHADER+"."+TEXTURE_2D,ap, true);
			URL is = Intro07.class.getResource("gridSmall.jpg");
			ImageData id = null;
			try {
				id = ImageData.load(new Input(is));
			} catch (IOException ee) {
				ee.printStackTrace();
			}
		    tex2d.setImage(id);					
			Matrix foo = new Matrix();
			MatrixBuilder.euclidean().scale(10, 10, 1).assignTo(foo);
			tex2d.setTextureMatrix(foo);
			
			JRViewer.display(myscene);
	}




}
