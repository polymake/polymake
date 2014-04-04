/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

package de.jreality.renderman;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.geom.Rectangle2D;
import java.io.File;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;

import de.jreality.geometry.BallAndStickFactory;
import de.jreality.geometry.GeometryUtility;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.IndexedLineSetUtility;
import de.jreality.geometry.PolygonalTubeFactory;
import de.jreality.geometry.TubeUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.renderman.shader.SLShader;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.pick.Graphics3D;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ImageData;
import de.jreality.shader.PointShader;
import de.jreality.shader.PolygonShader;
import de.jreality.shader.RenderingHintsShader;
import de.jreality.shader.RootAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;
import de.jreality.util.SceneGraphUtility;

/**
 * A {@link de.jreality.scene.SceneGraphVisitor} for writing RenderMan<sup>TM</sup>
 * rib files.
 * <p>
 * <b>TODO list and Known issues</b>:
 * <ul>
 * <li> "implode", "flat" polygon shaders not supported</li>
 * <li> "transparency" attribute not correctly read from EffectiveAppearance. "polygonShader.transparency" for example is NOT seen</li>
 * <li> Clipping planes written but not tested</li>
 * <li> Add control over global options using (something like) "renderingHints"
 * shader {@link de.jreality.shader.RenderingHintsShader}</li>
 * <li> Test support for the differences between the various RenderMan renderers
 * (Pixar, 3DLight, Asis, Pixie)</li>
 * <li>Make sure users understand what the "rgba" output format implies (no
 * background)</li>
 * </ul>
 * <p>
 * <b>comments:</b>:
 * <ul>
 * <li> Use the {@link de.jreality.shader.CommonAttributes#RMAN_GLOBAL_INCLUDE_FILE} to set
 * global options: this is rib file that is read in at the top of the output rib
 * file, and can be used to set all kinds of global variables.</li>
 * * <li>Writing ordinary texture files: Currently tries to write TIFF, if can't then writes PNG.</li>
 * </ul>
 * 
 * 
 * 
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>,
 *         Charles Gunn, Nils Bleicher
 * @see de.jreality.shader.CommonAttributes
 */
public class RIBVisitor extends SceneGraphVisitor {
	
	private SceneGraphComponent root;
	private SceneGraphPath cameraPath;
	transient protected double[] world2Camera;
	transient protected double[] object2worldTrafo;
	transient protected SceneGraphPath object2world = new SceneGraphPath();
	transient protected Graphics3D context;
	transient private Camera camera;
	transient private int width = 640;
	transient private int height = 480;
	transient private String ribFileName;
	transient private int[] maximumEyeSplits = { 10 };
	transient protected boolean shadowEnabled = false;
	transient private boolean fogEnabled=false;
	transient protected boolean fullSpotLight = false;
	transient protected boolean retainGeometry = false; // should geometry be saved
	// using "Begin/EndArchive"?
	transient protected boolean useProxyCommands = true;
	transient protected boolean useOldTransparency = false;
	// user can specify that tubes and spheres drawn by the appearances are to
	// be opaque reqardless of the current transparency value
	transient protected Color cs = null;
	transient protected boolean opaqueTubes = false;
	transient protected boolean smooth = true;
	transient protected boolean drawSpheres=true;
	transient protected boolean drawTubes=true;
	transient protected float tubeRadius = 0.1f;
	transient protected float pointRadius = 0.1f;
	transient private boolean writeShadersToFile = true;
	transient private boolean hasPw = false;		// can the renderer handle "Pw" successfully?

	transient private String globalIncludeFile = "";
	transient private int rendererType = RIBViewer.TYPE_PIXAR;
	transient private int currentMetric = Pn.EUCLIDEAN;
	transient private String outputDisplayFormat = "rgb";
	transient protected String textureFileSuffix = "tex"; 
	transient protected boolean handlingProxyGeometry = false;
	transient String currentProxyCommand = null;
	transient SceneGraphComponent currentProxySGC = null;

	transient protected String shaderPath = null;
	transient public EffectiveAppearance eAppearance = EffectiveAppearance.create();
	transient private int textureCount = 0;
	transient private Map<ImageData, String> textures = new HashMap<ImageData, String>();
	transient private Hashtable<SceneGraphComponent, String> archivedSGCTable = new Hashtable<SceneGraphComponent, String>();
	transient private Hashtable<PointSet, String> archivedPSTable = new Hashtable<PointSet, String>();
	transient private Hashtable<IndexedLineSet, String> archivedILSTable = new Hashtable<IndexedLineSet, String>();
	transient private Hashtable<IndexedFaceSet, String> archivedIFSTable = new Hashtable<IndexedFaceSet, String>();
	transient int archiveCount = 0;
	transient protected Ri ri = new Ri();
	transient int whichEye = CameraUtility.MIDDLE_EYE;
	private boolean raytracedReflectionsEnabled;
	private boolean raytracedVolumesEnabled;
	RenderScript renderScript;

	private float currentOpacity;
	private Appearance rootAppearance;
	private String outputFileName;
	protected boolean transparencyEnabled, ignoreAlpha0 = true;
	private int cubeMapCount;
	private String cubeMapFileSuffix = "env";
	protected DefaultGeometryShader dgs;
	protected RenderingHintsShader rhs;
	private HashMap<ImageData, String> cubeMaps = new HashMap<ImageData, String>();
	private double[] zFlipMatrix;

	public void visit(Viewer viewer, String name) {
		// handle the file name
		ribFileName = name;
		if (!ribFileName.endsWith(".rib"))
			ribFileName = ribFileName + ".rib";

		// determine files for render-script
		File ribF = new File(ribFileName);
		File dir = ribF.getParentFile();
		renderScript = new RenderScript(dir, ribF.getName(), rendererType);

		// register standard shaders for render script
		renderScript.addShader("defaultpolygonshader.sl");
		renderScript.addShader("twosidepolygonshader.sl");
		renderScript.addShader("constantTexture.sl");

		int index = ribFileName.lastIndexOf(File.separatorChar);
		outputFileName = ribFileName.substring(index + 1,
				ribFileName.length() - 3)
				+ "tif";

		root = viewer.getSceneRoot();
		cameraPath = viewer.getCameraPath();
		camera = CameraUtility.getCamera(viewer);
		rootAppearance = root.getAppearance();
		if (rootAppearance == null)
			rootAppearance = new Appearance();
		eAppearance = EffectiveAppearance.create();
		eAppearance = eAppearance.create(rootAppearance);
		context = new Graphics3D(cameraPath, object2world, ((double)width)/height);
		world2Camera = cameraPath.getInverseMatrix(null);

		if (writeShadersToFile) {
			writeStandardShaders(ribFileName);
		}
		if (rootAppearance != null)
			handleRootAppearance();

		ri.begin(ribFileName);
		if (camera.isStereo()) {
			// Careful: the rest of the code expects left eye to be rendered
			// first!
			whichEye = CameraUtility.LEFT_EYE;
			index = ribFileName.lastIndexOf(File.separator);
			outputFileName = ribFileName.substring(index + 1, ribFileName.length() - 4)+ "L.tif";
			ri.frameBegin(0);
			render();
			ri.frameEnd();
			whichEye = CameraUtility.RIGHT_EYE;
			outputFileName = ribFileName.substring(index + 1, ribFileName.length() - 4)+ "R.tif";
			ri.frameBegin(1);
			render();
			ri.frameEnd();
		} else
			render();
		ri.end();

		renderScript.finishScript();
	}

