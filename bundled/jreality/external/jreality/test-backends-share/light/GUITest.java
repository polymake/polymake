package light;

import java.awt.Color;

import javax.swing.SwingConstants;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.Scene;
import de.jreality.scene.Appearance;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SpotLight;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;

public class GUITest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		SceneGraphComponent torus = new SceneGraphComponent();
		
		
		Appearance a = new Appearance();
		ImageData imageData = null;
		double scale = 1;
		// get the image for the texture first
		SimpleTextureFactory stf = new SimpleTextureFactory();
		stf.setColor(0, new Color(0,0,0,0));	// gap color in weave pattern is totally transparent
		stf.setColor(1, new Color(255,0,100));
		stf.setColor(2, new Color(255,255,0));
		stf.update();
		imageData = stf.getImageData();
		scale = 10;
		
		Texture2D tex = TextureUtility.createTexture(a, CommonAttributes.POLYGON_SHADER, imageData);
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(scale).getMatrix());
		torus.setAppearance(a);
		
		//defining the text of the label
		IndexedFaceSet fs = Primitives.texturedQuadrilateral();
		fs.setFaceAttributes(Attribute.LABELS, StorageModel.STRING_ARRAY.createReadOnly(new String[]{"I'm a button"}));
		
		//defining positioning of the label
		DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(a, false);
		dgs.setShowFaces(true);
		DefaultTextShader fts = (DefaultTextShader) ((DefaultPolygonShader)dgs.getPolygonShader()).getTextShader();
	    fts.setAlignment(SwingConstants.CENTER);
	    double[] offset = new double[]{0,0,0.1};
	    fts.setOffset(offset);
		torus.setGeometry(fs);
		torus.addTool(new GUITool());
		
		
		//setting up the viewer
		JRViewer vr = new JRViewer();
//		Scene scene = (Scene)(vr.getPlugin(Scene.class));
//		scene.getCameraComponent().addChild(torus);
		vr.setContent(torus);
		vr.addVRSupport();
//		vr.addBasicUI();
		vr.addContentUI();
		vr.startup();
	}

}
