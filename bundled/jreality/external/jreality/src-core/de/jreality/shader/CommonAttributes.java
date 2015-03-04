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


package de.jreality.shader;

import java.awt.Color;
import java.awt.Font;

import javax.swing.SwingConstants;

import de.jreality.geometry.FrameFieldType;
import de.jreality.scene.Appearance;

/**
 * Standard attributes which appear in {@link Appearance} instances.
 * <p>
 * Since Appearance is based on <String, value> pairs, it's important to make sure that
 * you are using the right strings as keys.  This class provides a list of all the standard
 * attributes to help avoid mistakes in typing, etc.
 * @author Charles Gunn
 *
 */public class CommonAttributes {
    
  private CommonAttributes() {}

	// goes in root appearance: first set controls how the background of generated image appears
  	public static final String SMALL_OBJ_OPTIMIZATION = "smallObjectsOptimization";
  	public static final String STEREOGRAM_RENDERING = "stereogramRendering";
  	public static final String STEREOGRAM_NUM_SLICES = "stereogramNumSlices";
  	
  	public static final boolean SMALL_OBJ_OPTIMIZATION_DEFAULT = true;
  	public static final boolean STEREOGRAM_RENDERING_DEFAULT = false;
  	public static final int STEREOGRAM_NUM_SLICES_DEFAULT = 2;
  	
  	public final static String BACKGROUND_COLOR = 	"backgroundColor";
    public final static Color  BACKGROUND_COLOR_DEFAULT = new java.awt.Color(225, 225, 225);
	public final static String BACKGROUND_COLORS = 	"backgroundColors";		// array of four colors for corners of background
	// correct handling of background colors with stereo cameras requires stretching it to cover both views
	public final static String BACKGROUND_COLORS_STRETCH_X = "backgroundColorsStretchX";
	public final static String BACKGROUND_COLORS_STRETCH_Y = "backgroundColorsStretchY";
	public final static String BACKGROUND_TEXTURE2D = "backgroundTexture2D";		// value is a Texture2D
	public final static String SKY_BOX = "skyBox";	// the value is a de.jreality.shader.CubeMap
	
	public final static String FOG_ENABLED = 	"fogEnabled";
    public final static boolean  FOG_ENABLED_DEFAULT = false;
    public final static String FOG_COLOR = "fogColor";
    public final static String FOG_DENSITY = "fogDensity";
    public final static double FOG_DENSITY_DEFAULT = 0.1; 
    // rendering hints that have to be in the root appearance to take effect
	public static final String ONE_TEXTURE2D_PER_IMAGE = "oneTexture2DPerImage";	// allows optimizing performance if true; default: false
	public static final String CLEAR_COLOR_BUFFER = "clearColorBuffer";
	public final static String FORCE_RESIDENT_TEXTURES = "forceResidentTextures";
	public final static String USE_OLD_TRANSPARENCY = "useOldTransparency";		// use the alpha channels of shader colors: pre-2006
	public final static String ANTI_ALIASING_FACTOR = "antiAliasingFactor";
	public final static int ANTI_ALIASING_FACTOR_DEFAULT = 2;
    // rendering hints that can appear anywhere in the scene graph
	public final static String LIGHTING_ENABLED 	= 		"lightingEnabled";
	public final static boolean LIGHTING_ENABLED_DEFAULT = true;
	public final static String ANTIALIASING_ENABLED = 	"antiAliasing";
	public final static boolean ANTIALIASING_ENABLED_DEFAULT = 	false;
	public final static String TRANSPARENCY_ENABLED = 	"transparencyEnabled";
    public static final String ADDITIVE_BLENDING_ENABLED = "additiveBlendingEnabled";
	public final static String Z_BUFFER_ENABLED = 		"zBufferEnabled";
	public final static String LEVEL_OF_DETAIL = 			"levelOfDetail";
	public final static double LEVEL_OF_DETAIL_DEFAULT = 			0.0;
	public final static String OPAQUE_TUBES_AND_SPHERES = "opaqueTubesAndSpheres";
	public final static boolean OPAQUE_TUBES_AND_SPHERES_DEFAULT = false;
	public final static String CENTER_ON_BOUNDING_BOX = "centerOnBoundingBox";
	public static final String BACKEND_RETAIN_GEOMETRY = "rendermanRetainGeometry"; // a hint to backends to write one copy of geometry and re-use it
	public final static String LOCAL_LIGHT_MODEL = "localLightModel";
	public final static String RADII_WORLD_COORDINATES = "radiiWorldCoordinates";
	public final static boolean RADII_WORLD_COORDINATES_DEFAULT  = false;
	// these hints are heavily OpenGL specific
	public final static String DEPTH_FUDGE_FACTOR = 		"depthFudgeFactor";
	public final static String IGNORE_ALPHA0	=			"ignoreAlpha0";	// reject pixel writes for pixels with alpha == 0
	public final static String BACK_FACE_CULLING_ENABLED = 		"backFaceCulling";
	public final static String FLIP_NORMALS_ENABLED = 		"flipNormals";
	public final static String MANY_DISPLAY_LISTS = "manyDisplayLists";		// if true, one display list per scene graph path
	public final static String ANY_DISPLAY_LISTS = "anyDisplayLists";		// if true, use Display lists.
	public final static String SEPARATE_SPECULAR_COLOR = "separateSpecularColor";
	public final static String COMPONENT_DISPLAY_LISTS = "componentDisplayLists";
	public final static String USE_GLSL = "useGLSL";
	// default geometry shader
	public final static String FACE_DRAW = 		"showFaces";
	public final static boolean FACE_DRAW_DEFAULT = true;
	public final static String EDGE_DRAW = 		"showLines";
	public final static boolean EDGE_DRAW_DEFAULT = true;
	public final static String VERTEX_DRAW = 		"showPoints";
	public final static boolean VERTEX_DRAW_DEFAULT = true;
	public final static String POINT = 		"point";
	public final static String LINE = 		"line";
	public final static String POLYGON = 	"polygon";
	public final static String VERTEX = 	"vertex";
	public final static String VOLUME = 	"volume";
	public final static String TEXT =		"text";
	private final static String SHADER = "Shader";
	public final static String POINT_SHADER = 		POINT+SHADER;
	public final static String LINE_SHADER = 		LINE+SHADER;
	public final static String POLYGON_SHADER = 	POLYGON+SHADER;
	public final static String VERTEX_SHADER = 	VERTEX+SHADER;
	public final static String VOLUME_SHADER = 	VOLUME+SHADER;
	public final static String TEXT_SHADER = 	TEXT+SHADER;
	// default point shader
	public final static String SPRITES_DRAW = 	"spritesDraw";	// applies only when spheres = false
	public final static String SPHERES_DRAW = 	"spheresDraw";
	public final static boolean SPHERES_DRAW_DEFAULT = true;
	public final static String POINT_RADIUS = 	"pointRadius";			// object coordinates
    public final static double POINT_RADIUS_DEFAULT = 0.025;
	public final static String POINT_SIZE = 	"pointSize";				// pixel coordinates
    public final static double POINT_SIZE_DEFAULT = 30.0;
	public static final Color POINT_DIFFUSE_COLOR_DEFAULT = Color.RED;
	public final static String SPHERE_RESOLUTION = "sphereResolution";
	public final static String ATTENUATE_POINT_SIZE = "attenuatePointSize"; // perhaps belongs in rendering hints
	public final static boolean ATTENUATE_POINT_SIZE_DEFAULT = true;
	
  // default line shader
	public final static String TUBES_DRAW = 		"tubeDraw";
	public final static boolean TUBES_DRAW_DEFAULT = true;
	public final static String TUBE_RADIUS = 		"tubeRadius";		// object coordinates
    public final static double TUBE_RADIUS_DEFAULT = 0.025;
	public final static String TUBE_STYLE = 		"tubeStyle";		// parallel or frenet?
    public final static FrameFieldType TUBE_STYLE_DEFAULT = FrameFieldType.PARALLEL;
	public final static String VERTEX_COLORS_ENABLED = 		"vertexColors";	// get colors from vertices?
    public final static boolean VERTEX_COLORS_ENABLED_DEFAULT = false;
	public static final String SMOOTH_LINE_SHADING = "smoothLineShading";	
	public static final boolean SMOOTH_LINE_SHADING_DEFAULT = false;	// if true, then interpolate vertex colors
	public final static String LINE_WIDTH = 		"lineWidth";			// pixel coordinates
    public final static double LINE_WIDTH_DEFAULT = 1.0;
	public final static String NORMAL_SCALE = 		"normalScale";
	public final static String LINE_STIPPLE = 		"lineStipple";		// openGL line drawing options
	public final static String LINE_FACTOR = 		"lineFactor";
	public final static String LINE_STIPPLE_PATTERN = "lineStipplePattern";
	public static final Color LINE_DIFFUSE_COLOR_DEFAULT = Color.BLACK;
	public final static String POINT_SPRITE = "pointSprite";
	public final static String LINE_LIGHTING_ENABLED 	= 		"lineLighting";
	public final static boolean LINE_LIGHTING_ENABLED_DEFAULT = false;
	// default polygon shader
	public final static String SMOOTH_SHADING = 	"smoothShading";		// interpolate vertex shading values?
	public final static boolean SMOOTH_SHADING_DEFAULT = true;
	public final static String TEXTURE_2D = 		"texture2d";		
	public final static String TEXTURE_2D_1 = 		"texture2d[1]";		
	public final static String TEXTURE_2D_2 = 		"texture2d[2]";		
	public final static String TEXTURE_2D_3 = 		"texture2d[3]";		
	public static final String REFLECTION_MAP = "reflectionMap";
	public final static String TRANSPARENCY = 		"transparency";		
    public final static double TRANSPARENCY_DEFAULT =  0.0;
	public final static String AMBIENT_COLOR = 	"ambientColor";
    public final static Color  AMBIENT_COLOR_DEFAULT = Color.WHITE;
	public final static String DIFFUSE_COLOR = 	"diffuseColor";
    public final static Color  DIFFUSE_COLOR_DEFAULT = Color.BLUE;
	public final static String SPECULAR_COLOR = 	"specularColor";
    public final static Color SPECULAR_COLOR_DEFAULT = Color.WHITE;    
    public final static String SPECULAR_EXPONENT =  "specularExponent";
    public final static double SPECULAR_EXPONENT_DEFAULT =  60.;
    public final static String AMBIENT_COEFFICIENT =  "ambientCoefficient";
    public final static double AMBIENT_COEFFICIENT_DEFAULT =  .0;
    public final static String DIFFUSE_COEFFICIENT =  "diffuseCoefficient";
    public final static double DIFFUSE_COEFFICIENT_DEFAULT =  1.0;
    public final static String SPECULAR_COEFFICIENT =  "specularCoefficient";
    public final static double SPECULAR_COEFFICIENT_DEFAULT =  .7;
	public static final String LIGHT_DIRECTION = "lightDirection";
	// implode polygon shader
	public static final String IMPLODE = "implode";
	public static final String IMPLODE_FACTOR = "implodeFactor";
	public static final double IMPLODE_FACTOR_DEFAULT = 0.6;
	
	// default text shader:  New versions include "TEXT_" to make clear the scope of these constants
//	@Deprecated
	public static final String SCALE = "scale";
//	@Deprecated
	public static final String OFFSET = "offset";
//	@Deprecated
	public static final String ALIGNMENT = "alignment";
//	@Deprecated
	public static final String FONT	= "font";
	public static final String TEXT_SCALE = "scale";
	public static final double TEXT_SCALE_DEFAULT = 0.01;
	public static final String TEXT_OFFSET = "offset";
	public static final String TEXT_ALIGNMENT = "alignment";
	public static final int TEXT_ALIGNMENT_DEFAULT = SwingConstants.NORTH_EAST;
	public static final double[] TEXT_OFFSET_DEFAULT = new double[]{0d,0d,0d};
	public static final String TEXT_FONT	= "font";
	public static final String TEXT_COLOR	= TEXT_SHADER+".diffuseColor";
	
	// miscellaneous
	public static final String RENDER_S3 = "renderS3";
	public static final String PICKABLE = "pickable";
	/** @deprecated  Use {@link #METRIC}.*/
	public static final String SIGNATURE= "metric";  
	public static final String METRIC= "metric";
	public static final String INFO_STRING = "infoString";
	public static final String GLSL = "glsl";
	
	// RenderMan backend attributes
	public final static String RMAN_ATTRIBUTE = "rendermanAttribute";
	public final static String RMAN_SHADOWS_ENABLED = "rendermanShadowsEnabled";
    public static final String RMAN_RAY_TRACING_REFLECTIONS = "rendermanRayTracingReflectionsEnabled";
    public static final String RMAN_RAY_TRACING_VOLUMES="rendermanRayTracingVolumesEnabled";
  
    public final static String RMAN_SURFACE_SHADER = "rendermanSurfaceShader";
    public static final String RMAN_SL_SHADER = "rendermanSLShader";	
	public final static String RMAN_DISPLACEMENT_SHADER = "rendermanDisplacementShader";
    public final static String RMAN_IMAGER_SHADER = "rendermanImagerShader";
    public static final String RMAN_VOLUME_EXTERIOR_SHADER = "rendermanVolumeExteriorShader"; 
    public static final String RMAN_VOLUME_INTERIOR_SHADER = "rendermanVolumeInteriorShader"; 
    public static final String RMAN_VOLUME_ATMOSPHERE_SHADER = "rendermanVolumeAtmosphereShader"; 
    public final static String RMAN_LIGHT_SHADER = "rendermanLightShader";
    public final static String RMAN_SEARCHPATH_SHADER = "rendermanSearchpathShader";
  
	public final static String RMAN_TEXTURE_FILE = "rendermanTexFile";
    public static final String RMAN_TEXTURE_FILE_SUFFIX = "rendermanTextureFileSuffix";
	public static final String RMAN_REFLECTIONMAP_FILE = "rendermanReflectionmapFile";
	public final static String RMAN_GLOBAL_INCLUDE_FILE = "rendermanGlobalIncludeFile";	
	public static final String RMAN_OUTPUT_DISPLAY_FORMAT = "rendermanOutputDisplayFormat";
	public static final String RMAN_PROXY_COMMAND = "rendermanProxyCommand";
	public static final String RMAN_ARCHIVE_CURRENT_NODE = "rendermanArchiveCurrentNode";
	public static final String RMAN_MAX_EYE_SPLITS  = "rendermanMaxEyeSplits";

  
	public static final String HAPTIC_SHADER = "hapticShader";
	public static final String HAPTIC_ENABLED = "hapticEnabled";
	public static final String HAPTIC_TOUCHABLE_FRONT = "hapticToutchableFront";
	public static final boolean HAPTIC_TOUCHABLE_FRONT_DEFAULT = true;
	public static final String HAPTIC_TOUCHABLE_BACK = "hapticToutchableBack";
	public static final boolean HAPTIC_TOUCHABLE_BACK_DEFAULT = false;
	public static final boolean HAPTIC_ENABLED_DEFAULT = false;
	public static final String HAPTIC_STIFFNESS = "stiffness";
	public static final double HAPTIC_STIFFNESS_DEFAULT = 0.3;
	public static final String HAPTIC_DAMPING = "damping";
	public static final double HAPTIC_DAMPING_DEFAULT = 0.1;
	public static final String HAPTIC_STATIC_FRICTION = "staticFriction";
	public static final double HAPTIC_STATIC_FRICTION_DEFAULT = 0.2;
	public static final String HAPTIC_DYNAMIC_FRICTION = "dynamicFriction";
	public static final double HAPTIC_DYNAMIC_FRICTION_DEFAULT = 0.3;
	
	final static Boolean SHOW_LABELS_DEFAULT = Boolean.TRUE;
	final static String SHOW_LABELS = "showLabels";
	
	
	public static Object getDefault(String key, Object value){
		if(key.equals(SHOW_LABELS))
			return SHOW_LABELS_DEFAULT;
		if(key.equals("reflectionMap"))
			return false;
		if(key.equals("ambientColor"))
			return AMBIENT_COLOR_DEFAULT;
		if(key.equals("diffuseColor"))
			return DIFFUSE_COLOR_DEFAULT;
		if(key.equals("specularColor"))
			return SPECULAR_COLOR_DEFAULT;
		if(key.equals("specularCoefficient"))
			return SPECULAR_COEFFICIENT_DEFAULT;
		if(key.equals("diffuseCoefficient"))
			return DIFFUSE_COEFFICIENT_DEFAULT;
		if(key.equals("ambientCoefficient"))
			return AMBIENT_COEFFICIENT_DEFAULT;
		
		if(key.equals("specularExponent"))
			return SPECULAR_EXPONENT_DEFAULT;
		
		if(key.equals("backgroundColors"))
			return BACKGROUND_COLOR_DEFAULT;
		if(key.equals("fogEnabled"))
			return FOG_ENABLED_DEFAULT;
		if(key.equals("fogDensity"))
			return FOG_DENSITY_DEFAULT;
		if(key.equals("levenOfDetail"))
			return LEVEL_OF_DETAIL_DEFAULT;
		if(key.equals("opaqueTubesAndSpheres"))
			return OPAQUE_TUBES_AND_SPHERES_DEFAULT;
		if(key.equals("radiiWorldCoordinates"))
			return RADII_WORLD_COORDINATES_DEFAULT;
		if(key.equals("spheresDraw"))
			return SPHERES_DRAW_DEFAULT;
		if(key.equals("pointRadius"))
			return POINT_RADIUS_DEFAULT;
		
		if(key.equals("pointSize"))
			return POINT_SIZE_DEFAULT;
		if(key.equals("pointDiffuseColor"))
			return POINT_DIFFUSE_COLOR_DEFAULT;
		if(key.equals("attenuatePointSize"))
			return ATTENUATE_POINT_SIZE_DEFAULT;
		if(key.equals("pointRadius"))
			return POINT_RADIUS_DEFAULT;
		
		if(key.equals("tubesDraw"))
			return TUBES_DRAW_DEFAULT;
		if(key.equals("tubeRadius"))
			return TUBE_RADIUS_DEFAULT;
		if(key.equals(TUBE_STYLE))
			return TUBE_STYLE_DEFAULT;
		if(key.equals("VERTEX_COLORS_ENABLED"))
			return VERTEX_COLORS_ENABLED_DEFAULT;
		
		if(key.equals(SMOOTH_LINE_SHADING))
			return SMOOTH_LINE_SHADING_DEFAULT;
		if(key.equals(LINE_WIDTH))
			return LINE_WIDTH_DEFAULT;
		if(key.equals("lineDiffuseColor"))
			return LINE_DIFFUSE_COLOR_DEFAULT;
		if(key.equals(SMOOTH_SHADING))
			return SMOOTH_SHADING_DEFAULT;
		if(key.equals(CommonAttributes.VERTEX_COLORS_ENABLED))
			return VERTEX_COLORS_ENABLED_DEFAULT;
		if(key.equals(TRANSPARENCY))
			return TRANSPARENCY_DEFAULT;
		if(key.equals(LINE_LIGHTING_ENABLED))
			return LINE_LIGHTING_ENABLED_DEFAULT;
		if(key.equals(LIGHTING_ENABLED))
			return LIGHTING_ENABLED_DEFAULT;
		
		if(key.equals(FONT))
			return new Font(Font.SANS_SERIF, Font.BOLD, 30);
		if(key.equals(TEXT_SCALE))
			return TEXT_SCALE_DEFAULT;
		if(key.equals(TEXT_OFFSET))
			return TEXT_OFFSET_DEFAULT;
		if(key.equals(TEXT_ALIGNMENT))
			return TEXT_ALIGNMENT_DEFAULT;
		if(key.equals(SMALL_OBJ_OPTIMIZATION))
			return SMALL_OBJ_OPTIMIZATION_DEFAULT;
		if(key.equals(STEREOGRAM_RENDERING))
			return STEREOGRAM_RENDERING_DEFAULT;
		if(key.equals(STEREOGRAM_NUM_SLICES))
			return STEREOGRAM_NUM_SLICES_DEFAULT;
		if(key.equals(ANTI_ALIASING_FACTOR))
			return ANTI_ALIASING_FACTOR_DEFAULT;
		if(key.equals(ANTIALIASING_ENABLED))
			return ANTIALIASING_ENABLED_DEFAULT;
		
		return value;
	}
}