	/**
	 * Does the work in rendering the scene graph.
	 * 
	 */
	private void render() {

		handleGlobalSettings();

		// handle the camera model next
		// 
		// TODO handle negative far clipping plane as used in elliptic geometry
		// (OpenGL supports this) (can we do this here by generating our own
		// matrices?)
		// TODO figure out how to specify RI_INFINITY in a RIB file
		ri.clipping(camera.getNear(), (camera.getFar() > 0) ? camera.getFar(): 1000.0);
		ri.depthOfField(camera.getFStop(), camera.getFocalLength(), camera.getFocus());
		boolean testCameraExplicit = false;
		double aspectRatio = ((double) width) / height;
		zFlipMatrix = MatrixBuilder.euclidean().scale(1, 1, -1).getArray();
		if (testCameraExplicit) {
			// following experiment to set the camera to NDC transformation
			// explicitly without
			// using RenderMan commands for doing that. Doesn't work. -gunn
			// 01.09.06
			// camera.setNear( camera.getNear() * -1);
			// camera.setFar(camera.getFar() * -1);
			// flip the z-direction
			ri.transform(RIBHelper.fTranspose(zFlipMatrix));
			ri.comment("Home-grown camera transformation");
			double[] c2ndc = CameraUtility.getCameraToNDC(camera, 1.0,
					currentMetric);
			ri.concatTransform(RIBHelper.fTranspose(c2ndc));
			// camera.setNear( camera.getNear() * -1);
			// camera.setFar( camera.getFar() * -1);
		} else {
			if (camera.isStereo()) {
				double[] eyeP = CameraUtility.getEyePosition(camera, whichEye);
				Rectangle2D vp = CameraUtility.getOffAxisViewPort(camera,
						CameraUtility.getViewport(camera, aspectRatio), eyeP);
				ri.comment("Testing left eye stereo");
				// can a stereo camera be non-perspective?
				ri.projection(camera.isPerspective() ? "perspective": "orthographic", null);
				ri.screenWindow(vp);
				ri.concatTransform(RIBHelper.fTranspose(zFlipMatrix));
				double[] moveToEye = Rn.inverse(null, P3.makeTranslationMatrix(
						null, eyeP, currentMetric));
				ri.concatTransform(RIBHelper.fTranspose(moveToEye));
			} else {
				HashMap<String, Object> map = new HashMap<String, Object>();
				float fov = (float) camera.getFieldOfView();
				float a = 1.0f;
				if (camera.isPerspective()) {
					map.put("fov", new Float(fov));
					ri.projection("perspective", map);
				} else {
					ri.projection("orthographic", map);
					a = (float) (1 / ((Math.tan((Math.PI / 180.0)
							* camera.getFieldOfView() / 2.0) * camera
							.getFocus())));
					ri.concatTransform(new float[] { a, 0, 0, 0, 0, a, 0, 0, 0,
							0, 1, 0, 0, 0, 0, 1 });
				}
				ri.concatTransform(RIBHelper.fTranspose(zFlipMatrix));
			}
		}

		if (whichEye == CameraUtility.LEFT_EYE) {
			ri.archiveBegin("world");
		} else if (whichEye == CameraUtility.RIGHT_EYE) {
			ri.readArchive("world");
			return;
		}

		// handle the lights in camera coordinates 
		// since PRMan can't seem to properly transform light parameters by non-euclidean tforms
		new LightCollector(root, this);
		// handle the world
		ri.comment("world to camera");
		ri.concatTransform(RIBHelper.fTranspose(world2Camera));
		processClippingPlanes();
		
		
		ri.worldBegin();
		// alpha in the output format means skip over any background settings
		if (outputDisplayFormat != "rgba")
			handleBackground();   

		handleFog();    //<- should be called in handleGlobalSettings, but since jogl-viewer doesn't renders fog on the background (bug?), we call it here

		// finally render the scene graph
		root.accept(this);
		ri.worldEnd();
		if (whichEye == CameraUtility.LEFT_EYE) {
			ri.archiveEnd();
			ri.readArchive("world");
		}
	}

	private void handleRootAppearance() {
		maximumEyeSplits[0] = ((Integer) eAppearance.getAttribute(
				CommonAttributes.RMAN_MAX_EYE_SPLITS, new Integer(
						maximumEyeSplits[0]))).intValue();
		shaderPath = (String) eAppearance.getAttribute(
				CommonAttributes.RMAN_SEARCHPATH_SHADER, "");
		textureFileSuffix = (String) eAppearance.getAttribute(
				CommonAttributes.RMAN_TEXTURE_FILE_SUFFIX, "tex");
		shadowEnabled = eAppearance.getAttribute(
				CommonAttributes.RMAN_SHADOWS_ENABLED, false);
		raytracedReflectionsEnabled = eAppearance.getAttribute(
				CommonAttributes.RMAN_RAY_TRACING_REFLECTIONS, false);
		raytracedVolumesEnabled = eAppearance.getAttribute(
				CommonAttributes.RMAN_RAY_TRACING_VOLUMES, false);
		currentMetric = eAppearance.getAttribute(CommonAttributes.METRIC,
				Pn.EUCLIDEAN);
		outputDisplayFormat = (String) eAppearance.getAttribute(
				CommonAttributes.RMAN_OUTPUT_DISPLAY_FORMAT, "rgb");
		globalIncludeFile = (String) eAppearance.getAttribute(
				CommonAttributes.RMAN_GLOBAL_INCLUDE_FILE, "");
		useOldTransparency = eAppearance.getAttribute(
				CommonAttributes.USE_OLD_TRANSPARENCY, false);
		if(!globalIncludeFile.equals("")) System.err.println("Preamble is " + globalIncludeFile);    
		fogEnabled=(boolean)eAppearance.getAttribute(CommonAttributes.FOG_ENABLED, CommonAttributes.FOG_ENABLED_DEFAULT);   
	}

	/**
	 * Write the top of the rib file
	 * 
	 */
	private void handleGlobalSettings() {
		HashMap<String, Object> map = new HashMap<String, Object>();
		map.put("shader", ("".equals(shaderPath) ? "" : (shaderPath + ":"))
				+ ".:&");
		ri.option("searchpath", map);

		map.clear();
		map.put("eyesplits", maximumEyeSplits);
		ri.option("limits", map);
		ri.display(new File(outputFileName).getName(), "tiff",
				outputDisplayFormat, null);

		ri.format(width, height, 1);
		// TODO make this a variable
		ri.shadingRate(1f);
		if(shadowEnabled){
			if(rendererType==RIBViewer.TYPE_3DELIGHT)
				ri.verbatim("Attribute \"visibility\"  \"string transmission\" \"shader\"");            
			else {
				ri.verbatim("Attribute \"visibility\"  \"int transmission\" [1]");
				ri.verbatim("Option \"trace\"  \"int maxdepth\" [2]");
			}
		}

		if(raytracedReflectionsEnabled||raytracedVolumesEnabled){
			if(rendererType==RIBViewer.TYPE_3DELIGHT){
				ri.verbatim("Attribute \"visibility\"  \"integer trace\" [1]");
				ri.verbatim("Attribute \"visibility\"  \"string diffuse\" \"shader\"");
				ri.verbatim("Attribute \"visibility\"  \"string specular\" \"shader\"");
				ri.verbatim("Option \"trace\"  \"integer maxdepth\" [2]");
			}else {      
				ri.verbatim("Attribute \"visibility\"  \"int diffuse\" [1]");
				ri.verbatim("Attribute \"visibility\"  \"int specular\" [1]");
				ri.verbatim("Option \"trace\"  \"int maxdepth\" [2]");
			}
		}

//		if(fogEnabled)
//		handleFog();    

		// make sure this is the last thing done, to maximize what the user can override.
		if (globalIncludeFile != "")
			ri.readArchive((String) globalIncludeFile);

	}

