package de.jreality.writer.u3d;

import static de.jreality.math.MatrixBuilder.euclidean;
import static de.jreality.scene.Appearance.INHERITED;
import static de.jreality.scene.data.Attribute.COLORS;
import static de.jreality.scene.data.Attribute.INDICES;
import static de.jreality.scene.data.Attribute.NORMALS;
import static de.jreality.scene.data.AttributeEntityUtility.createAttributeEntity;
import static de.jreality.shader.CommonAttributes.AMBIENT_COEFFICIENT;
import static de.jreality.shader.CommonAttributes.AMBIENT_COLOR;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COEFFICIENT;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR_DEFAULT;
import static de.jreality.shader.CommonAttributes.FACE_DRAW;
import static de.jreality.shader.CommonAttributes.FACE_DRAW_DEFAULT;
import static de.jreality.shader.CommonAttributes.LIGHTING_ENABLED;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.SKY_BOX;
import static de.jreality.shader.CommonAttributes.SMOOTH_SHADING;
import static de.jreality.shader.CommonAttributes.SPECULAR_COEFFICIENT;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY_ENABLED;
import static de.jreality.shader.TextureUtility.createTexture;
import static java.awt.Color.WHITE;
import static java.awt.image.BufferedImage.TYPE_INT_ARGB;
import static java.lang.Double.MAX_VALUE;
import static java.lang.Math.PI;

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.awt.image.WritableRaster;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.imageio.ImageIO;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.geometry.PointSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.geometry.SphereUtility;
import de.jreality.io.JrScene;
import de.jreality.math.Pn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.CubeMap;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.CameraUtility;
import de.jreality.util.Rectangle3D;
import de.jreality.writer.u3d.texture.SphereMapGenerator;

public class U3DSceneUtility {

	static final Geometry 
		POINT_SPHERE = SphereUtility.tessellatedIcosahedronSphere(1),
		LINE_CYLINDER = new U3DClosedCylinder(8, 1.0);
	
	private static final IndexedFaceSet 
		SPHERE = SphereUtility.tessellatedIcosahedronSphere(2, true),
		CYLINDER = Primitives.cylinder(60);
	
	
	public static HashMap<SceneGraphComponent, Collection<SceneGraphComponent>> getParentsMap(Collection<SceneGraphComponent> l) {
		HashMap<SceneGraphComponent, Collection<SceneGraphComponent>> r = new HashMap<SceneGraphComponent, Collection<SceneGraphComponent>>();
		for (SceneGraphComponent c : l)
			r.put(c, new LinkedList<SceneGraphComponent>());
		for (SceneGraphComponent c : l) {
			for (int i = 0; i < c.getChildComponentCount(); i++) {
				SceneGraphComponent child = c.getChildComponent(i);
				Collection<SceneGraphComponent> parents = r.get(child);
				parents.add(c);
			}
		}
		return r;
	}
	
	
	
	private static List<SceneGraphComponent> getFlatScene_R(SceneGraphComponent root) {
		LinkedList<SceneGraphComponent> r = new LinkedList<SceneGraphComponent>();
		r.add(root);
		for (int i = 0; i < root.getChildComponentCount(); i++)
			r.addAll(getFlatScene_R(root.getChildComponent(i)));
		HashSet<SceneGraphComponent> uniqueSet = new HashSet<SceneGraphComponent>(r);
		return new LinkedList<SceneGraphComponent>(uniqueSet);
	}	
	
	
	public static List<SceneGraphComponent> getSceneGraphComponents(JrScene scene) {
		return getFlatScene_R(scene.getSceneRoot());
	}
	
	
	public static <
		T extends SceneGraphNode
	> HashMap<T, String> getUniqueNames(Collection<T> l) 
	{
		HashMap<String, List<T>> nameMap = new HashMap<String, List<T>>();
		HashMap<T, String> r = new HashMap<T, String>();
		for (T c : l) {
			if (c == null) continue;
			String name = c.getName();
			List<T> cList = nameMap.get(c.getName());
			if (cList == null) {
				cList = new LinkedList<T>();
				nameMap.put(name, cList);
			}
			cList.add(c);
		}
		for (String name : nameMap.keySet()) {
			List<T> nodes = nameMap.get(name);
			if (nodes.size() > 1) {
				int index = 1;
				DecimalFormat df = new DecimalFormat("000");
				for (T c : nodes) {
					String newName = name + df.format(index);
					r.put(c, newName);
					index++;
				}
			} else {
				r.put(nodes.get(0), name);
			}
		}
		return r;
	}
	
