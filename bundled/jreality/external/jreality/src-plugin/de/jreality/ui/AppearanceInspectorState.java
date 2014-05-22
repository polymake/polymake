package de.jreality.ui;

import java.awt.Color;
import java.util.HashMap;

public class AppearanceInspectorState {

	public boolean DEFAULT_SHOW_POINTS = true;
	public boolean DEFAULT_SHOW_POINT_LABELS = true;
	public boolean DEFAULT_POINTS_REFLECTING = true;
	public double DEFAULT_POINT_RADIUS = .5;
	public Color DEFAULT_POINT_COLOR = Color.blue;
	public Color DEFAULT_POINT_LABEL_COLOR = Color.black;
	public double DEFAULT_POINT_LABEL_SIZE = 0.5;
	public int DEFAULT_POINT_LABEL_RESOLUTION = 48;
	public boolean DEFAULT_SHOW_LINES = true;
	public boolean DEFAULT_SHOW_LINE_LABELS = true;
	public boolean DEFAULT_LINES_REFLECTING = true;
	public double DEFAULT_TUBE_RADIUS = .4;
	public Color DEFAULT_LINE_COLOR = Color.red;
	public Color DEFAULT_LINE_LABEL_COLOR = Color.black;
	public double DEFAULT_LINE_LABEL_SIZE = 0.5;
	public int DEFAULT_LINE_LABEL_RESOLUTION = 48;
	public boolean DEFAULT_SHOW_FACES = true;
	public boolean DEFAULT_SHOW_FACE_LABELS = true;
	public boolean DEFAULT_FACES_REFLECTING = true;
	public double DEFAULT_FACE_REFLECTION = .5;
	public double DEFAULT_LINE_REFLECTION = .3;
	public double DEFAULT_POINT_REFLECTION = .3;
	public Color DEFAULT_FACE_COLOR = Color.white;
	public Color DEFAULT_FACE_LABEL_COLOR = Color.black;
	public double DEFAULT_FACE_LABEL_SIZE = 0.5;
	public int DEFAULT_FACE_LABEL_RESOLUTION = 48;
	public boolean DEFAULT_TRANSPARENCY_ENABLED = false;
	public double DEFAULT_TRANSPARENCY = .7;
	public boolean DEFAULT_FACES_FLAT = false;
	public boolean DEFAULT_TUBES = true;
	public boolean DEFAULT_SPHERES = true;
	public String DEFAULT_TEXTURE = "none";

	private HashMap<String, String> textures = new HashMap<String, String>();

	private final AppearanceInspector appearanceInspector;

	public AppearanceInspectorState(AppearanceInspector appInspector) {
		appearanceInspector = appInspector;
		textures.put("1 None", null);
		textures.put("2 Metal Grid", "textures/boysurface.png");
		textures.put("3 Metal Floor", "textures/metal_basic88.png");
		textures.put("4 Chain-Link Fence", "textures/chainlinkfence.png");
	}
	
	public HashMap<String, String> getTextures() {
		return textures = new HashMap<String, String>();
	}

	public void setTextures(HashMap<String, String> textures) {
		this.textures = textures;
	}

