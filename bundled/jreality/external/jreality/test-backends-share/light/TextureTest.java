package light;

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.reader.ReaderOBJ;
import de.jreality.scene.Appearance;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SpotLight;
import de.jreality.scene.data.Attribute;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.util.Input;

public class TextureTest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		SceneGraphComponent cmp = new SceneGraphComponent();
		
		
		//a directional light
		DirectionalLight l1 = new DirectionalLight();
		l1.setGlobal(true);
		l1.setColor(Color.BLUE);
		l1.setIntensity(0.5);
		SceneGraphComponent light1 = new SceneGraphComponent();
		light1.setLight(l1);
		MatrixBuilder.euclidean().rotate(4, new double[]{1, 0, 0, 0}).assignTo(light1);
		cmp.addChild(light1);
		
		//a point light with attenuation
		PointLight l2 = new PointLight();
		l2.setGlobal(true);
		l2.setColor(Color.RED);
		l2.setIntensity(1.0);
		l2.setFalloff(0, 0, 1);
		SceneGraphComponent light2 = new SceneGraphComponent();
		light2.setLight(l2);
		MatrixBuilder.euclidean().translate(new double[]{2, 1, 2, 1}).assignTo(light2);
		cmp.addChild(light2);
		
		//a point light with attenuation
		SpotLight l3 = new SpotLight();
		l3.setGlobal(true);
		l3.setColor(Color.GREEN);
		l3.setIntensity(0.5);
		l3.setFalloff(1, 0, 0);
		l3.setDistribution(10);
		l3.setConeAngle(1.5);
		SceneGraphComponent light3 = new SceneGraphComponent();
		light3.setLight(l3);
		MatrixBuilder.euclidean().translate(new double[]{-2, 1, 2, 1}).rotate(1.5, new double[]{1, 0, 0}).assignTo(light3);
		cmp.addChild(light3);
		
		double r = Math.sqrt(2)*2;
		int n = 1;
		SpotLight[] lights = new SpotLight[n];
		for(int i = 0; i < lights.length; i++){
			double a = i*Math.PI*2/n;
			lights[i] = new SpotLight();
			lights[i].setGlobal(true);
			lights[i].setColor(Color.GREEN);
			lights[i].setIntensity(0.5);
			lights[i].setFalloff(1, 0, 0);
			lights[i].setDistribution(0);
			lights[i].setConeAngle(0.2);
			SceneGraphComponent light = new SceneGraphComponent();
			light.setLight(lights[i]);
			MatrixBuilder.euclidean().translate(new double[]{r*Math.sin(a), 1, r*Math.cos(a), 1}).rotate(1.5, new double[]{1, 0, 0}).assignTo(light);
			cmp.addChild(light);
		}
		
		SceneGraphComponent torus = new SceneGraphComponent();
		
		
		Appearance a = new Appearance();
		DefaultGeometryShader dgs = (DefaultGeometryShader) ShaderUtility.createDefaultGeometryShader(a, true);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
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
		dps.setDiffuseColor(Color.white);
		
//		Texture2D tex = TextureUtility.createTexture(a, CommonAttributes.POLYGON_SHADER, imageData);
//		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(scale).getMatrix());
//		tex.setApplyMode(Texture2D.GL_BLEND);
		BufferedImage imag = null;
		try {
		    imag = ImageIO.read(new File("/home/benjamin/Downloads/qube2.png"));
		} catch (IOException e) {
			
		}
		ImageData img = new ImageData(imag);
		TextureUtility.createTexture(a, CommonAttributes.POLYGON_SHADER, img);
		//TextureUtility.createT
		ReaderOBJ reader = new ReaderOBJ();
		try {
			torus = reader.read(new Input(new File("/home/benjamin/Downloads/qube2.obj")));
		} catch (Exception e) {
			
		}
		IndexedFaceSet ifs = (IndexedFaceSet) torus.getChildComponent(0).getGeometry();
		double[][] target = null;
		//target = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(target);
		target = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(target);
		System.out.println("writing array");
		for(int i = 0; i < target.length; i++){
			System.out.print("v ");
			for(int j = 0; j < target[i].length;j++)
				System.out.print("" + target[i][j] + " ");
			System.out.println("");
		}
		
		torus.setAppearance(a);
		light2.addChild(torus);
		
		JRViewer vr = new JRViewer();
		
		vr.setContent(cmp);
		vr.addVRSupport();
		vr.addBasicUI();
		vr.addContentUI();
		vr.startup();
	}

}
