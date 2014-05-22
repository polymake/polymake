package de.jreality.vr;

import java.awt.Color;
import java.util.prefs.Preferences;

import javax.swing.JPanel;

public class AppearancePluginVR extends AbstractPluginVR {

	// defaults for app panel
	private static final boolean DEFAULT_SHOW_POINTS = false;
	private static final boolean DEFAULT_POINTS_REFLECTING = false;
	private static final double DEFAULT_POINT_RADIUS = .4;
	private static final Color DEFAULT_POINT_COLOR = Color.blue;
	private static final boolean DEFAULT_SHOW_LINES = false;
	private static final boolean DEFAULT_LINES_REFLECTING = false;
	private static final double DEFAULT_TUBE_RADIUS = .3;
	private static final Color DEFAULT_LINE_COLOR = Color.red;
	private static final boolean DEFAULT_SHOW_FACES = true;
	private static final boolean DEFAULT_FACES_REFLECTING = true;
	private static final double DEFAULT_FACE_REFLECTION = .7;
	private static final double DEFAULT_LINE_REFLECTION = .7;
	private static final double DEFAULT_POINT_REFLECTION = .7;
	private static final Color DEFAULT_FACE_COLOR = Color.white;
	private static final boolean DEFAULT_TRANSPARENCY_ENABLED = false;
	private static final double DEFAULT_TRANSPARENCY = .7;
	private static final boolean DEFAULT_FACES_FLAT = false;
	private static final boolean DEFAULT_TUBES = true;
	private static final boolean DEFAULT_SPHERES = true;
	
	private AppearancePanel appearancePanel;

	public AppearancePluginVR() {
		super("app");
		makeAppTab();
	}
	
	private void makeAppTab() {
		appearancePanel = new AppearancePanel();
	}
	
	@Override
	public void setViewerVR(ViewerVR vvr) {
		super.setViewerVR(vvr);
		appearancePanel.setAppearance(getViewerVR().getContentAppearance());
	}
	
	@Override
	public JPanel getPanel() {
		return appearancePanel;
	}
	
	@Override
	public void contentChanged() {
		appearancePanel.setObjectScale(getViewerVR().getObjectScale());
	}
	
	@Override
	public void environmentChanged() {
		appearancePanel.setSkyBox(getViewerVR().getEnvironment());
	}

	@Override
	public void storePreferences(Preferences prefs) {
		// app panel
		prefs.putBoolean("showPoints", appearancePanel.isShowPoints());
		prefs.putDouble("pointRadius", appearancePanel.getPointRadius());
		Color c = appearancePanel.getPointColor();
		prefs.putInt("pointColorRed", c.getRed());
		prefs.putInt("pointColorGreen", c.getGreen());
		prefs.putInt("pointColorBlue", c.getBlue());
		prefs.putBoolean("showLines", appearancePanel.isShowLines());
		prefs.putDouble("tubeRadius", appearancePanel.getTubeRadius());
		c = appearancePanel.getLineColor();
		prefs.putInt("lineColorRed", c.getRed());
		prefs.putInt("lineColorGreen", c.getGreen());
		prefs.putInt("lineColorBlue", c.getBlue());
		prefs.putBoolean("showFaces", appearancePanel.isShowFaces());
		prefs.putBoolean("facesReflecting", appearancePanel.isFacesReflecting());
		prefs.putBoolean("linesReflecting", appearancePanel.isLinesReflecting());
		prefs.putBoolean("pointsReflecting", appearancePanel.isPointsReflecting());
		prefs.putDouble("faceReflection", appearancePanel.getFaceReflection());
		prefs.putDouble("lineReflection", appearancePanel.getLineReflection());
		prefs.putDouble("pointReflection", appearancePanel.getPointReflection());
		c = appearancePanel.getFaceColor();
		prefs.putInt("faceColorRed", c.getRed());
		prefs.putInt("faceColorGreen", c.getGreen());
		prefs.putInt("faceColorBlue", c.getBlue());
		prefs.putBoolean("transparencyEnabled", appearancePanel.isTransparencyEnabled());
		prefs.putDouble("transparency", appearancePanel.getTransparency());
		prefs.putBoolean("facesFlat", appearancePanel.isFacesFlat());
		prefs.putBoolean("tubes", appearancePanel.getTubes());
		prefs.putBoolean("spheres", appearancePanel.getSpheres());
	}