	private static List<SceneGraphComponent> getViewNodes_R(SceneGraphComponent root) {
		LinkedList<SceneGraphComponent> r = new LinkedList<SceneGraphComponent>();
		if (root.getCamera() != null)
			r.add(root);
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			List<SceneGraphComponent> subList = getViewNodes_R(root.getChildComponent(i));
			if (subList.size() != 0)
				r.addAll(subList);
		}
		HashSet<SceneGraphComponent> uniqueSet = new HashSet<SceneGraphComponent>(r);
		return new LinkedList<SceneGraphComponent>(uniqueSet);
	}	
	
	
	public static List<SceneGraphComponent> getViewNodes(JrScene scene) {
		return getViewNodes_R(scene.getSceneRoot());
	}
	
	
	private static List<SceneGraphComponent> getLightNodes_R(SceneGraphComponent root) {
		LinkedList<SceneGraphComponent> r = new LinkedList<SceneGraphComponent>();
		if (root.getLight() != null)
			r.add(root);
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			List<SceneGraphComponent> subList = getLightNodes_R(root.getChildComponent(i));
			if (subList.size() != 0)
				r.addAll(subList);
		}
		HashSet<SceneGraphComponent> uniqueSet = new HashSet<SceneGraphComponent>(r);
		return new LinkedList<SceneGraphComponent>(uniqueSet);
	}	
	
	
	public static List<SceneGraphComponent> getLightNodes(JrScene scene) {
		return getLightNodes_R(scene.getSceneRoot());
	}
	
	
	public static void printComponents(Collection<SceneGraphComponent> l) {
		System.out.println("SceneGraphComponents -------------------");
		for (SceneGraphComponent c : l) {
			System.out.println(c.getName());
		}
		System.out.println("----------------------------------------");
	}
	
	
	public static <
		T extends SceneGraphNode
	> void printNameMap(HashMap<T, String> map) {
		System.out.println("Names ----------------------------------");
		for (SceneGraphNode c : map.keySet()) {
			System.out.println(c.getName() + " -> " + map.get(c));
		}
		System.out.println("----------------------------------------");
	}
	
	public static void printNodes(String title, Collection<? extends SceneGraphNode> l) {
		System.out.println(title + " -----------------------------");
		for (SceneGraphNode g : l) {
			System.out.println(g.getName());
		}
		System.out.println("----------------------------------------");
	}
	
	public static void printTextures(Collection<U3DTexture> l) {
		System.out.println("Textures -----------------------------");
		for (U3DTexture g : l) {
			System.out.println(g.getImage());
		}
		System.out.println("----------------------------------------");
	}
	
	public static <
		T extends EffectiveAppearance
	> void printAppearanceNameMap(HashMap<T, String> map) {
		System.out.println("Material Names ----------------------------------");
		for (EffectiveAppearance c : map.keySet()) {
			System.out.println(c + " -> " + map.get(c));
		}
		System.out.println("----------------------------------------");
	}
	
	public static <
		T extends U3DTexture
	> void printTextureNameMap(HashMap<T, String> map) {
		System.out.println("Texture Names ----------------------------------");
		for (U3DTexture c : map.keySet()) {
			System.out.println(c.getImage() + " -> " + map.get(c));
		}
		System.out.println("----------------------------------------");
	}
	

	private static Map<Geometry, SceneGraphComponent> getGeometries_R(SceneGraphComponent root, SceneGraphPath p) {
		p.push(root);
		Map<Geometry, SceneGraphComponent> r = new HashMap<Geometry, SceneGraphComponent>();
		if (root.getGeometry() != null) {
			Geometry g = root.getGeometry();
			if (g instanceof PointSet) { // remove vertex normals if flat shading
				PointSet pSet = (PointSet)g;
				if (pSet.getVertexAttributes().containsAttribute(Attribute.NORMALS)) {
					EffectiveAppearance ea = EffectiveAppearance.create(p);
					DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(ea);
					DefaultPolygonShader ps = (DefaultPolygonShader)dgs.getPolygonShader();
					if (!ps.getSmoothShading()) {
						pSet.setVertexAttributes(Attribute.NORMALS, null);
					}
					
				}
			}
			r.put(root.getGeometry(), root);
		}
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			Map<Geometry, SceneGraphComponent> subList = getGeometries_R(root.getChildComponent(i), p);
			if (subList.size() != 0) {
				r.putAll(subList);
			}
		}
		p.pop();
		return r;
	}	
	
	
	/**
	 * TODO
	 * Returns a map that contains data that is used to build a workaround
	 * for the texture matrix bug in adobe reader. 
	 * @param scene
	 * @return
	 */
	public static Map<Geometry, SceneGraphComponent> getGeometries(JrScene scene) {
		SceneGraphPath p = new SceneGraphPath();
		return getGeometries_R(scene.getSceneRoot(), p);
	}
	
	
	private static List<Camera> getCameras_R(SceneGraphComponent root) {
		LinkedList<Camera> r = new LinkedList<Camera>();
		if (root.getCamera() != null)
			r.add(root.getCamera());
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			List<Camera> subList = getCameras_R(root.getChildComponent(i));
			if (subList.size() != 0)
				r.addAll(subList);
		}
		HashSet<Camera> uniqueSet = new HashSet<Camera>(r);
		return new LinkedList<Camera>(uniqueSet);
	}	
	
	
	public static List<Camera> getCameras(JrScene scene) {
		return getCameras_R(scene.getSceneRoot());
	}
	
	
	private static List<Light> getLights_R(SceneGraphComponent root) {
		LinkedList<Light> r = new LinkedList<Light>();
		if (root.getLight() != null)
			r.add(root.getLight());
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			List<Light> subList = getLights_R(root.getChildComponent(i));
			if (subList.size() != 0)
				r.addAll(subList);
		}
		HashSet<Light> uniqueSet = new HashSet<Light>(r);
		return new LinkedList<Light>(uniqueSet);
	}	
	
	
	public static List<Light> getLights(JrScene scene) {
		return getLights_R(scene.getSceneRoot());
	}
	
	
	
	public static IndexedFaceSet prepareFaceSet(IndexedFaceSet ifs) {
		IntArrayArray fData = (IntArrayArray)ifs.getFaceAttributes(INDICES);
		DoubleArrayArray fcData = (DoubleArrayArray)ifs.getFaceAttributes(COLORS);
		if (fData == null) return ifs;
		int[][] faces = fData.toIntArrayArray(null);
		boolean needsTreatment = false;
		int numFaces = 0; // count faces
		for (int i = 0; i < faces.length; i++) {
			if (faces[i].length != 3) {
				needsTreatment = true;
				numFaces += faces[i].length - 2;
			} else {
				numFaces++;
			}
		}
		if (!needsTreatment)
			return ifs;
		IndexedFaceSet rifs = new IndexedFaceSet();
		int[][] newFaceData = new int[numFaces][];
		double[][] fColors = null;
		double[][] newFaceColors = null;
		if (fcData != null) {
			 fColors = fcData.toDoubleArrayArray(null);
			 newFaceColors = new double[numFaces][];
		}
		int j = 0;
		int faceIndex = 0;
		for (int[] f : faces) {
			if (f.length != 3){
				int v = f.length;
				for (int k = 0; k < v / 2 - 1; k++){
					newFaceData[j++] = new int[]{ f[k], f[k+1], f[v - 1 - k]};
					newFaceData[j++] = new int[]{ f[k+1], f[v - 2 - k], f[v - 1 - k]};
					if (newFaceColors != null && fColors != null) {
						newFaceColors[j - 1] = fColors[faceIndex];
						newFaceColors[j - 2] = fColors[faceIndex];
					}
				}
				if (v % 2 != 0) {
					int k = v / 2 - 1;
					newFaceData[j++] = new int[]{ f[k], f[k+1], f[k+2]};
					if (newFaceColors != null && fColors != null) {
						newFaceColors[j - 1] = fColors[faceIndex];
					}
				}
			} else {
				newFaceData[j++] = f;
				if (newFaceColors != null && fColors != null) {
					newFaceColors[j - 1] = fColors[faceIndex];
				}
			}
			faceIndex++;
		}
		rifs.setVertexCountAndAttributes(ifs.getVertexAttributes());
		rifs.setFaceCountAndAttributes(INDICES, new IntArrayArray.Array(newFaceData).readOnlyList());
		if (fColors != null) {
			rifs.setFaceAttributes(COLORS, new DoubleArrayArray.Array(newFaceColors).readOnlyList());
		}
		if (ifs.getFaceAttributes(NORMALS) != null) {
			IndexedFaceSetUtility.calculateAndSetFaceNormals(rifs);
		}
		return rifs;
	}
	

	//TODO: write label preparation
	static void prepareLabels(SceneGraphComponent root) {
		SceneGraphComponent dummy = new SceneGraphComponent();
		dummy.addChild(root);
		dummy.childrenWriteAccept(new SceneGraphVisitor() {
			SceneGraphPath p = new SceneGraphPath();
			@Override
			public void visit(SceneGraphComponent c) {
				p.push(c);
				Geometry g = c.getGeometry();
				if (g != null && g instanceof PointSet) {
					PointSet ps = (PointSet)g;
					DataList labelList = ps.getVertexAttributes(Attribute.LABELS);
					if (labelList != null) {
//						SceneGraphComponent LabelUtility.sceneGraphForLabel(sgc, xscale, yscale, offset, alignment, camToObj, position)
					}
				}
				
				c.childrenWriteAccept(this, false, false, false, false, false, true);
				p.pop();
			}
		}, false, false, false, false, false, true);
		
	}
	
	
	public static void prepareTubesAndSpheres(SceneGraphComponent root) {
		SceneGraphComponent dummy = new SceneGraphComponent();
		dummy.addChild(root);
		dummy.childrenWriteAccept(new SceneGraphVisitor() {
			SceneGraphPath p = new SceneGraphPath();
			@Override
			public void visit(SceneGraphComponent c) {
				p.push(c);
				SceneGraphComponent basPoints=null;
				SceneGraphComponent basLines=null;
				Geometry g = c.getGeometry();
				if (g != null && g instanceof PointSet){
					EffectiveAppearance ea = EffectiveAppearance.create(p);
					Appearance ballsAndSticksAppearance = new Appearance();
					ea.create(ballsAndSticksAppearance);
					DefaultGeometryShader dgs = ShaderUtility.createDefaultGeometryShader(ea);
					DefaultPointShader dps = (DefaultPointShader) dgs.getPointShader();
					DefaultLineShader dls = (DefaultLineShader) dgs.getLineShader();
					IndexedLineSet ils = null;
					if (g instanceof IndexedLineSet) {
						ils = (IndexedLineSet) g;
					} else {
						ils = new IndexedLineSet();
						ils.setVertexCountAndAttributes(((PointSet)g).getVertexAttributes());
					}
					if (dgs.getShowPoints()) { // create spheres for point set 
						if (dps.getSpheresDraw()) {
							Color difColor = (Color)ea.getAttribute(POINT_SHADER + "." + DIFFUSE_COLOR, DIFFUSE_COLOR_DEFAULT);
							BallAndStickFactory bsf = new BallAndStickFactory(ils);
							bsf.setBallGeometry(POINT_SPHERE);
							bsf.setBallColor(difColor);
							double sphereSizeFactor = 1.0;
							if (dps.getRadiiWorldCoordinates() != null && dps.getRadiiWorldCoordinates())	{
								double[] o2w = p.getMatrix(null);
								sphereSizeFactor = CameraUtility.getScalingFactor(o2w, Pn.EUCLIDEAN);
								sphereSizeFactor = 1.0 / sphereSizeFactor;
							}
							bsf.setBallRadius(dps.getPointRadius() * sphereSizeFactor);
							bsf.setShowBalls(true);
							bsf.setShowSticks(false);
							bsf.update();
							basPoints = bsf.getSceneGraphComponent();
							basPoints.setOwner("foo");
							basPoints.setName("Spheres");
							Appearance app = basPoints.getAppearance();
							if (app == null) {
								app = new Appearance();
								basPoints.setAppearance(app);
							}
							app.setAttribute(FACE_DRAW, true);
							app.setAttribute(POLYGON_SHADER + "." + SMOOTH_SHADING, true);
							app.setAttribute(POLYGON_SHADER + "." + TRANSPARENCY_ENABLED, false);
							if (TextureUtility.hasReflectionMap(ea, "pointShader.polygonShader")) {
								CubeMap cm = TextureUtility.readReflectionMap(ea, "pointShader.polygonShader.reflectionMap");
								CubeMap cmDest = TextureUtility.createReflectionMap(app, "polygonShader", 
										cm.getBack(),
										cm.getFront(),
										cm.getBottom(),
										cm.getTop(),
										cm.getLeft(),
										cm.getRight()
										);
								cmDest.setBlendColor(cm.getBlendColor());
							} else {
								app.setAttribute("polygonShader.reflectionMap", Appearance.DEFAULT);
							}
						} else {
							PointSet ps = (PointSet)g;
							PointSetFactory psf = new PointSetFactory();
							psf.setVertexCount(ps.getNumPoints());
							psf.setVertexAttributes(ps.getVertexAttributes());
							psf.update();
							basPoints = new SceneGraphComponent();
							basPoints.setOwner("foo");
							basPoints.setName("Points");
							basPoints.setGeometry(psf.getGeometry());
							Appearance app = basPoints.getAppearance();
							if (app == null) {
								app = new Appearance();
								basPoints.setAppearance(app);
							}
						}
					}
					if (g instanceof IndexedLineSet && dgs.getShowLines()) { // create sticks for line set
						if (dls.getTubeDraw()) {
							Color difColor = (Color)ea.getAttribute("lineShader.polygonShader." + DIFFUSE_COLOR, DIFFUSE_COLOR_DEFAULT);
							BallAndStickFactory bsf = new BallAndStickFactory(ils);
							bsf.setStickGeometry(LINE_CYLINDER);
							bsf.setStickColor(difColor);
							double tubeSizeFactor = 1.0;
							if (dls.getRadiiWorldCoordinates() != null && dls.getRadiiWorldCoordinates())	{
								double[] o2w = p.getMatrix(null);
								tubeSizeFactor = CameraUtility.getScalingFactor(o2w, Pn.EUCLIDEAN);
								tubeSizeFactor = 1.0 / tubeSizeFactor;
							}
							bsf.setStickRadius(dls.getTubeRadius() * tubeSizeFactor);
							bsf.setShowBalls(false);
							bsf.setShowSticks(true);
							bsf.update();
							basLines = bsf.getSceneGraphComponent();
							basLines.setOwner("foo");
							basLines.setName("Tubes");
							Appearance app = basLines.getAppearance();
							if (app == null) {
								app = new Appearance();
								basLines.setAppearance(app);
							}
							app.setAttribute(FACE_DRAW, true);
							app.setAttribute(POLYGON_SHADER + "." + SMOOTH_SHADING, true);
							app.setAttribute(POLYGON_SHADER + "." + TRANSPARENCY_ENABLED, false);
							if (TextureUtility.hasReflectionMap(ea, "lineShader.polygonShader")) {
								CubeMap cm = TextureUtility.readReflectionMap(ea, "lineShader.polygonShader.reflectionMap");
								CubeMap cmDest = TextureUtility.createReflectionMap(app, "polygonShader", 
										cm.getBack(),
										cm.getFront(),
										cm.getBottom(),
										cm.getTop(),
										cm.getLeft(),
										cm.getRight()
										);
								cmDest.setBlendColor(cm.getBlendColor());
							} else {
								app.setAttribute("polygonShader.reflectionMap", Appearance.DEFAULT);
							}
						} else {
							IndexedLineSet ls = (IndexedLineSet)g;
							IndexedLineSetFactory lsf = new IndexedLineSetFactory();
							lsf.setVertexCount(ls.getNumPoints());
							lsf.setVertexAttributes(ls.getVertexAttributes());
							lsf.setEdgeCount(ls.getNumEdges());
							lsf.setEdgeIndices(ls.getEdgeAttributes(Attribute.INDICES));
							lsf.update();
							basLines = new SceneGraphComponent();
							basLines.setOwner("foo");
							basLines.setName("Lines");
							basLines.setGeometry(lsf.getGeometry());
							Appearance app = basLines.getAppearance();
							if (app == null) {
								app = new Appearance();
								basLines.setAppearance(app);
							}
						}
					}
					
				}
				c.childrenWriteAccept(this, false, false, false, false, false, true);
				if (basPoints != null) { 
//					basPoints.getAppearance().setAttribute("U3D_ForceVisible", true);
					c.addChild(basPoints);
					if (!(g instanceof IndexedFaceSet)) {
						c.setGeometry(null);
					}
				}
				if (basLines != null) { 
//					basLines.getAppearance().setAttribute("U3D_ForceVisible", true);
					c.addChild(basLines);
					if (!(g instanceof IndexedFaceSet)) {
						c.setGeometry(null);
					}
				}
				p.pop();
			}
		}, false, false, false, false, false, true);
	}
	
	
	public static HashMap<Geometry, Geometry> prepareGeometries(Collection<Geometry> geometry) {
		HashMap<Geometry, Geometry> r = new HashMap<Geometry, Geometry>();
		for (Geometry g : geometry) {
			if (g instanceof IndexedFaceSet) {
				IndexedFaceSet p = prepareFaceSet((IndexedFaceSet)g);
				r.put(g, p);
			}
			else if (g instanceof IndexedLineSet)
				r.put(g, g);
			else if (g instanceof PointSet)
				r.put(g, g);
			else if (g instanceof Sphere)
				r.put(g, SPHERE);
			else if (g instanceof Cylinder) {
				r.put(g, CYLINDER);
			}
		}
		return r;
	}
	
	
	private static void fillAppearanceMap_R(SceneGraphComponent root, HashMap<SceneGraphComponent, EffectiveAppearance> map) {
		EffectiveAppearance ea = map.get(root);
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			SceneGraphComponent child = root.getChildComponent(i);
			Appearance app = child.getAppearance();
			if (app != null) {
				EffectiveAppearance childEa = ea.create(child.getAppearance());
				map.put(child, childEa);
			} else {
				map.put(child, ea);
			}
			fillAppearanceMap_R(child, map);
		}
	}
	
	
	public static HashMap<SceneGraphComponent, EffectiveAppearance> getAppearanceMap(JrScene scene) {
		HashMap<SceneGraphComponent, EffectiveAppearance> map = new HashMap<SceneGraphComponent, EffectiveAppearance>();
		SceneGraphComponent root = scene.getSceneRoot();
		EffectiveAppearance ea = EffectiveAppearance.create();
		Appearance app = root.getAppearance();
		if (app != null) {
			EffectiveAppearance rootEa = ea.create(root.getAppearance());
			map.put(root, rootEa);
		} else {
			map.put(root, ea);
		}
		fillAppearanceMap_R(root, map);
		return map;
	}
	
	
	public static HashMap<EffectiveAppearance, String> getAppearanceNames(Collection<EffectiveAppearance> apps) {
		HashMap<EffectiveAppearance, String> map = new HashMap<EffectiveAppearance, String>();
		int number = 1;
		DecimalFormat df = new DecimalFormat("000");
		for (EffectiveAppearance ae : apps) {
			map.put(ae, "Material " + df.format(number));
			number++;
		}
		return map;
	}
	
	
	
	public static HashMap<EffectiveAppearance, U3DTexture> getSphereMapsMap(Collection<EffectiveAppearance> apps) {
		HashMap<EffectiveAppearance, U3DTexture> r = new HashMap<EffectiveAppearance, U3DTexture>();
		for (EffectiveAppearance a : apps) {
		    if (TextureUtility.hasReflectionMap(a, "polygonShader")) {
		    	CubeMap tex = TextureUtility.readReflectionMap(a, "polygonShader.reflectionMap");
		    	r.put(a, new U3DTexture(tex));
		    }
		}
		return r;
	}
	
	
	public static HashMap<CubeMap, byte[]> prepareSphereMap(Collection<CubeMap> maps) {
		HashMap<CubeMap, byte[]> r = new HashMap<CubeMap, byte[]>();
		for (CubeMap cm : maps) {
			BufferedImage bi = SphereMapGenerator.create(cm, 768, 768);
			ByteArrayOutputStream buffer = new ByteArrayOutputStream();
			try {
				ImageIO.write(bi, "PNG", buffer);
			} catch (IOException e) {
				e.printStackTrace();
			}
			r.put(cm, buffer.toByteArray());
		}
		return r;
	}
	
	
	public static HashMap<U3DTexture, String> getTextureNames(String prefix, Collection<U3DTexture> l) {
		HashMap<U3DTexture, String> map = new HashMap<U3DTexture, String>();
		int number = 1;
		DecimalFormat df = new DecimalFormat("000");
		for (U3DTexture ae : l) {
			map.put(ae, prefix + " " + df.format(number));
			number++;
		}
		return map;
	}
	
	
	public static HashMap<EffectiveAppearance, U3DTexture> getTextureMap(Collection<EffectiveAppearance> apps) {
		HashMap<EffectiveAppearance, U3DTexture> r = new HashMap<EffectiveAppearance, U3DTexture>();
		for (EffectiveAppearance a : apps) {
		    if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class, POLYGON_SHADER + "." + TEXTURE_2D, a)) {
		    	Texture2D tex = (Texture2D) createAttributeEntity(Texture2D.class, POLYGON_SHADER + "." + TEXTURE_2D, a);
		    	if (tex != null && tex.getImage() != null) {
		    		r.put(a, new U3DTexture(tex));
		    	}
		    }
		}
		return r;
	}
	
	
	
	public static BufferedImage getBufferedImage(ImageData img) {
		byte[] byteArray = img.getByteArray();
		int w = img.getWidth();
		int h = img.getHeight();
	    BufferedImage bi = new BufferedImage(w, h, TYPE_INT_ARGB);
        WritableRaster raster = bi.getRaster();
        int[] pix = new int[4];
        for (int y = 0, ptr = 0; y < h; y++) {
        	for (int x = 0; x < w; x++, ptr += 4) {
	            pix[0] = 0xFF & byteArray[ptr + 0];
	            pix[1] = 0xFF & byteArray[ptr + 1];
	            pix[2] = 0xFF & byteArray[ptr + 2];
	            pix[3] = 0xFF & byteArray[ptr + 3];
	            raster.setPixel(x, h - y - 1, pix);
        	}
        }
        return bi;
	}
	
	
	
	public static byte[] preparePNGImageData(ImageData img) {
		BufferedImage bi = getBufferedImage(img);
		ByteArrayOutputStream buffer = new ByteArrayOutputStream();
		try {
			ImageIO.write(bi, "PNG", buffer);
		} catch (IOException e) {
			e.printStackTrace();
		}
		return buffer.toByteArray();
	}
	
	
	public static HashMap<U3DTexture, byte[]> preparePNGTextures(Collection<U3DTexture> textures) {
		HashMap<U3DTexture, byte[]> r = new HashMap<U3DTexture, byte[]>();
		for (U3DTexture tex : textures) {
			ImageData img = tex.getImage();
			r.put(tex, preparePNGImageData(img));
		}
		return r;
	}
	
	
	public static HashMap<Geometry, Rectangle3D> getBoundingBoxes(Collection<Geometry> l) {
		HashMap<Geometry, Rectangle3D> r = new HashMap<Geometry, Rectangle3D>();
		for (Geometry g : l) {
			if (g instanceof PointSet) {
				PointSet ps = (PointSet) g;
				r.put(g, BoundingBoxUtility.calculateBoundingBox(ps));
			} else {
				r.put(g, new Rectangle3D(MAX_VALUE, MAX_VALUE, MAX_VALUE));
			}
		}
		return r;
	}
	
	
	public static SceneGraphComponent getSkyBox(JrScene scene) {
		Appearance rootApp = scene.getSceneRoot().getAppearance();
		if (rootApp == null || rootApp.getAttribute(SKY_BOX) == INHERITED) {
			return null;
		}
		CubeMap skyBox = (CubeMap)createAttributeEntity(CubeMap.class, SKY_BOX, rootApp, true);
		if (skyBox == null) return null;
		if (skyBox.getFront() 	== null
		||	skyBox.getBack() 	== null
		|| 	skyBox.getLeft() 	== null
		||	skyBox.getRight() 	== null
		|| 	skyBox.getTop() 	== null
		|| 	skyBox.getBottom() 	== null)
			return null;
		SceneGraphComponent r = new SceneGraphComponent();
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setVertexCount(4);
		ifsf.setFaceCount(1);
		ifsf.setVertexCoordinates(new double[][]{{1,1,0},{1,-1,0},{-1,-1,0},{-1,1,0}});
		ifsf.setFaceIndices(new int[][]{{0,1,2,3}});
		double o = 0.005;
		ifsf.setVertexTextureCoordinates(new double[][]{{1-o,1-o},{1-o,o},{o,o},{o,1-o}});
		ifsf.update();
		
		SceneGraphComponent front = new SceneGraphComponent();
		Appearance frontApp = new Appearance();
		createTexture(frontApp, POLYGON_SHADER, skyBox.getFront());
		front.setAppearance(frontApp);
		front.setGeometry(ifsf.getGeometry());
		front.setName("front");
		euclidean().translate(0, 0, 1.0).rotate(PI, 0, 1, 0).assignTo(front);

		SceneGraphComponent back = new SceneGraphComponent();
		Appearance backApp = new Appearance();
		createTexture(backApp, POLYGON_SHADER, skyBox.getBack());
		back.setAppearance(backApp);
		back.setGeometry(ifsf.getGeometry());
		back.setName("back");
		euclidean().translate(0, 0, -1.0).assignTo(back);

		SceneGraphComponent top = new SceneGraphComponent();
		Appearance topApp = new Appearance();
		createTexture(topApp, POLYGON_SHADER, skyBox.getTop());
		top.setAppearance(topApp);
		top.setGeometry(ifsf.getGeometry());
		top.setName("bottom");
		euclidean().translate(0, 1.0, 0).rotate(PI / 2, 1, 0, 0).rotate(-PI / 2, 0, 0, 1).assignTo(top);

		SceneGraphComponent bottom = new SceneGraphComponent();
		Appearance bottomApp = new Appearance();
		createTexture(bottomApp, POLYGON_SHADER, skyBox.getBottom());
		bottom.setAppearance(bottomApp);
		bottom.setGeometry(ifsf.getGeometry());
		bottom.setName("top");
		euclidean().translate(0, -1.0, 0).rotate(-PI / 2, 1, 0, 0).rotate(PI / 2, 0, 0, 1).assignTo(bottom);

		SceneGraphComponent left = new SceneGraphComponent();
		Appearance leftApp = new Appearance();
		createTexture(leftApp, POLYGON_SHADER, skyBox.getLeft());
		left.setAppearance(leftApp);
		left.setGeometry(ifsf.getGeometry());
		left.setName("left");
		euclidean().translate(-1.0, 0, 0).rotate(PI / 2, 0, 1, 0).assignTo(left);

		SceneGraphComponent right = new SceneGraphComponent();
		Appearance rightApp = new Appearance();
		createTexture(rightApp, POLYGON_SHADER, skyBox.getRight());
		right.setAppearance(rightApp);
		right.setGeometry(ifsf.getGeometry());
		right.setName("right");
		euclidean().translate(1.0, 0, 0).rotate(-PI / 2, 0, 1, 0).assignTo(right);
		  
		r.addChildren(front, back, top, bottom, left, right);
		r.setName("skybox");
		euclidean().rotate(PI / 2, 0, 1, 0).rotate(PI, 1, 0, 0).scale(1000.0).assignTo(r);
		
		Appearance skyBoxApp = new Appearance();
		skyBoxApp.setAttribute(POLYGON_SHADER + "." + LIGHTING_ENABLED, false);
		skyBoxApp.setAttribute(POLYGON_SHADER + "." + AMBIENT_COLOR, WHITE);
		skyBoxApp.setAttribute(POLYGON_SHADER + "." + AMBIENT_COEFFICIENT, 1.0);
		skyBoxApp.setAttribute(POLYGON_SHADER + "." + DIFFUSE_COEFFICIENT, 0.0);
		skyBoxApp.setAttribute(POLYGON_SHADER + "." + SPECULAR_COEFFICIENT, 0.0);
		r.setAppearance(skyBoxApp);
		return r;
	}
	
	
	private static void getVisibility_R(
		SceneGraphComponent root, 
		HashMap<SceneGraphComponent, Boolean> map, 
		boolean subTreeV,
		HashMap<SceneGraphComponent, EffectiveAppearance> appMap
	) {
		boolean visible = root.isVisible() && subTreeV;
		EffectiveAppearance ea = appMap.get(root);
//		boolean forceVisible = ea.getAttribute("U3D_ForceVisible", false);
		boolean showFaces = (visible && ea.getAttribute(FACE_DRAW, FACE_DRAW_DEFAULT));// || forceVisible; 
		map.put(root, showFaces);
		for (int i = 0; i < root.getChildComponentCount(); i++) {
			getVisibility_R(root.getChildComponent(i), map, visible, appMap);
		}
	}
	
	public static HashMap<SceneGraphComponent, Boolean> getVisibility(JrScene scene, HashMap<SceneGraphComponent, EffectiveAppearance> appMap) {
		HashMap<SceneGraphComponent, Boolean> r = new HashMap<SceneGraphComponent, Boolean>();
		getVisibility_R(scene.getSceneRoot(), r, true, appMap);
		return r;
	}
	
	
}
