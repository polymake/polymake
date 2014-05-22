package light;
import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.Scene;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;


public class Test {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		JRViewer v = new JRViewer();
		v.addVRSupport();
		v.addContentUI();
		v.addBasicUI();
		v.addContentSupport(ContentType.Custom);
		Scene s = v.getPlugin(Scene.class);
		SceneGraphComponent sgc = new SceneGraphComponent("test");//Primitives.wireframeSphere();
		//PointSet ps = new PointSet();
		
		//PointSet ps = Primitives.point(new double[]{0,1,2});
		
		//MatrixBuilder.euclidean().rotate(Math.PI/2, new double[]{1,0,0}).scale(1000).assignTo(sgc);
		//s.getContentComponent().addChild(sgc);
		//s.getCameraComponent().addChild(sgc);
		//s.getAvatarComponent().addChild(sgc);
		
		
//		double[][] center = new double[2][3];
//		center[0] = new double[]{0, 1, 2};
//		center[1] = new double[]{3, 4, 5};
//		PointSet ps = new PointSet(2);
//		//int n = center.length;
//		double[][] pts = new double[2][3];
//		System.arraycopy(center[0],0,pts[0],0,3);
//		System.arraycopy(center[1],0,pts[1],0,3);
//		double[][] texc = {{0,0}, {0,0}};
//		ps.setVertexCountAndAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(pts));
//		ps.setVertexAttributes(Attribute.TEXTURE_COORDINATES,StorageModel.DOUBLE_ARRAY.array(2).createReadOnly(texc));
//		
		IndexedFaceSet ifs = Primitives.torus(5, 1, 20, 20);
//		IndexedFaceSet ifs = Primitives.torus(5, 1, 5, 5);

		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexAttributes(ifs.getVertexAttributes());
		ifsf.setFaceAttributes(ifs.getFaceAttributes());
		ifsf.setGenerateEdgesFromFaces(true);
		//ifsf.setGenerateVertexNormals(true);
		ifsf.update();
		
		sgc.setGeometry(ifsf.getIndexedFaceSet());
		
		Appearance a = new Appearance();
		String[] source = new String[2];
		source[0] = "polygon.v";
		source[1] = "polygonParam.f";
		a.setAttribute("polygonShader.glsl330-source", source);
		//TODO strange behaviour here results from not binding a default value
		//to certain uniform variables
		//a.set
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
		
		Texture2D tex = TextureUtility.createTexture(a, CommonAttributes.POLYGON_SHADER, imageData);
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(scale).getMatrix());
		
		sgc.setAppearance(a);
		
		//Appearance b = new Appearance();
		//a.setAttribute("nothing", true);
		//sgc.setAppearance(b);
		
		s.getContentComponent().addChild(sgc);
		//v.setContent(sgc);
		
		v.startup();
		//JOGLSceneGraph jsg = new JOGLSceneGraph(v.getPlugin(Scene.class).getSceneRoot());
		//jsg.createProxyTree();
	}

}
