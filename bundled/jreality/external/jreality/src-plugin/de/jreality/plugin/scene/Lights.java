package de.jreality.plugin.scene;

import java.awt.Color;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.PointLight;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class Lights extends Plugin implements ChangeListener  {

	protected SceneGraphComponent lights;

	private static final double DEFAULT_SUN_LIGHT_INTENSITY = .75;
	private static final double DEFAULT_SKY_LIGHT_INTENSITY = .25;

	private DirectionalLight sunLight;
	private DirectionalLight skyLight;
	protected SceneGraphComponent cameraComponent;
	protected SceneGraphComponent headLight;
	private PointLight cameraLight;
	public Lights() {
		lights = new SceneGraphComponent("lights");

		SceneGraphComponent sun = new SceneGraphComponent("sun");
		sunLight = new DirectionalLight("sun light");
		sunLight.setIntensity(DEFAULT_SUN_LIGHT_INTENSITY);
		sun.setLight(sunLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 1 }).assignTo(sun);
		lights.addChild(sun);

		SceneGraphComponent sky = new SceneGraphComponent("sky");
		skyLight = new DirectionalLight();
		skyLight.setIntensity(DEFAULT_SKY_LIGHT_INTENSITY);
		skyLight.setAmbientFake(true);
		skyLight.setName("sky light");
		sky.setLight(skyLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 0 }).assignTo(sky);
		lights.addChild(sky);
		
		headLight = new SceneGraphComponent("camera light");
		cameraLight = new PointLight("camera light");
		cameraLight.setIntensity(.3);
		cameraLight.setAmbientFake(true);
		cameraLight.setFalloff(1, 0, 0);
		cameraLight.setName("camera light");
		cameraLight.setColor(new Color(255,255,255,255));
		headLight.setLight(cameraLight);
	}

	public double getSkyLightIntensity() {
		return skyLight.getIntensity();
	}

	public void setSkyLightIntensity(double x) {
		skyLight.setIntensity(x);
	}

	public void setSunLightIntensity(double intensity) {
		sunLight.setIntensity(intensity);
	}

	public double getSunLightIntensity() {
		return sunLight.getIntensity();
	}
	
	public void setCameraLightIntensity(double intensity) {
		cameraLight.setIntensity(intensity);
	}

	public double getCameraLightIntensity() {
		return cameraLight.getIntensity();
	}

	public void install(Scene scene) {
		SceneGraphComponent sceneRoot = scene.getSceneRoot();
		sceneRoot.addChild(lights);
		updateCameraLight(scene);
		scene.addChangeListener(this);
	}

	private void updateCameraLight(Scene scene) {
		SceneGraphComponent newCmp = null;
		SceneGraphPath camPath = scene.getCameraPath();
		if (camPath != null) newCmp = scene.getCameraPath().getLastComponent();
		if (newCmp != cameraComponent) {
			if (cameraComponent != null) cameraComponent.removeChild(headLight);
			if (newCmp != null) newCmp.addChild(headLight);
			cameraComponent = newCmp;
		}
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "AbstractLights";
		info.vendorName = "Ulrich Pinkall";
		info.icon = ImageHook.getIcon("sonne.png");
		return info;
	}

	@Override
	public void install(Controller c) throws Exception {
		install(c.getPlugin(Scene.class));
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		c.getPlugin(Scene.class).getSceneRoot().removeChild(lights);
		if (cameraComponent != null) {
			Lights.this.cameraComponent.removeChild(headLight);
		}
	}

	public void stateChanged(ChangeEvent e) {
		if (e.getSource() instanceof Scene) {
			Scene scene = (Scene) e.getSource();
			updateCameraLight(scene);
		}
	}

	public DirectionalLight getSunLight() {
		return sunLight;
	}

	public DirectionalLight getSkyLight() {
		return skyLight;
	}
}