	@Override
	public void restoreDefaults() {
		// app panel
		appearancePanel.setShowPoints(DEFAULT_SHOW_POINTS);
		appearancePanel.setPointsReflecting(DEFAULT_POINTS_REFLECTING);
		appearancePanel.setPointRadius(DEFAULT_POINT_RADIUS);
		appearancePanel.setPointColor(DEFAULT_POINT_COLOR);
		appearancePanel.setShowLines(DEFAULT_SHOW_LINES);
		appearancePanel.setLinesReflecting(DEFAULT_LINES_REFLECTING);
		appearancePanel.setTubeRadius(DEFAULT_TUBE_RADIUS);
		appearancePanel.setLineColor(DEFAULT_LINE_COLOR);
		appearancePanel.setShowFaces(DEFAULT_SHOW_FACES);
		appearancePanel.setFacesReflecting(DEFAULT_FACES_REFLECTING);
		appearancePanel.setFaceReflection(DEFAULT_FACE_REFLECTION);
		appearancePanel.setFaceReflection(DEFAULT_LINE_REFLECTION);
		appearancePanel.setFaceReflection(DEFAULT_POINT_REFLECTION);
		appearancePanel.setFaceColor(DEFAULT_FACE_COLOR);
		appearancePanel.setTransparencyEnabled(DEFAULT_TRANSPARENCY_ENABLED);
		appearancePanel.setTransparency(DEFAULT_TRANSPARENCY);
		appearancePanel.setFacesFlat(DEFAULT_FACES_FLAT);
		appearancePanel.setTubes(DEFAULT_TUBES);
		appearancePanel.setSpheres(DEFAULT_SPHERES);
	}

	
	@Override
	public void restorePreferences(Preferences prefs) {
		int r, g, b;
		// app panel
		appearancePanel.setShowPoints(prefs.getBoolean("showPoints", DEFAULT_SHOW_POINTS));
		appearancePanel.setPointsReflecting(prefs.getBoolean("pointsReflecting", DEFAULT_POINTS_REFLECTING));
		appearancePanel.setPointRadius(prefs.getDouble("pointRadius", DEFAULT_POINT_RADIUS));
		r = prefs.getInt("pointColorRed", DEFAULT_POINT_COLOR.getRed());
		g = prefs.getInt("pointColorGreen", DEFAULT_POINT_COLOR.getGreen());
		b = prefs.getInt("pointColorBlue", DEFAULT_POINT_COLOR.getBlue());
		appearancePanel.setPointColor(new Color(r,g,b));
		appearancePanel.setShowLines(prefs.getBoolean("showLines", DEFAULT_SHOW_LINES));
		appearancePanel.setLinesReflecting(prefs.getBoolean("linesReflecting", DEFAULT_LINES_REFLECTING));
		appearancePanel.setTubeRadius(prefs.getDouble("tubeRadius", DEFAULT_TUBE_RADIUS));
		r = prefs.getInt("lineColorRed", DEFAULT_LINE_COLOR.getRed());
		g = prefs.getInt("lineColorGreen", DEFAULT_LINE_COLOR.getGreen());
		b = prefs.getInt("lineColorBlue", DEFAULT_LINE_COLOR.getBlue());
		appearancePanel.setLineColor(new Color(r,g,b));
		appearancePanel.setShowFaces(prefs.getBoolean("showFaces", DEFAULT_SHOW_FACES));
		appearancePanel.setFacesReflecting(prefs.getBoolean("facesReflecting", DEFAULT_FACES_REFLECTING));
		appearancePanel.setFaceReflection(prefs.getDouble("faceReflection", DEFAULT_FACE_REFLECTION));
		appearancePanel.setLineReflection(prefs.getDouble("lineReflection", DEFAULT_LINE_REFLECTION));
		appearancePanel.setPointReflection(prefs.getDouble("pointReflection", DEFAULT_POINT_REFLECTION));
		r = prefs.getInt("faceColorRed", DEFAULT_FACE_COLOR.getRed());
		g = prefs.getInt("faceColorGreen", DEFAULT_FACE_COLOR.getGreen());
		b = prefs.getInt("faceColorBlue", DEFAULT_FACE_COLOR.getBlue());
		appearancePanel.setFaceColor(new Color(r,g,b));
		appearancePanel.setTransparencyEnabled(prefs.getBoolean("transparencyEnabled", DEFAULT_TRANSPARENCY_ENABLED));
		appearancePanel.setTransparency(prefs.getDouble("transparency", DEFAULT_TRANSPARENCY));
		appearancePanel.setFacesFlat(prefs.getBoolean("facesFlat", DEFAULT_FACES_FLAT));
		appearancePanel.setTubes(prefs.getBoolean("tubes", DEFAULT_TUBES));
		appearancePanel.setSpheres(prefs.getBoolean("spheres", DEFAULT_SPHERES));
	}

	public void setFaceColor(Color c) {
		appearancePanel.setFaceColor(c);
	}

	public void setFaceReflection(double d) {
		appearancePanel.setFaceReflection(d);
	}

	public void setFacesFlat(boolean b) {
		appearancePanel.setFacesFlat(b);
	}

	public void setFacesReflecting(boolean b) {
		appearancePanel.setFacesReflecting(b);
	}

	public void setLineColor(Color c) {
		appearancePanel.setLineColor(c);
	}

	public void setLineReflection(double d) {
		appearancePanel.setLineReflection(d);
	}

	public void setLinesReflecting(boolean b) {
		appearancePanel.setLinesReflecting(b);
	}

	public void setPointColor(Color c) {
		appearancePanel.setPointColor(c);
	}

	public void setPointRadius(double d) {
		appearancePanel.setPointRadius(d);
	}

	public void setPointReflection(double d) {
		appearancePanel.setPointReflection(d);
	}

	public void setPointsReflecting(boolean b) {
		appearancePanel.setPointsReflecting(b);
	}

	public void setShowFaces(boolean selected) {
		appearancePanel.setShowFaces(selected);
	}

	public void setShowLines(boolean selected) {
		appearancePanel.setShowLines(selected);
	}

	public void setShowPoints(boolean selected) {
		appearancePanel.setShowPoints(selected);
	}

	public void setTransparency(double d) {
		appearancePanel.setTransparency(d);
	}

	public void setTransparencyEnabled(boolean b) {
		appearancePanel.setTransparencyEnabled(b);
	}

	public void setTubeRadius(double d) {
		appearancePanel.setTubeRadius(d);
	}
	
}