	/**
	 * Handle background specifications contained in the top-level appearance:
	 * skybox, flat background color, or gradient background colors TODO: fog
	 * 
	 * @see de.jreality.shader.RootAppearance
	 */
	private void handleBackground() {
		HashMap<String, Object> map = new HashMap<String, Object>();
		Appearance ap = root.getAppearance();
		updateShaders(eAppearance);
		if (ap != null) {
			if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class,
					CommonAttributes.SKY_BOX, ap)) {
				CubeMap cm = (CubeMap) AttributeEntityUtility
				.createAttributeEntity(CubeMap.class,
						CommonAttributes.SKY_BOX, ap, true);        
				RendermanSkyBox.render(this, world2Camera, cm);  
			} else {
				Color[] clrs = new Color[1];
				Object o = eAppearance.getAttribute(
						CommonAttributes.BACKGROUND_COLORS, Appearance.DEFAULT,
						clrs.getClass());
				if (o != Appearance.DEFAULT && o instanceof Color[]) {
					clrs = (Color[]) o;
					double sx = eAppearance.getAttribute(
							CommonAttributes.BACKGROUND_COLORS_STRETCH_X, 1.0);
					double sy = eAppearance.getAttribute(
							CommonAttributes.BACKGROUND_COLORS_STRETCH_Y, 1.0);
					// insert a polygon at the back of the viewing frustrum
					handleBackgroundColors(clrs, sx, sy, camera, cameraPath
							.getMatrix(null));
				} else {
					Color clr = (Color) eAppearance.getAttribute(
							CommonAttributes.BACKGROUND_COLOR,
							CommonAttributes.BACKGROUND_COLOR_DEFAULT);
					float[] f = clr.getRGBColorComponents(null);
					clrs = new Color[4];
					for (int i = 0; i<4; ++i) clrs[i] = new Color(f[0], f[1], f[2]);
					handleBackgroundColors(clrs, 1.0, 1.0, camera, cameraPath.getMatrix(null));

//					map.put("color background", f);
//					ri.imager("background", map);
				}
			}
		}
	}

	/**
	 * Handle request for gradient like background image
	 * 
	 * @param colors
	 * @param sx
	 * @param sy
	 * @param camera
	 * @param w2c
	 */
	private void handleBackgroundColors(Color[] colors, double sx, double sy,
			Camera camera, double[] w2c) {
		Rectangle2D vp = CameraUtility.getViewport(camera, ((double) width)/ height);
		double z = camera.getFar() - 10E-4;
		double xmin = sx * vp.getMinX();
		double xmax = sx * vp.getMaxX();
		double ymin = sy * vp.getMinY();
		double ymax = sy * vp.getMaxY();
		double[][] pts = { { z * xmin, z * ymin, -z },
				{ z * xmax, z * ymin, -z }, { z * xmax, z * ymax, -z },
				{ z * xmin, z * ymax, -z } };
		double[][] cd = new double[4][3];
		for (int i = 0; i < 4; ++i) {
			float[] foo = colors[i].getRGBComponents(null);
			for (int j = 0; j < 3; ++j)
				cd[(i + 2) % 4][j] = foo[j];
		}
		IndexedFaceSet bkgd = IndexedFaceSetUtility.constructPolygon(pts);
		bkgd.setVertexAttributes(Attribute.COLORS, StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(cd));
		ri.attributeBegin();
		ri.concatTransform(RIBHelper.fTranspose(w2c));
		ri.comment("Disable shadows for background");
		if (rendererType == RIBViewer.TYPE_3DELIGHT) 
			ri.verbatim("Attribute \"visibility\"  \"string transmission\" \"transparent\"");
		else 
			ri.verbatim("Attribute \"visibility\"  \"int transmission\" [0]");
		ri.surface("constant", null);
		pointPolygon(bkgd, null);
		ri.attributeEnd();
	}

	private static void writeStandardShaders(String name) {
		RIBHelper.writeShader(name, "defaultpolygonshader.sl");
		RIBHelper.writeShader(name, "twosidepolygonshader.sl");
		RIBHelper.writeShader(name, "noneuclideanpolygonshader.sl");
		RIBHelper.writeShader(name, "noneuclideanlight.sl");
		RIBHelper.writeShader(name, "constantTexture.sl");
	}

	// look first for explicitly set atmosphere shader; if not set, 
	// look for jreality fog settings and use standard RenderMan "fog" shader
	private void handleFog() {
		Object obj = eAppearance.getAttribute(CommonAttributes.RMAN_VOLUME_ATMOSPHERE_SHADER, Appearance.INHERITED,
				SLShader.class);
		if (obj != Appearance.INHERITED) {
			SLShader slShader = (SLShader) obj;      
			ri.atmosphere(slShader.getName(), slShader.getParameters());
			return;
		}
		if (!fogEnabled) return;
		HashMap<String, Object> fogMap=new HashMap<String, Object>();  
		// if no fog color is set, use background color
		Color fogcolor;
		obj =  eAppearance.getAttribute(CommonAttributes.FOG_COLOR, Appearance.INHERITED);
		if (obj == Appearance.INHERITED) 
			fogcolor = (Color)eAppearance.getAttribute(CommonAttributes.BACKGROUND_COLOR,RootAppearance.BACKGROUND_COLOR_DEFAULT);
		else fogcolor = (Color) obj;
		fogMap.put("background",fogcolor);
		fogMap.put("distance",new Float(1/(double)eAppearance.getAttribute(CommonAttributes.FOG_DENSITY, CommonAttributes.FOG_DENSITY_DEFAULT)));
		ri.atmosphere("fog", fogMap);    
//		ri.interior("fog", fogMap);    
		ri.exterior("fog", fogMap);    
	}

	private void processClippingPlanes() {
		List clipPlanes = SceneGraphUtility.collectClippingPlanes(root);
		int n = clipPlanes.size();
		for (int i = 0; i < n; ++i) {
			SceneGraphPath lp = (SceneGraphPath) clipPlanes.get(i);
			SceneGraphNode cp = lp.getLastElement();
			if (!(cp instanceof ClippingPlane))
				LoggingSystem.getLogger(RIBHelper.class).log(Level.WARNING,
						"Invalid clipplane class " + cp.getClass().toString());
			else {
				double[] mat = lp.getMatrix(null);
				Matrix m = new Matrix(mat);
				// find the image of (0,0,0,1) and (0,0,1,0) under object to world transform
				double[] point = m.getColumn(3), point3 = new double[3];
				double[] normal = m.getColumn(2), normal3 = new double[3];
				Pn.dehomogenize(point3, point);
				Pn.dehomogenize(normal3, normal);
				Rn.times( normal3, -1, Pn.dehomogenize(normal3, normal));
				ri.clippingPlane((float) normal3[0], (float) normal3[1], (float) normal3[2],
						(float)  point3[0], (float) point3[1], (float) point3[2]);
			}
		}
	}
	/*
	 * Visit methods start here
	 */
	public void visit(SceneGraphComponent c) {
		if (!c.isVisible())    return;
		// allow possibility of archiving scene graph components too
		Appearance a = c.getAppearance();
		boolean archive = false;
		String archiveName = null;
		if (a != null) {
			Object obj = a.getAttribute(CommonAttributes.RMAN_ARCHIVE_CURRENT_NODE, Boolean.class);
			if (obj instanceof Boolean) {
				archive = ((Boolean) obj).booleanValue();
			}
			if (archive)	{
				Object which = archivedSGCTable.get(c);
				if (which != null) {
					ri.readArchive((String) which);
					return;
				} else {
					ri.comment("Retained SGC " + c.getName());
					archiveName = c.getName() + archiveCount;
					ri.archiveBegin(archiveName);
				}
				
			}			
		}
		EffectiveAppearance tmp = eAppearance;
		ri.attributeBegin(c.getName());
		if (a != null) 	eAppearance = eAppearance.create(a);
		readAttributesFromEffectiveAppearance(eAppearance);
		object2world.push(c);
		object2world.getMatrix(object2worldTrafo);
		if (hasProxy(c)) {
			RIBHelper.processShader(dgs.getPolygonShader(), this, "polygonShader");
			handleCurrentProxy();
		} else
			c.childrenAccept(this);
		object2world.pop();
		ri.attributeEnd(c.getName());
		if (archive)	{
			ri.archiveEnd();
			ri.readArchive(archiveName);
			archivedSGCTable.put(c, archiveName);
			archiveCount++;
		}
		// restore effective appearance
		eAppearance = tmp;
		readAttributesFromEffectiveAppearance(eAppearance);
	}

	public void visit(Transformation t) {
		double[] mat = t.getMatrix();
		boolean isIdentity = Rn.isIdentityMatrix(t.getMatrix(), 10E-8);
		if (!isIdentity) ri.concatTransform(RIBHelper.fTranspose(mat));
	}

	public void visit(Appearance a) {
	}

	private void readAttributesFromEffectiveAppearance(EffectiveAppearance eap) {
		//    if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class, ShaderUtility.nameSpace(type,"reflectionMap"), eap))
//		{
//		ri.verbatim("Attribute \"visibility\"  \"int diffuse\" [1]");
//		ri.verbatim("Attribute \"visibility\"  \"int specular\" [1]");
//		}

		Map m = (Map) eap.getAttribute(CommonAttributes.RMAN_ATTRIBUTE, null, Map.class);
		if (m != null) {
			for (Iterator i = m.keySet().iterator(); i.hasNext();) {
				String key = (String) i.next();
				ri.attribute(key, (Map) m.get(key));
			}
		}
		updateShaders(eap);
		// read current values from the effective appearance
		currentMetric = eap.getAttribute(CommonAttributes.METRIC,Pn.EUCLIDEAN);
		retainGeometry = eap.getAttribute(CommonAttributes.BACKEND_RETAIN_GEOMETRY, false); 
		//if(rhs.getOpaqueTubesAndSpheres()!=null)  
		opaqueTubes = rhs.getOpaqueTubesAndSpheres();
		transparencyEnabled = rhs.getTransparencyEnabled();
		ignoreAlpha0 = rhs.getIgnoreAlpha0();
		/** 
		 * evaluate the special shaders which might be specified in the effective appearance:
		 * displacement, imager, and interior and exterior volume shaders
		 * The values of the attributes are instances of de.jreality.renderman.shader.SLShader
		 **/
		Object obj = eap.getAttribute(CommonAttributes.RMAN_DISPLACEMENT_SHADER, Appearance.INHERITED,SLShader.class);
		if (obj != Appearance.INHERITED) {
			SLShader slShader = (SLShader) obj;
			ri.displacement(slShader.getName(), slShader.getParameters());
		}
		obj = eap.getAttribute(CommonAttributes.RMAN_IMAGER_SHADER, Appearance.INHERITED,
				SLShader.class);
		if (obj != Appearance.INHERITED) {
			SLShader slShader = (SLShader) obj;
			ri.imager(slShader.getName(), slShader.getParameters());
		}
		obj = eap.getAttribute(CommonAttributes.RMAN_VOLUME_EXTERIOR_SHADER, Appearance.INHERITED,
				SLShader.class);
		if (obj != Appearance.INHERITED) {
			SLShader slShader = (SLShader) obj;
			if(!raytracedVolumesEnabled) System.err.println("CommonAttributes.RMAN_RAY_TRACING_VOLUMES must be set true for Exterior volume shaders"); //ri.verbatim("Attribute \"shade\" \"strategy\" [\"vpvolumes\"]");  
			ri.exterior(slShader.getName(), slShader.getParameters());
		}
		obj = eap.getAttribute(CommonAttributes.RMAN_VOLUME_INTERIOR_SHADER, Appearance.INHERITED,
				SLShader.class);
		if (obj != Appearance.INHERITED) {
			SLShader slShader = (SLShader) obj;
			if(!raytracedVolumesEnabled) ri.verbatim("Attribute \"shade\" \"strategy\" [\"vpvolumes\"]"); 
			ri.interior(slShader.getName(), slShader.getParameters());
		}
	}


	private void updateShaders(EffectiveAppearance eap) {
		dgs = ShaderUtility.createDefaultGeometryShader(eap);
		rhs = ShaderUtility.createRenderingHintsShader(eap);
		// we need to know the current opacity to workaround a renderman prman bug in pointsPolygon ... see below
		currentOpacity = 1f;
		if (!(handlingProxyGeometry && rhs.getOpaqueTubesAndSpheres()) && rhs.getTransparencyEnabled()) {
			double d = eap.getAttribute(CommonAttributes.TRANSPARENCY, CommonAttributes.TRANSPARENCY_DEFAULT);
			currentOpacity = 1f - (float) d;
		}
//		System.err.println("Current opacity is "+currentOpacity);
	}

	/**
	 * @param tex
	 * @return
	 */
	public String writeTexture(Texture2D tex) {
		String extSource = tex.getExternalSource();
		if (extSource != null) return extSource + "." + textureFileSuffix;
		ImageData data = tex.getImage();
		return writeTexture(data, tex.getRepeatS(), tex.getRepeatT());
	}

	public String writeTexture(ImageData data, int repeatS, int repeatT) {
		String noSuffix = (String) textures.get(data);
		if (noSuffix == null) {
			String texFileName = "_texture" + (textureCount++);
			noSuffix = ribFileName + texFileName;
			RIBHelper.writeTexture(data, noSuffix);
			textures.put(data, noSuffix);
			renderScript.addTexture(texFileName, repeatS, repeatT);
		}
		return noSuffix + "." + textureFileSuffix; 
	}

	/**
	 * Check to see if the Geometry has a proxy rib command attached to it.
	 * 
	 * @param g
	 * @return
	 */
	public void checkForProxy(Geometry g) {
		if (!useProxyCommands) return;
		Object proxy = g.getGeometryAttributes(CommonAttributes.RMAN_PROXY_COMMAND);
		handleProxyObject(proxy);
	}

	private boolean hasProxy()	{
		return currentProxyCommand != null || currentProxySGC != null;
	}

	public boolean hasProxy(Geometry g)	{
		checkForProxy(g);
		return hasProxy();
	}

	public boolean hasProxy(SceneGraphComponent g) {
		if (!useProxyCommands || g.getAppearance() == null) return false;
		Object obj = g.getAppearance().getAttribute(CommonAttributes.RMAN_PROXY_COMMAND);
		handleProxyObject(obj);
		return hasProxy();
	}

	private void handleProxyObject(Object obj) {
		currentProxyCommand = null;
		currentProxySGC = null;
		if (obj != null) {
			if (obj instanceof String) {
				currentProxyCommand = (String) obj;
			}
			if (obj instanceof SceneGraphComponent)	{
				currentProxySGC = (SceneGraphComponent) obj;
			}
		}
	}

	private void handleCurrentProxy() {
		if (currentProxyCommand != null)	{
			ri.verbatim(currentProxyCommand);
			currentProxyCommand = null;			
		} else if (currentProxySGC != null)	{
			visit(currentProxySGC);
			currentProxySGC = null;
		}
	}

	public void visit(Geometry g) {
		if (hasProxy(g)) handleCurrentProxy();
		super.visit(g);
	}

	// we need this to know whether we're generating tubes and spheres; if so, we don't generate more
	transient boolean insidePointset = false;
	public void visit(PointSet g) {
		boolean drawPoints = dgs.getShowPoints();
		if (drawPoints)	{
			ri.attributeBegin();
			RIBHelper.processShader(dgs.getPointShader(), this, "pointShader");
			if (!insidePointset) {
				insidePointset = true;
				if (retainGeometry) {
					Object which = archivedPSTable.get(g);
					if (which != null) {
						ri.readArchive((String) which);
					} else {
						ri.comment("Retained point set " + g.getName());
						String finalname = g.getName() + archiveCount;
						ri.archiveBegin(finalname);
						_visit(g);
						ri.archiveEnd();
						ri.readArchive(finalname);
						archivedPSTable.put(g, finalname);
						archiveCount++;
					}
				} else
					_visit(g);
			} else
				_visit(g);
			insidePointset = false;
			ri.attributeEnd();
		}
	}
	
	public void visit(IndexedLineSet g) {
		boolean drawLines = dgs.getShowLines();
		if (drawLines)	{
			ri.attributeBegin();
			RIBHelper.processShader(dgs.getLineShader(), this, "lineShader");
			ri.comment("IndexedLineSet " + g.getName());
			checkForProxy(g);
			if (hasProxy(g)) {
				handleCurrentProxy();
				insidePointset = false;
			} else {
				if (!insidePointset) {
					insidePointset = true;
					// p is not a proper subclass of IndexedLineSet
					if (retainGeometry) {
						Object which = archivedILSTable.get(g);
						if (which != null) {
							ri.readArchive((String) which);
						} else {
							ri.comment("Retained line set " + g.getName());
							String finalname = g.getName() + archiveCount;
							ri.archiveBegin(finalname);
							_visit(g);
							ri.archiveEnd();
							ri.readArchive(finalname);
							archivedILSTable.put(g, finalname);
							archiveCount++;
						}
					} else _visit(g);
					visit((PointSet) g);
					insidePointset = false;
				} else
					_visit(g);
				visit((PointSet) g);
			}
			ri.attributeEnd();			
		}  else
			visit((PointSet) g);
	}


	

	public void visit(IndexedFaceSet g) {
		boolean faceDraw = dgs.getShowFaces();
		if (faceDraw) {
			ri.attributeBegin();
			RIBHelper.processShader(dgs.getPolygonShader(), this, "polygonShader");
			checkForProxy(g);
			if (hasProxy((Geometry) g)) {
				handleCurrentProxy();
				insidePointset = false;
			} else {
				if (!insidePointset) {
					insidePointset = true;
					if (retainGeometry) {
						Object which = archivedIFSTable.get(g);
						if (which != null) {
							ri.readArchive((String) which);
						} else {
							ri.comment("Retained geometry " + g.getName());
							String finalname = g.getName() + "_" + archiveCount;
							ri.archiveBegin(finalname);
							_visit(g);
							ri.archiveEnd();
							ri.readArchive(finalname);
							archivedIFSTable.put(g, finalname);
							archiveCount++;
						}
					} else _visit(g);
					visit((IndexedLineSet) g);
					insidePointset = false;
				} else
					_visit(g);
			}
			ri.attributeEnd();
		} else visit ((IndexedLineSet) g);
	}

	 // here are the methods which actually render the geometric primitives
	float[] raw = new float[4];
	private void _visit(PointSet p) {		
			ri.comment("PointSet " + p.getName());
			int n = p.getNumPoints();
			DataList coord = p.getVertexAttributes(Attribute.COORDINATES);
			if (coord == null)return;
			ri.attributeBegin();

			DataList radii = p.getVertexAttributes(Attribute.RELATIVE_RADII);
			DoubleArray da = null;
			if (radii != null) da = radii.toDoubleArray();
			DataList ind = p.getVertexAttributes(Attribute.INDICES);
			int[] vind = null;
			if (ind != null) vind = ind.toIntArray(null);
			Color pointColor;
			
			if (drawSpheres) {
				// process the polygon shader associated to this point shader
				// This is something of a hack since we don't really know what the associated string is
				handlingProxyGeometry = true; 
				if (opaqueTubes)	ri.opacity(1.0f);
				PointShader ls = dgs.getPointShader();
				if (ls instanceof DefaultPointShader)	{
					PolygonShader ps = ((DefaultPointShader)ls).getPolygonShader();
					if (ps instanceof DefaultPolygonShader)	{
						pointColor = ((DefaultPolygonShader)ps).getDiffuseColor();
						float[] cc = new float[3];
						cc = pointColor.getRGBColorComponents(cc);
						ri.color(cc);
					}
				}
				double[][] vColData=null;
				if( p.getVertexAttributes(Attribute.COLORS)!=null)
					vColData= p.getVertexAttributes(Attribute.COLORS).toDoubleArrayArray(null);          
				double[][] a = coord.toDoubleArrayArray(null);
				double[] trns = new double[16];
				for (int i = 0; i < n; i++) {
					if (vind != null && vind[i] == 0) continue;		
					float realR = pointRadius;
					if (radii != null) realR = (float) (realR*da.getValueAt(i));
					if (a[i].length == 4 && a[i][3] == 0.0) continue;
					trns = MatrixBuilder.init(null, currentMetric).translate(a[i]).getArray();
					ri.transformBegin();
					ri.concatTransform(RIBHelper.fTranspose(trns));          
					//varying vertexColors
					if (vColData != null && vColData[0]!=null){
						if(vColData[0].length==4 && !opaqueTubes)
							vColData[i][3]*=currentOpacity;
						ri.color(vColData[i]);
					}          
					HashMap map = new HashMap();
					ri.sphere(realR, -realR, realR, 360f, map);
					ri.transformEnd();
				}
				handlingProxyGeometry = false;  

			} else {
				// use the RenderMan "points" command to draw the points				
				PointShader ls = dgs.getPointShader();
				if (ls instanceof DefaultPointShader)	{
						pointColor = ((DefaultPointShader)ls).getDiffuseColor();
						ri.color(pointColor);
				}
				HashMap<String, Object> map = new HashMap<String, Object>();
				int fiber = GeometryUtility.getVectorLength(coord);
				double[][] pc = new double[n][fiber];
				coord.toDoubleArrayArray(pc);
				float[] pcf = new float[3 * n];
				double[] vector = new double[3];
				for (int i = 0; i < pc.length; i++) {
					if (fiber == 4)	{
						Pn.dehomogenize(vector, pc[i]);	
					} else
						vector = pc[i];
					for (int k = 0; k<3; ++k)	pcf[i*3+k] = (float) vector[k];
				}
				map.put("P", pcf);
				map.put("constant float constantwidth", new Float(pointRadius));
				
				DataList vCol=p.getVertexAttributes(Attribute.COLORS);
		        if(vCol!=null){
		        	int vColLength=GeometryUtility.getVectorLength(vCol);
		        	float[] vCs=new float[3*vCol.size()];
		        	float[] vOs=new float[3*vCol.size()];
		        	for(int i=0;i<vCol.size();i++){
		        		DoubleArray rgba = vCol.item(i).toDoubleArray();
//		        		System.out.println(new Color((int)rgba.getValueAt(0),(int)rgba.getValueAt(1),(int)rgba.getValueAt(2)));
		        		vCs[3*i] = (float) rgba.getValueAt(0);
		        		vCs[3*i+1] = (float) rgba.getValueAt(1);
		        		vCs[3*i+2] = (float) rgba.getValueAt(2);
		        		if (vColLength == 4) {
		        			vOs[3*i] = (float) rgba.getValueAt(3);
		        			vOs[3*i+1] = (float) rgba.getValueAt(3);
		        			vOs[3*i+2] = (float) rgba.getValueAt(3);
		        		}
		        	}
		        	map.put("varying color Cs", vCs);
		        	if (vColLength == 4)
		        		map.put("varying color Os", vOs);
		        }
		        
				ri.points(n, map);
			}
			
			ri.attributeEnd();
			
			DataList labelsList=p.getVertexAttributes(Attribute.LABELS);
			if(labelsList!=null){
				if(dgs.getPointShader() instanceof DefaultPointShader){
					RIBHelper.createRIBLabel(p, (DefaultTextShader)((DefaultPointShader)dgs.getPointShader()).getTextShader(), this);
				}else 
					System.err.println("textshaders only for defaultshaders");
			}			
		}

	private void _visit(IndexedLineSet g)	{

			DataList dl = g.getEdgeAttributes(Attribute.INDICES);
			if(dl != null){
				if (drawTubes)  {
					
					DataList edgec =  g.getEdgeAttributes(Attribute.COLORS);
					Object ga = g.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
					if (ga != null) System.err.println("GA = "+ga.toString());
					if (ga == null || !( ga instanceof Dimension))	{
						// TODO make sure texture coordinates are not generated here!
						BallAndStickFactory bsf = new BallAndStickFactory(g);
						bsf.setMetric(currentMetric);
						bsf.setStickRadius(tubeRadius);
						bsf.setShowBalls(false);	// need to actually omit the balls
						if (cs != null) bsf.setStickColor(cs);
						bsf.update();
						handlingProxyGeometry = true;  
						SceneGraphComponent sgc = bsf.getSceneGraphComponent();
						sgc.setAppearance(RIBHelper.shiftTubesAppearance(dgs));
						sgc.getAppearance().setAttribute(CommonAttributes.RMAN_SURFACE_SHADER, eAppearance.getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.RMAN_SURFACE_SHADER, Appearance.DEFAULT));
						visit(sgc);
						handlingProxyGeometry = false;
					} else {
						int n = g.getNumEdges();
						double[][] crossSection = TubeUtility.octagonalCrossSection;
						// TODO make this official or get rid of it.
						Object lineShaderCrossSection = eAppearance.getAttribute("lineShader.crossSection", crossSection);
						if (lineShaderCrossSection  != crossSection)	{
							crossSection = (double[][]) lineShaderCrossSection;
						}
						for (int i = 0; i<n; ++i)	{
							if (edgec != null) {
								double[] edgecolor = edgec.item(i).toDoubleArray(null);
								ri.comment("Edge color");
								ri.color(edgecolor);
							}
							double[][] oneCurve = null;
							oneCurve = IndexedLineSetUtility.extractCurve(oneCurve, g, i);
							PolygonalTubeFactory ptf = new PolygonalTubeFactory(oneCurve);
							ptf.setCrossSection(crossSection);
							ptf.setMetric(currentMetric);
							ptf.setRadius(tubeRadius);
							ptf.update();
							IndexedFaceSet tube = ptf.getTube();
							handlingProxyGeometry = true;
							pointPolygon(tube, null);    
							handlingProxyGeometry = false;
						}
					}
				} else {
					// use "Curves" command to simulate no tubes
					// Renderman expects object coordinates for width of "Curves" so use tube parameter
					
					HashMap<String, Object> mappo = new HashMap<String, Object>();
					mappo.put("constantwidth", tubeRadius);
					int[][] ei = g.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null);
					int numEdges = ei.length;
					int[] nvertices = new int[numEdges];
					double[][] dv = g.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
					int fiber = dv[0].length;
					int totalV = 0;
					for (int i = 0; i<numEdges; ++i)	{
						nvertices[i] = ei[i].length;
						totalV += nvertices[i];
					}
					if (fiber == 4)	{
						dv = Pn.dehomogenize(new double[dv.length][3], dv);
						fiber = 3;
					}
					float[] vertices = new float[fiber*totalV];
					int counter = 0;
					for (int i = 0; i<numEdges; ++i)		{
						for (int j = 0; j<ei[i].length; ++j)	{
							int k = ei[i][j];
							for (int m = 0; m<fiber; ++m)  
								vertices[counter+m] = (float) dv[k][m];
							counter += fiber;
						}
					}
					mappo.put(fiber == 3 ? "P" : "vertex hpoint P",  vertices);					
					
					DataList eCol=g.getEdgeAttributes(Attribute.COLORS);
			        if(eCol!=null){
			        	int eColLength=GeometryUtility.getVectorLength(eCol);
			        	float[] eCs=new float[3*eCol.size()];
			        	float[] eOs=new float[3*eCol.size()];
			        	for(int i=0;i<eCol.size();i++){
			        		DoubleArray rgba = eCol.item(i).toDoubleArray();
			        		eCs[3*i] = (float) rgba.getValueAt(0);
			        		eCs[3*i+1] = (float) rgba.getValueAt(1);
			        		eCs[3*i+2] = (float) rgba.getValueAt(2);
			        		if (eColLength == 4) {
			        			eOs[3*i] = (float) rgba.getValueAt(3);
			        			eOs[3*i+1] = (float) rgba.getValueAt(3);
			        			eOs[3*i+2] = (float) rgba.getValueAt(3);
			        		}
			        	}
			        	mappo.put("uniform color Cs", eCs);
			        	if (eColLength == 4)
			        		mappo.put("uniform color Os", eOs);
			        }
					
					ri.curves("linear", nvertices, "nonperiodic", mappo);
				}   	        
			}
			
			DataList labelsList=g.getEdgeAttributes(Attribute.LABELS);
			if(labelsList!=null){
				if(dgs.getLineShader() instanceof DefaultLineShader){
					RIBHelper.createRIBLabel(g, (DefaultTextShader)((DefaultLineShader)dgs.getLineShader()).getTextShader(), this);
				}else 
					System.err.println("textshaders only for defaultshaders");
			}

			
			
		}
	/**
	 * The second argument here is a short-term solution to an apparent bug in
	 * the Renderman renderer which makes it impossible to pass the transparency
	 * ("Os") field to the pointspolygon command on a per-face basis. ("Cs"
	 * works on a per face basis but any value for "Os" (even all 1's
	 * corresponding to opaque surface) results in odd, incorrect results. -gunn
	 * 9.6.6
	 * 
	 * @param i
	 * @param color
	 */
	protected void _visit(IndexedFaceSet i) {
			ri.comment("IndexedFaceSet " + i.getName());
			DataList colors = i.getFaceAttributes(Attribute.COLORS);
			// if (colors !=null && currentOpacity != 1.0) {
			// the bug occurs when one attempts to set uniform colors or opacity
			boolean opaqueColors = true;
			if (colors != null && GeometryUtility.getVectorLength(colors) >= 3) {
				double[][] colorArray = colors.toDoubleArrayArray(null);
				if (colorArray[0].length > 3)
//					for (double[] cc : colorArray) {
//						if (cc[3] != 1.0)
							opaqueColors = false;
//					}
				if (!opaqueColors) {
					int nn = GeometryUtility.getVectorLength(colors);
					int numFaces = i.getNumFaces();
					float[][] colorArrayf = new float[numFaces][nn];
					for (int k = 0; k < numFaces; k++) {
						for (int j = 0; j < nn; j++)
							colorArrayf[k][j] = (float) colorArray[k][j];
					}
//					double[] m = getCurrentObjectToCamera();
//					currentPoints = i.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
//					if (currentPoints[0].length == 3) currentPoints = Pn.homogenize(null, currentPoints);
//					Pn.setToLength(currentPoints, currentPoints, 1.0, currentSignature);
//					currentPoints2 = Rn.matrixTimesVector(null,m, currentPoints);
//					System.err.println("point = "+Rn.toString(currentPoints2));
					IndexedFaceSet[] faceList = IndexedFaceSetUtility.splitIfsToPrimitiveFaces(i);
					for (int k = 0; k < numFaces; k++) {
						pointPolygon(faceList[k], colorArrayf[k]);
					}
				} 
				else pointPolygon(i,null);
			} else
					pointPolygon(i, null);

			DataList labelsList=i.getFaceAttributes(Attribute.LABELS);
			if(labelsList!=null){
				if(dgs.getPolygonShader() instanceof DefaultPolygonShader){
					RIBHelper.createRIBLabel(i, (DefaultTextShader)((DefaultPolygonShader)dgs.getPolygonShader()).getTextShader(), this);
				}else 
					System.err.println("textshaders only for defaultshaders");
			}
			
	}

	/**
	 * The second argument non-null means we're doing\ a work-around of a
	 * problem in Renderman prman renderer related to non-opaque per-face
	 * colors.
	 * 
	 * @param ifs
	 * @param color
	 */
	protected void pointPolygon(IndexedFaceSet ifs, float[] color) {
		int npolys = ifs.getNumFaces();
		if (color != null && color.length == 4 && color[3] == 0.0 && ignoreAlpha0) return;
		if (npolys != 0) {
			boolean isQuadMesh = false;
			Dimension qmDim = null;
			Object foo = ifs.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
			if (foo != null && foo instanceof Dimension)	{
				qmDim = (Dimension) foo;
				isQuadMesh = true;
			}
			HashMap<String, Object> map = new HashMap<String, Object>();
			DataList coords = ifs.getVertexAttributes(Attribute.COORDINATES);
			DoubleArrayArray da = coords.toDoubleArrayArray();
			int pointlength = GeometryUtility.getVectorLength(coords);
			// We'd like to be able to use the "Pw" attribute which accepts
			// 4-vectors for point coordinates, but 3Delight-5.0.1
			// does not support it ...
			// As of now, the prman renderer seems unable to handle the "Pw"
			// parameter correctly.
			// See
			// https://renderman.pixar.com/forum/showthread.php?s=&threadid=5935&highlight=tuberlin
			// for a bug description
			// As a result, we set hasPw to false.
			double[] o2w = object2world.getMatrix(null);
			double[] rmanc = P3.makeScaleMatrix(null, 1, 1, -1);
			double[] o2c = Rn.times(null, Rn.times(null, rmanc, world2Camera),o2w);
			if (!hasPw || pointlength == 3) {
				float[] fcoords = new float[3 * da.getLength()];
				for (int j = 0; j < da.getLength(); j++) {
					if (pointlength == 4) {
						float w = (float) da.getValueAt(j, 3);
						if (w != 0)
							w = 1.0f / w;
						else
							w = 10E10f; // hack! but what else can you do?
						fcoords[3 * j + 0] = (float) da.getValueAt(j, 0) * w;
						fcoords[3 * j + 1] = (float) da.getValueAt(j, 1) * w;
						fcoords[3 * j + 2] = (float) da.getValueAt(j, 2) * w;
					} else {
						fcoords[3 * j + 0] = (float) da.getValueAt(j, 0);
						fcoords[3 * j + 1] = (float) da.getValueAt(j, 1);
						fcoords[3 * j + 2] = (float) da.getValueAt(j, 2);
					}
				}
				map.put("P", fcoords);
				double[][] dpoints = da.toDoubleArrayArray(null);
			} else if (pointlength == 4) {
				float[] fcoords = new float[4 * da.getLength()];
				for (int j = 0; j < da.getLength(); j++) {
					fcoords[4 * j + 0] = (float) da.getValueAt(j, 0);
					fcoords[4 * j + 1] = (float) da.getValueAt(j, 1);
					fcoords[4 * j + 2] = (float) da.getValueAt(j, 2);
					fcoords[4 * j + 3] = (float) da.getValueAt(j, 3);
				}
				map.put("vertex hpoint P", fcoords);
			}
			DataList normals = null;
			boolean vertexNormals = true;
			if (smooth && ifs.getVertexAttributes(Attribute.NORMALS) != null)
				normals = ifs.getVertexAttributes(Attribute.NORMALS);
			else if (ifs.getFaceAttributes(Attribute.NORMALS) != null)
			{vertexNormals = false; normals = ifs.getFaceAttributes(Attribute.NORMALS);}
			int n;
			if ( normals != null) {
				da = normals.toDoubleArrayArray();
				n = da.getLengthAt(0);
				if (n == 4 && currentMetric == Pn.EUCLIDEAN) {
//					throw new IllegalStateException(
//							"4D normals only valid with non-euclidean metric");
				}
				if (currentMetric == Pn.EUCLIDEAN) {
					float[] fnormals = new float[3 * da.getLength()];
					for (int j = 0; j < da.getLength(); j++) {
						fnormals[n * j + 0] = (float) da.getValueAt(j, 0);
						fnormals[n * j + 1] = (float) da.getValueAt(j, 1);
						fnormals[n * j + 2] = (float) da.getValueAt(j, 2);
					}
					// Note: using "Np" for uniform normal here causes problems in some files.
					map.put(vertexNormals ? "N" : "uniform normal N", fnormals);
					//map.put(type + "vector N", fnormals);
				} else {
					// in noneuclidean case we have to use 4D vectors and ship them over as
					// float[] type.
					float[] fnormals = new float[4 * da.getLength()];
					double[][] dnormals = da.toDoubleArrayArray(null);
					double[] m = getCurrentObjectToCamera();
					double[][] dnormals2 = Rn.matrixTimesVector(null,m, dnormals);
//					System.err.println("normals = "+Rn.toString(dnormals2));
					int nn = dnormals[0].length;
					int[][] ind = ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
					for (int ii = 0, j = 0; j < dnormals.length; ++j) {
//						if (currentPoints2 != null)	{
//							for (int jj = 0; jj<ind[0].length; ++jj) {
//							System.err.println("P.N="+Pn.innerProduct(currentPoints[ind[0][jj]], dnormals[j], currentSignature));						
//							System.err.println("TP.N="+Pn.innerProduct(currentPoints2[ind[0][jj]], dnormals2[j], currentSignature));						
//							}
//							}
						for (int k = 0; k < 4; ++k)
							if (k < nn) fnormals[ii++] = (float) dnormals[j][k];
							else fnormals[ii++] = 0f;
//						System.err.println("P.N="+Pn.innerProduct(dpoints[j], dnormals[j], currentSignature));
					}
					map.put(((vertexNormals) ? "vertex" : "uniform") + " float[4] Nw", fnormals);
				}
			}
			// texture coords:
			DataList texCoords = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES);
			if (texCoords != null) {
				float[] ftex = new float[2 * texCoords.size()];
				for (int j = 0; j < texCoords.size(); j++) {
					// ftex[j] =(float)d.getValueAt(j);
					DoubleArray l = texCoords.item(j).toDoubleArray();

					ftex[2 * j] = (float) l.getValueAt(0);
					ftex[2 * j + 1] = (float) l.getValueAt(1);
					// ftex[2*j] =(float)d.getValueAt(j,0);
					// ftex[2*j+1] =(float)d.getValueAt(j,1);
				}
				map.put("st", ftex);
			}

			DataList vertexColors = ifs.getVertexAttributes(Attribute.COLORS);
			DataList faceColors = ifs.getFaceAttributes(Attribute.COLORS);
			if ((smooth || (color == null && faceColors == null)) && vertexColors != null) {
				int vertexColorLength = GeometryUtility
				.getVectorLength(vertexColors);
				float[] vCol = new float[3 * vertexColors.size()];
				float[] vOp = null;
				if (vertexColorLength == 4)
					vOp = new float[3 * vertexColors.size()];
				for (int j = 0; j < vertexColors.size(); j++) {
					// ftex[j] =(float)d.getValueAt(j);
					DoubleArray rgba = vertexColors.item(j).toDoubleArray();

					vCol[3 * j] = (float) rgba.getValueAt(0);
					vCol[3 * j + 1] = (float) rgba.getValueAt(1);
					vCol[3 * j + 2] = (float) rgba.getValueAt(2);
					if (vertexColorLength == 4) {
						vOp[3 * j] = (float) rgba.getValueAt(3)*currentOpacity;
						vOp[3 * j + 1] = (float) rgba.getValueAt(3)*currentOpacity;
						vOp[3 * j + 2] = (float) rgba.getValueAt(3)*currentOpacity;
					}
					// ftex[2*j] =(float)d.getValueAt(j,0);
					// ftex[2*j+1] =(float)d.getValueAt(j,1);
				}
				map.put("varying color Cs", vCol);
				if (vertexColorLength == 4)
					map.put("varying color Os", vOp);
			} else if (faceColors != null) {
				int faceColorLength = GeometryUtility
				.getVectorLength(faceColors);
				float[] vCol = new float[3 * faceColors.size()];
				float[] vOp = null;
				if (faceColorLength == 4)
					vOp = new float[3 * faceColors.size()];
				for (int j = 0; j < faceColors.size(); j++) {
					// ftex[j] =(float)d.getValueAt(j);
					DoubleArray rgba = faceColors.item(j).toDoubleArray();

					vCol[3 * j] = (float) rgba.getValueAt(0);
					vCol[3 * j + 1] = (float) rgba.getValueAt(1);
					vCol[3 * j + 2] = (float) rgba.getValueAt(2);
					if (faceColorLength == 4) {
						float alpha = (float) rgba.getValueAt(3) * currentOpacity;
						vOp[3 * j] = vOp[3 * j + 1] = vOp[3 * j + 2] = alpha;
					}
				}
				map.put("uniform color Cs", vCol);
				// Following is a bug in 12.5 prman so we have to simply avoid
				// it.
				//if (false)
				if (faceColorLength == 4)
					//map.put("uniform color Os", vOp);
					// seems that a work-around is to leave out "uniform color"
					map.put("Os", vOp);
			}

			int[] nvertices = new int[npolys];
			int verticesLength = 0;
			for (int k = 0; k < npolys; k++) {
				IntArray fi = ifs.getFaceAttributes(Attribute.INDICES).item(k).toIntArray();
				nvertices[k] = fi.getLength();
				verticesLength += nvertices[k];
			}
			int[] vertices = new int[verticesLength];
			int l = 0;
			for (int k = 0; k < npolys; k++) {
				for (int m = 0; m < nvertices[k]; m++, l++) {
					IntArray fi = ifs.getFaceAttributes(Attribute.INDICES).item(k).toIntArray();
					vertices[l] = fi.getValueAt(m);
				}
			}
			// TODO figure out work-around: this doesn't work
			// with PRMAN 12.5
			// Since there is a bug in PRMAN 12.5: see RenderMan Forums
			// https://renderman.pixar.com/forum/showthread.php?s=&threadid=5935&highlight=tuberlin
			// the following implements the work-around mentioned above caused
			// by problems with
			// setting opacity as a uniform color parameter inside the geometry
			if (color != null) {
				float[] f = new float[3];
				ri.color(new float[] { color[0], color[1], color[2] });
				float thisOpacity = currentOpacity;
				if (color.length == 4) {
					thisOpacity = color[3] * currentOpacity;
					f[0] = f[1] = f[2] = thisOpacity;
					ri.opacity(f);
				}
			}
			if (isQuadMesh)
				ri.patchMesh("bilinear", qmDim.width, false,qmDim.height, false, map);
			else 
				ri.pointsPolygons(npolys, nvertices, vertices, map);
		}
	}

	// TODO figure out if this works!
	public void visit(ClippingPlane p) {
//		ri.clippingPlane(0, 0, 1, 0, 0, 0);
	}

	public void visit(Sphere s) {
		if (hasProxy(s))return;
		RIBHelper.processShader(dgs.getPolygonShader(), this, "polygonShader");
		ri.sphere(1f, -1f, 1f, 360f, null);
	}

	public void visit(Cylinder c) {
		if (hasProxy(c))return;
		RIBHelper.processShader(dgs.getPolygonShader(), this, "polygonShader");
		ri.cylinder(1f, -1f, 1f, 360f, null);
		// TODO Decide whether a jReality Cylinder is closed or not!
		ri.disk(-1f, 1f, 360f, null);
		ri.disk(1f, 1f, 360f, null);

	}

	/**
	 * @return Returns the height.
	 */
	public int getHeight() {
		return height;
	}

	public void setHeight(int height) {
		this.height = height;
	}

	public int getWidth() {
		return width;
	}

	public void setWidth(int width) {
		this.width = width;
	}

	/**
	 * @param maximumEyeSplits.
	 * @deprecated Use {@link CommonAttributes#RMAN_MAX_EYE_SPLITS} in root
	 *             Appearance
	 */
	public void setMaximumEyeSplits(int maximumEyeSplits) {
		this.maximumEyeSplits[0] = maximumEyeSplits;
	}

	/**
	 * The beginning of support for different renderers: currently not used for
	 * anything
	 * 
	 * @param rendererType
	 */
	public void setRendererType(int rendererType) {
		this.rendererType = rendererType;
	}

	public int getRendererType() {
		return rendererType;
	}

	public String writeCubeMap(CubeMap reflectionMap) {		
		String noSuffix = cubeMaps.get(reflectionMap.getTop());
		if (noSuffix == null) {
			String cubeMapFileName = "_cubeMap" + (cubeMapCount++);
			noSuffix = ribFileName + cubeMapFileName;
			cubeMaps.put(reflectionMap.getTop(), noSuffix);
			String top = new File(writeTexture(reflectionMap.getTop(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			String bottom = new File(writeTexture(reflectionMap.getBottom(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			String left = new File(writeTexture(reflectionMap.getLeft(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			String right = new File(writeTexture(reflectionMap.getRight(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			String front = new File(writeTexture(reflectionMap.getFront(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			String back = new File(writeTexture(reflectionMap.getBack(),
					Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE))
			.getName();
			renderScript.addReflectionMap(cubeMapFileName, back, front, bottom,
					top, left, right);
		}
		return noSuffix + "." + cubeMapFileSuffix;
	}


	public double[] getCurrentObjectToCamera() {
		double[] o2w = object2world.getMatrix(null);
		double[] rmanc = P3.makeScaleMatrix(null, 1, 1, -1);
		double[] o2c = Rn.times(null, Rn.times(null, rmanc, world2Camera), o2w);
		return o2c;
	}
}