	public HashMap<String, Object> getState() {
		HashMap<String, Object> state = new HashMap<String, Object>();
		state.put("showPoints", appearanceInspector.isShowPoints());
		state.put("showPointLabels", appearanceInspector.isShowPointLabels());
		state.put("pointsReflecting", appearanceInspector.isPointsReflecting());
		state.put("pointReflection", appearanceInspector.getPointReflection());
		state.put("pointRadius", appearanceInspector.getPointRadius());
		state.put("pointColor", appearanceInspector.getPointColor());
		state.put("pointLabelColor", appearanceInspector.getPointLabelColor());
		state.put("pointLabelSize", appearanceInspector.getPointLabelSize());
		state.put("pointLabelResolution", appearanceInspector.getPointLabelResolution());
		
		state.put("showLines", appearanceInspector.isShowLines());
		state.put("showLineLabels", appearanceInspector.isShowLineLabels());
		state.put("linesReflecting", appearanceInspector.isLinesReflecting());
		state.put("lineReflection", appearanceInspector.getLineReflection());
		state.put("tubeRadius", appearanceInspector.getTubeRadius());
		state.put("lineColor", appearanceInspector.getLineColor());
		state.put("lineLabelColor", appearanceInspector.getLineLabelColor());
		state.put("lineLabelSize", appearanceInspector.getLineLabelSize());
		state.put("lineLabelResolution", appearanceInspector.getLineLabelResolution());
		
		state.put("showFaces", appearanceInspector.isShowFaces());
		state.put("showFaceLabels", appearanceInspector.isShowFaceLabels());
		state.put("facesReflecting", appearanceInspector.isFacesReflecting());
		state.put("faceReflection", appearanceInspector.getFaceReflection());
		state.put("faceColor", appearanceInspector.getFaceColor());
		state.put("faceLabelColor", appearanceInspector.getFaceLabelColor());
		state.put("faceLabelSize", appearanceInspector.getFaceLabelSize());
		state.put("faceLabelResolution", appearanceInspector.getFaceLabelResolution());		
		
		state.put("transparencyEnabled", appearanceInspector.isTransparencyEnabled());
		state.put("transparency", appearanceInspector.getTransparency());
		state.put("facesFlat", appearanceInspector.isFacesFlat());
		state.put("tubes", appearanceInspector.isTubes());
		state.put("spheres", appearanceInspector.isSpheres());
		state.put("textures", appearanceInspector.getTextures());
		state.put("texture", appearanceInspector.getTexture());

		return state;
	}
	
	public void restoreDefaults() {
		appearanceInspector.setShowPoints(DEFAULT_SHOW_POINTS);
		appearanceInspector.setShowPointLabels(DEFAULT_SHOW_POINT_LABELS);
		appearanceInspector.setPointsReflecting(DEFAULT_POINTS_REFLECTING);
		appearanceInspector.setPointReflection(DEFAULT_POINT_REFLECTION);
		appearanceInspector.setPointRadius(DEFAULT_POINT_RADIUS);
		appearanceInspector.setPointColor(DEFAULT_POINT_COLOR);
		appearanceInspector.setPointLabelColor(DEFAULT_POINT_LABEL_COLOR);
		appearanceInspector.setPointLabelSize(DEFAULT_POINT_LABEL_SIZE);
		appearanceInspector.setPointLabelResolution(DEFAULT_POINT_LABEL_RESOLUTION);
		
		appearanceInspector.setShowLines(DEFAULT_SHOW_LINES);
		appearanceInspector.setShowLineLabels(DEFAULT_SHOW_LINE_LABELS);
		appearanceInspector.setLinesReflecting(DEFAULT_LINES_REFLECTING);
		appearanceInspector.setLineReflection(DEFAULT_LINE_REFLECTION);
		appearanceInspector.setTubeRadius(DEFAULT_TUBE_RADIUS);
		appearanceInspector.setLineColor(DEFAULT_LINE_COLOR);
		appearanceInspector.setLineLabelColor(DEFAULT_LINE_LABEL_COLOR);
		appearanceInspector.setLineLabelSize(DEFAULT_LINE_LABEL_SIZE);
		appearanceInspector.setLineLabelResolution(DEFAULT_LINE_LABEL_RESOLUTION);
		
		appearanceInspector.setShowFaces(DEFAULT_SHOW_FACES);
		appearanceInspector.setShowFaceLabels(DEFAULT_SHOW_FACE_LABELS);
		appearanceInspector.setFacesReflecting(DEFAULT_FACES_REFLECTING);
		appearanceInspector.setFaceReflection(DEFAULT_FACE_REFLECTION);
		appearanceInspector.setFaceColor(DEFAULT_FACE_COLOR);
		appearanceInspector.setFaceLabelColor(DEFAULT_FACE_LABEL_COLOR);
		appearanceInspector.setFaceLabelSize(DEFAULT_FACE_LABEL_SIZE);
		appearanceInspector.setFaceLabelResolution(DEFAULT_FACE_LABEL_RESOLUTION);
		
		appearanceInspector.setTransparencyEnabled(DEFAULT_TRANSPARENCY_ENABLED);
		appearanceInspector.setTransparency(DEFAULT_TRANSPARENCY);
		appearanceInspector.setFacesFlat(DEFAULT_FACES_FLAT);
		appearanceInspector.setTubes(DEFAULT_TUBES);
		appearanceInspector.setSpheres(DEFAULT_SPHERES);
		appearanceInspector.setTexture(DEFAULT_TEXTURE);
	}
	
