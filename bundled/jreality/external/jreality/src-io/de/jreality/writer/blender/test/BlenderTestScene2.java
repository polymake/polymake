package de.jreality.writer.blender.test;

import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.EDGE_DRAW;
import static de.jreality.shader.CommonAttributes.FACE_DRAW;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.RADII_WORLD_COORDINATES;
import static de.jreality.shader.CommonAttributes.SPHERES_DRAW;
import static de.jreality.shader.CommonAttributes.TUBES_DRAW;
import static de.jreality.shader.CommonAttributes.TUBE_RADIUS;
import static de.jreality.shader.CommonAttributes.VERTEX_DRAW;
import static de.jreality.writer.blender.BlenderAttributes.BLENDER_USE_SKINTUBES;
import static java.awt.Color.WHITE;
import static java.lang.Math.PI;

import java.awt.Color;
import java.awt.Image;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Random;

import javax.imageio.ImageIO;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.io.JrScene;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.reader.ReaderOBJ;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.writer.WriterJRS;
import de.jreality.writer.blender.BlenderConnection;

public class BlenderTestScene2 {

	public static void main(String[] args) throws IOException {
		SceneGraphComponent root = new SceneGraphComponent();
		MatrixBuilder.euclidean().scale(2).assignTo(root);
		Appearance rootApp = new Appearance("Root Appearance");
		rootApp.setAttribute(POINT_SHADER + '.' + RADII_WORLD_COORDINATES, true);
		rootApp.setAttribute(LINE_SHADER + '.' + RADII_WORLD_COORDINATES, true);
		rootApp.setAttribute(VERTEX_DRAW, false);
		rootApp.setAttribute(FACE_DRAW, false);
		root.setAppearance(rootApp);
		root.setName("Scene Root");
		
		SceneGraphComponent pointLightRoot = new SceneGraphComponent();
		pointLightRoot.setName("Point Light Component");
		PointLight plight = new PointLight("Point Light");
		plight.setColor(Color.WHITE);
		pointLightRoot.setLight(plight);
		MatrixBuilder.euclidean().translate(-3, 2, 5).assignTo(pointLightRoot);
		root.addChild(pointLightRoot);
		
		SceneGraphComponent cameraRoot = new SceneGraphComponent();
		cameraRoot.setName("Camera Root");
		Camera cam = new Camera("My Camera");
		cam.setOrientationMatrix(MatrixBuilder.euclidean().translate(0, 3, 8).rotateX(-0.3).getArray());
		cameraRoot.setCamera(cam);
		root.addChild(cameraRoot);
		
		SceneGraphComponent refTestRoot = new SceneGraphComponent("References Test Root");
		Appearance refApp = new Appearance("Referenced App");
		refTestRoot.setAppearance(refApp);
		refTestRoot.setGeometry(Primitives.coloredCube());
		root.addChild(refTestRoot);
		
		SceneGraphComponent refTestRoot2 = new SceneGraphComponent("References Test Root 2");
		Appearance refApp2 = new Appearance("Referenced App");
		refTestRoot2.setAppearance(refApp2);
		refTestRoot2.setGeometry(refTestRoot.getGeometry());
		root.addChild(refTestRoot2);
		
		SceneGraphComponent texturedQuad = new SceneGraphComponent("Textured Quad");
		Appearance texApp = new Appearance("Texture Appearance");
		texApp.setAttribute(POLYGON_SHADER + '.' + DIFFUSE_COLOR, WHITE);
		texApp.setAttribute(FACE_DRAW, true);
		texApp.setAttribute(EDGE_DRAW, true);
		texApp.setAttribute(VERTEX_DRAW, false);
		Image image = ImageIO.read(BlenderTestScene2.class.getResourceAsStream("texture01.jpg"));
		TextureUtility.createTexture(texApp, POLYGON_SHADER, new ImageData(image));
		texturedQuad.setAppearance(texApp);
		texturedQuad.setGeometry(Primitives.texturedQuadrilateral());
		MatrixBuilder.euclidean().translate(0, 0, 3).assignTo(texturedQuad);
		root.addChild(texturedQuad);
		
		SceneGraphComponent quad2 = new SceneGraphComponent("Textured Quad2");
		quad2.setGeometry(texturedQuad.getGeometry());
		quad2.setAppearance(texturedQuad.getAppearance());
		MatrixBuilder.euclidean().translate(-2, 0, 3).assignTo(quad2);
		root.addChild(quad2);
		
		
		SceneGraphComponent skinRoot = new SceneGraphComponent("Skin Component");
		MatrixBuilder.euclidean().scale(0.5).assignTo(skinRoot);
		root.addChild(skinRoot);
		
		SceneGraphComponent skinTubeComponent = new SceneGraphComponent("Skin Tubes Test");
		IndexedFaceSet polygon = Primitives.regularPolygon(100);
		IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(polygon);
		Random rnd = new Random();
		double[] radii = new double[100];
		for (int i = 0; i < radii.length; i++) {
			radii[i] = rnd.nextDouble();
		}
//		polygon.setVertexAttributes(Attribute.RELATIVE_RADII, new DoubleArray(radii));
		skinTubeComponent.setGeometry(polygon);
		Appearance skinApp = new Appearance("Skin Material");
		skinApp.setAttribute(VERTEX_DRAW, true);
		skinApp.setAttribute(POINT_SHADER + "." + SPHERES_DRAW, true);
		skinApp.setAttribute(POINT_SHADER + "." + POINT_RADIUS, 0.05);
		skinApp.setAttribute(EDGE_DRAW, true);
		skinApp.setAttribute(LINE_SHADER + "." + TUBES_DRAW, true);
		skinApp.setAttribute(LINE_SHADER + "." + DIFFUSE_COLOR, new Color(0.1f, 0.6f, 0.2f));
		skinApp.setAttribute(LINE_SHADER + "." + BLENDER_USE_SKINTUBES, true);
		skinApp.setAttribute(LINE_SHADER + "." + TUBE_RADIUS,	0.05);
		skinApp.setAttribute(FACE_DRAW, false);
		skinTubeComponent.setAppearance(skinApp);
		skinRoot.addChild(skinTubeComponent);

		
		SceneGraphComponent skinTubeComponent2 = new SceneGraphComponent("Skin Tubes Test 2");
		skinTubeComponent2.setAppearance(skinApp);
		skinTubeComponent2.setGeometry(polygon);
		MatrixBuilder.euclidean().translate(2, 0, 0).assignTo(skinTubeComponent2);
		skinRoot.addChild(skinTubeComponent2);
		
		ReaderOBJ robj = new ReaderOBJ();
		SceneGraphComponent torusRoot = robj.read(BlenderTestScene2.class.getResource("torus.obj"));
		torusRoot.setName("Textured Torus");
		MatrixBuilder.euclidean().translate(-2, -1, 0).scale(0.1).assignTo(torusRoot);
		Appearance texApp2 = new Appearance("Quad Texture Appearance");
		texApp2.setAttribute(POLYGON_SHADER + '.' + DIFFUSE_COLOR, WHITE);
		texApp2.setAttribute(FACE_DRAW, true);
		texApp2.setAttribute(EDGE_DRAW, false);
		texApp2.setAttribute(VERTEX_DRAW, false);
		Image imageQuads = ImageIO.read(BlenderTestScene2.class.getResource("quads01.png"));
		TextureUtility.createTexture(texApp2, POLYGON_SHADER, new ImageData(imageQuads));
		texApp2.setAttribute("polygonShader.texture2d:textureMatrix", new Matrix(new double[]{
			16.1100, 0.00000, 0.00000, 1.17603,
			0.00000, 16.1900, 0.00000, 0.129520,
			0.00000, 0.00000, 1.00000, 0.00000,
			0.00000, 0.00000, 0.00000, 1.00000
		}));
		torusRoot.setAppearance(texApp2);
		root.addChild(torusRoot);
		
		robj = new ReaderOBJ();
		SceneGraphComponent catRoot = robj.read(BlenderTestScene2.class.getResource("cathead_conformal.obj"));
		catRoot.setName("Homogeneously Textured Cat");
		catRoot.setAppearance(texApp2);
		MatrixBuilder.euclidean().rotate(-PI/2, 1, 0, 0).translate(2, -1, 0).scale(1).assignTo(catRoot);
		root.addChild(catRoot);
		
		SceneGraphPath camPath = new SceneGraphPath(root, cameraRoot, cam);
		
		JRViewer v = new JRViewer();
		v.addPythonSupport();
		v.addContentUI();
		v.addBasicUI();
		v.setContent(root);
//		v.startup();
		
		JrScene scene = new JrScene(root);
		scene.addPath("cameraPath", camPath);

		WriterJRS jrsWriter = new WriterJRS();
		jrsWriter.writeScene(scene, new FileOutputStream("test2.jrs"));
		
		// write blender file 
		BlenderConnection r = new BlenderConnection();
		File blenderFile = new File("test2.blend");
		r.writeFile(scene, blenderFile);
	}
	
}
