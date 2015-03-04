package de.jreality.writer.blender.test;

import static de.jreality.scene.data.Attribute.COLORS;
import static de.jreality.scene.data.Attribute.COORDINATES;
import static de.jreality.scene.data.Attribute.INDICES;
import static de.jreality.scene.data.StorageModel.DOUBLE_ARRAY;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.OPAQUE_TUBES_AND_SPHERES;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.SMOOTH_SHADING;
import static de.jreality.shader.CommonAttributes.SPHERES_DRAW;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY_ENABLED;
import static de.jreality.shader.CommonAttributes.TUBES_DRAW;
import static de.jreality.shader.CommonAttributes.TUBE_RADIUS;
import static de.jreality.shader.CommonAttributes.Z_BUFFER_ENABLED;
import static java.lang.Math.PI;

import java.awt.Color;
import java.io.File;
import java.util.Random;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.geometry.PointSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.io.JrScene;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Cylinder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.RootAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.writer.blender.BlenderConnection;

public class BlenderTestScene {

	public static void main(String[] args) throws Exception {
		SceneGraphComponent root = new SceneGraphComponent();
		root.setName("Scene Root");
		Appearance rootAppearance = new Appearance("Root Appearance");
		rootAppearance.setAttribute(POLYGON_SHADER + "." + DIFFUSE_COLOR, new Color(0.8f, 0.4f, 0.6f));
		rootAppearance.setAttribute(LINE_SHADER + "." + DIFFUSE_COLOR, new Color(1.0f, 1.0f, 1.0f));
		rootAppearance.setName("Root Appearance");
		RootAppearance rootApp = ShaderUtility.createRootAppearance(rootAppearance);
		rootApp.setBackgroundColor(new Color(0.8f, 0.9f, 0.7f));
		root.setAppearance(rootAppearance);
		
		SceneGraphComponent icosahedron = new SceneGraphComponent();
		icosahedron.setName("Icosahedron Root");
		icosahedron.setGeometry(Primitives.icosahedron());
		Appearance icosahedronAppearance = new Appearance("Icosahedron Appearance");
		icosahedronAppearance.setAttribute(POLYGON_SHADER + "." + DIFFUSE_COLOR, new Color(0.3f, 0.9f, 0.8f));
		icosahedron.setAppearance(icosahedronAppearance);
		root.addChild(icosahedron);
		SceneGraphComponent icosahedron2 = new SceneGraphComponent();
		icosahedron2.setName("Icosahedron Transformed Root");
		icosahedron2.setGeometry(icosahedron.getGeometry());
		Appearance icosahedron2Appearance = new Appearance("Transformed Icosahedron Appearance");
		icosahedron2Appearance.setAttribute(POLYGON_SHADER + "." + DIFFUSE_COLOR, new Color(0.9f, 0.2f, 0.7f));
		icosahedron2.setAppearance(icosahedron2Appearance);
		MatrixBuilder.euclidean().translate(2, 2, 2).assignTo(icosahedron2);
		root.addChild(icosahedron2);
		SceneGraphComponent icosahedron3 = new SceneGraphComponent();
		icosahedron3.setName("Icosahedron 2 Root");
		SceneGraphComponent geometryChild = new SceneGraphComponent("parent");
		geometryChild.setGeometry(icosahedron.getGeometry());
		Appearance icosahedron3Appearance = new Appearance("Icosahedron 2 Appearance");
		icosahedron3Appearance.setAttribute(POLYGON_SHADER + "." + DIFFUSE_COLOR, new Color(0.5f, 0.3f, 0.9f));
		icosahedron3Appearance.setAttribute(POLYGON_SHADER + "." + TRANSPARENCY, 0.5);
		icosahedron3Appearance.setAttribute(TRANSPARENCY_ENABLED, true);
		icosahedron3Appearance.setAttribute(OPAQUE_TUBES_AND_SPHERES, true);
		icosahedron3Appearance.setAttribute(Z_BUFFER_ENABLED, true);
		icosahedron3.setAppearance(icosahedron3Appearance);
		icosahedron3.addChild(geometryChild);
		MatrixBuilder.euclidean().translate(-2, 2, 2).assignTo(icosahedron3);
		root.addChild(icosahedron3);		
		SceneGraphComponent cameraRoot = new SceneGraphComponent();
		cameraRoot.setName("Camera Root");
		Camera cam = new Camera("My Camera");
		cam.setFieldOfView(30);
		cam.setOrientationMatrix(MatrixBuilder.euclidean().translate(0, 3, 8).rotateX(-0.3).getArray());
		cameraRoot.setCamera(cam);
		root.addChild(cameraRoot);
		root.addChild(cameraRoot);
		SceneGraphComponent camera2Root = new SceneGraphComponent();
		camera2Root.setName("Orthographic Camera Root");
		Camera cam2 = new Camera("My Orthographic Camera");
		cam2.setOrientationMatrix(MatrixBuilder.euclidean().translate(-3, 1, 2).rotateX(-0.1).getArray());
		cam2.setPerspective(false);
		camera2Root.setCamera(cam2);
		root.addChild(camera2Root);
		SceneGraphComponent invisible = new SceneGraphComponent();
		invisible.setName("Invisible Object");
		invisible.setVisible(false);
		root.addChild(invisible);
		SceneGraphComponent visibilityInherited = new SceneGraphComponent();
		visibilityInherited.setName("Inherited Visibility");
		invisible.addChild(visibilityInherited);
		SceneGraphComponent transformedObject = new SceneGraphComponent();
		transformedObject.setName("Transformed Object");
		double[] pos = {1,2,3,1};
		MatrixBuilder mb = MatrixBuilder.euclidean();
		mb.rotate(Math.PI / 4, 1, 0, 0);
		Matrix M = mb.getMatrix();
		pos = M.multiplyVector(pos);
		mb.translate(pos);
		mb.assignTo(transformedObject);
		root.addChild(transformedObject);
		
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexCount(4);
		ifsf.setEdgeCount(2);
		ifsf.setFaceCount(1);
		ifsf.setVertexCoordinates(new double[][]{
			{0,0,1,10},
			{10,0,1,10},
			{10,10,1,10},
			{0,10,1,10}
		});
		ifsf.setEdgeIndices(new int[][]{
			{0,1,3},
			{2,3}
		});
		ifsf.setEdgeColors(new double[][]{
			{0.3,1.0,0.6}, {1.0,0.1,0.3}	
		});
		ifsf.setFaceIndices(new int[][]{
			{0,1,2,3}
		});
		ifsf.update();
		SceneGraphComponent geometryComponent1 = new SceneGraphComponent();
		geometryComponent1.setName("DoubleArrayArray Geometry Component");
		geometryComponent1.setGeometry(ifsf.getGeometry());
		MatrixBuilder.euclidean().translate(0, -3, 0).assignTo(geometryComponent1);
		Appearance tubesAndSpheresApp = new Appearance("Tubes and Spheres Appearance");
		tubesAndSpheresApp.setAttribute(POINT_SHADER + '.' + SPHERES_DRAW, true);
		tubesAndSpheresApp.setAttribute(POINT_SHADER + '.' + POINT_RADIUS, 0.1);
		tubesAndSpheresApp.setAttribute(LINE_SHADER + '.' + TUBES_DRAW, true);
		tubesAndSpheresApp.setAttribute(LINE_SHADER + '.' + TUBE_RADIUS, 0.05);
		geometryComponent1.setAppearance(tubesAndSpheresApp);
		root.addChild(geometryComponent1);
		
		IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
		ilsf.setVertexCount(100);
		ilsf.setEdgeCount(99);
		double[][] vertData = new double[100][];
		int[][] indexData = new int[vertData.length - 1][];
		for (int i = 0; i < vertData.length; i++) {
			vertData[i] = new double[]{i/10.0, Math.sin(2*PI*i/(double)vertData.length)};
			if (i < indexData.length) {
				indexData[i] = new int[]{i, i+1};
			}
		}
		ilsf.setVertexCoordinates(vertData);
		ilsf.setVertexColors(vertData);
		ilsf.setEdgeIndices(indexData);
		ilsf.update();
		SceneGraphComponent lineSetObject = new SceneGraphComponent();
		lineSetObject.setName("Line Set Geometry Component");
		lineSetObject.setGeometry(ilsf.getGeometry());
		root.addChild(lineSetObject);
		
		PointSetFactory psf = new PointSetFactory();
		psf.setVertexCount(1000);
		double[][] pointData = new double[1000][];
		double[] pointRadiusData = new double[1000];
		Random rnd = new Random(0);
		for (int i = 0; i < pointData.length; i++) {
			pointData[i] = new double[]{rnd.nextGaussian(), rnd.nextGaussian(), rnd.nextGaussian(), 1.0};
			pointRadiusData[i] = rnd.nextDouble() * 3;
		}
		psf.setVertexCoordinates(pointData);
		psf.setVertexRelativeRadii(pointRadiusData);
		psf.setVertexColors(pointData);
		psf.update();
		SceneGraphComponent pointSetRoot = new SceneGraphComponent();
		MatrixBuilder.euclidean().translate(-3, 0, 0).assignTo(pointSetRoot);
		pointSetRoot.setName("Point Set Component");
		pointSetRoot.setGeometry(psf.getGeometry());
		root.addChild(pointSetRoot);
		
		SceneGraphComponent customGeoemtryRoot = new SceneGraphComponent();
		customGeoemtryRoot.setName("Custom Geometry Component");
		MatrixBuilder.euclidean().translate(0, 0, -4).scale(0.5).assignTo(customGeoemtryRoot);
		root.addChild(customGeoemtryRoot);
		
		SceneGraphComponent pointLightRoot = new SceneGraphComponent();
		pointLightRoot.setName("Point Light Component");
		PointLight plight = new PointLight("Point Light");
		plight.setColor(Color.RED);
		pointLightRoot.setLight(plight);
		MatrixBuilder.euclidean().translate(-3, 5, 2).assignTo(pointLightRoot);
		root.addChild(pointLightRoot);
		
		SceneGraphComponent spotLightRoot = new SceneGraphComponent();
		spotLightRoot.setName("Spot Light Component");
		SpotLight spotlight = new SpotLight("Spot Light");
		spotlight.setColor(Color.GREEN);
		spotLightRoot.setLight(spotlight);
		MatrixBuilder.euclidean().translate(0, -2, 5).assignTo(spotLightRoot);
		root.addChild(spotLightRoot);
		
		SceneGraphComponent dirLightRoot = new SceneGraphComponent();
		dirLightRoot.setName("Directional Light Component");
		DirectionalLight dirLight = new DirectionalLight("Directional Light");
		dirLight.setColor(Color.WHITE);
		dirLightRoot.setLight(dirLight);
		MatrixBuilder.euclidean().translate(5, 0, 5).rotateY(Math.PI/4).assignTo(dirLightRoot);
		root.addChild(dirLightRoot);
		
		SceneGraphComponent colorsComponent = new SceneGraphComponent();
		colorsComponent.setName("Colors Smooth Component");
		double[][] vertexColors = new double[8][];
		for (int i = 0; i < vertexColors.length; i++) {
			vertexColors[i] = new double[]{rnd.nextDouble(), rnd.nextDouble(), rnd.nextDouble()};
		}
		double[][] faceColors = new double[6][];
		for (int i = 0; i < faceColors.length; i++) {
			faceColors[i] = new double[]{rnd.nextDouble(), rnd.nextDouble(), rnd.nextDouble()};
		}
		int[] faceIndices = {0, 2, 3, 1, 1, 5, 4, 0, 0, 4, 6, 2, 5, 7, 6, 4, 2, 6, 7, 3, 3, 7, 5, 1};
		double[][] vertexCoordinates = {{1.0, 1.0, 1.0}, {1.0, 1.0, -1.0}, {1.0, -1.0, 1.0}, {1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}, {-1.0, 1.0, -1.0}, {-1.0, -1.0, 1.0}, {-1.0, -1.0, -1.0}};
		IndexedFaceSet cube = Primitives.coloredCube();
		cube.setVertexAttributes(COLORS, DOUBLE_ARRAY.array(3).createReadOnly(vertexColors));
		colorsComponent.setGeometry(cube);
		MatrixBuilder.euclidean().translate(-2, 0, 1).scale(0.5).assignTo(colorsComponent);
		root.addChild(colorsComponent);
		
		SceneGraphComponent colorsFaceComponent = new SceneGraphComponent();
		Appearance flatShadingApp = new Appearance("Flat Shading Appearance");
		flatShadingApp.setAttribute(POLYGON_SHADER + "." + SMOOTH_SHADING, false);
		colorsFaceComponent.setAppearance(flatShadingApp);
		colorsFaceComponent.setName("Colors Flat Component");
		IndexedFaceSet cube2 = Primitives.coloredCube();
		cube2.setFaceAttributes(INDICES, null);
		cube2.setFaceAttributes(INDICES, new IntArrayArray.Inlined(faceIndices, 4));
		cube2.setVertexAttributes(COLORS, new DoubleArrayArray.Array(vertexColors));
		cube2.setVertexAttributes(COORDINATES, null);
		cube2.setVertexAttributes(COORDINATES, new DoubleArrayArray.Array(vertexCoordinates));
		cube2.setFaceAttributes(COLORS, null);
		cube2.setFaceAttributes(COLORS, new DoubleArrayArray.Array(faceColors));
		colorsFaceComponent.setGeometry(cube2);
		MatrixBuilder.euclidean().translate(2, 0, 1).scale(0.5).assignTo(colorsFaceComponent);
		root.addChild(colorsFaceComponent);
		
		SceneGraphComponent sphereAndCylinderComponent = new SceneGraphComponent("Sphere And Cylinder Component");
		MatrixBuilder.euclidean().translate(2, -2, 0).scale(0.5).assignTo(sphereAndCylinderComponent);
		SceneGraphComponent sphereComponent = new SceneGraphComponent("Sphere Component");
		SceneGraphComponent cylinderComponent = new SceneGraphComponent("Cylinder Component");
		sphereComponent.setGeometry(new Sphere());
		MatrixBuilder.euclidean().scale(0.5).assignTo(sphereComponent);
		cylinderComponent.setGeometry(new Cylinder());
		sphereAndCylinderComponent.addChild(sphereComponent);
		sphereAndCylinderComponent.addChild(cylinderComponent);
		root.addChild(sphereAndCylinderComponent);
		
		SceneGraphPath camPath = new SceneGraphPath(root, cameraRoot, cam);
		
//		Viewer v = JRViewer.display(root);
//		v.setCameraPath(camPath);
		
		JrScene scene = new JrScene(root);
		scene.addPath("cameraPath", camPath);
		
		// write scene file
//		WriterJRS jrsWriter = new WriterJRS();
//		jrsWriter.writeScene(scene, new FileOutputStream("test.jrs"));
		
		// write blender file 
		BlenderConnection r = new BlenderConnection();
		File blenderFile = new File("test.blend");
		r.writeFile(scene, blenderFile);
//		File imageFile = new File("test.png");
//		r.renderImage(scene, imageFile);
//		BufferedImage image = ImageIO.read(imageFile);
//		if (image != null) {
//			JFrame f = new JFrame();
//			f.add(new JLabel(new ImageIcon(image)));
//			f.pack();
//			f.setVisible(true);
//			f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
//		}
	}
	
}