	public void setState(HashMap<String, Object> state) {
		appearanceInspector.setShowPoints(read(state, "showPoints", DEFAULT_SHOW_POINTS));
		appearanceInspector.setShowPointLabels(read(state, "showPointLabels", DEFAULT_SHOW_POINT_LABELS));
		appearanceInspector.setPointsReflecting(read(state, "pointsReflecting", DEFAULT_POINTS_REFLECTING));
		appearanceInspector.setPointReflection(read(state, "pointReflection", DEFAULT_POINT_REFLECTION));
		appearanceInspector.setPointRadius(read(state, "pointRadius", DEFAULT_POINT_RADIUS));
		appearanceInspector.setPointColor(read(state, "pointColor", DEFAULT_POINT_COLOR));
		appearanceInspector.setPointLabelColor(read(state, "pointLabelColor", DEFAULT_POINT_LABEL_COLOR));
		appearanceInspector.setPointLabelSize(read(state, "pointLabelSize", DEFAULT_POINT_LABEL_SIZE));
		appearanceInspector.setPointLabelResolution(read(state, "pointLabelResolution", DEFAULT_POINT_LABEL_RESOLUTION));
		
		appearanceInspector.setShowLines(read(state, "showLines", DEFAULT_SHOW_LINES));
		appearanceInspector.setShowLineLabels(read(state, "showLineLabels", DEFAULT_SHOW_LINE_LABELS));
		appearanceInspector.setLinesReflecting(read(state, "linesReflecting", DEFAULT_LINES_REFLECTING));
		appearanceInspector.setLineReflection(read(state, "lineReflection", DEFAULT_LINE_REFLECTION));
		appearanceInspector.setTubeRadius(read(state, "tubeRadius", DEFAULT_TUBE_RADIUS));
		appearanceInspector.setLineColor(read(state, "lineColor", DEFAULT_LINE_COLOR));
		appearanceInspector.setLineLabelColor(read(state, "lineLabelColor", DEFAULT_LINE_LABEL_COLOR));
		appearanceInspector.setLineLabelSize(read(state, "lineLabelSize", DEFAULT_LINE_LABEL_SIZE));
		appearanceInspector.setLineLabelResolution(read(state, "lineLabelResolution", DEFAULT_LINE_LABEL_RESOLUTION));
		
		appearanceInspector.setShowFaces(read(state, "showFaces", DEFAULT_SHOW_FACES));
		appearanceInspector.setShowFaceLabels(read(state, "showFaceLabels", DEFAULT_SHOW_FACE_LABELS));
		appearanceInspector.setFacesReflecting(read(state, "facesReflecting", DEFAULT_FACES_REFLECTING));
		appearanceInspector.setFaceReflection(read(state, "faceReflection", DEFAULT_FACE_REFLECTION));
		appearanceInspector.setFaceColor(read(state, "faceColor", DEFAULT_FACE_COLOR));
		appearanceInspector.setFaceLabelColor(read(state, "faceLabelColor", DEFAULT_FACE_LABEL_COLOR));
		appearanceInspector.setFaceLabelSize(read(state, "faceLabelSize", DEFAULT_FACE_LABEL_SIZE));
		appearanceInspector.setFaceLabelResolution(read(state, "faceLabelResolution", DEFAULT_FACE_LABEL_RESOLUTION));
		
		appearanceInspector.setTransparencyEnabled(read(state, "transparencyEnabled", DEFAULT_TRANSPARENCY_ENABLED));
		appearanceInspector.setTransparency(read(state, "transparency", DEFAULT_TRANSPARENCY));
		appearanceInspector.setFacesFlat(read(state, "facesFlat", DEFAULT_FACES_FLAT));
		appearanceInspector.setTubes(read(state, "tubes", DEFAULT_TUBES));
		appearanceInspector.setSpheres(read(state, "spheres", DEFAULT_SPHERES));
		appearanceInspector.setTextures(read(state, "textures", textures));
		appearanceInspector.setTexture(read(state, "texture", DEFAULT_TEXTURE));
		
		appearanceInspector.updateAll();
	}

	@SuppressWarnings("unchecked")
	private <T> T read(HashMap<String, Object> state, String string, T defVal) {
		T val = (T) state.get(string);
		return (val == null) ? defVal : val;
	}
}
