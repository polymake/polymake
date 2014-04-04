package de.jreality.sunflow;

import java.awt.image.BufferedImage;

import org.sunflow.core.ParameterList;
import org.sunflow.core.ParameterList.InterpolationType;

import de.jreality.math.MatrixBuilder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.ImageData;

public class PerezSky extends Geometry {
	private SkyLight skyLight = new SkyLight();
	private ImageData[] cubeMap;
	private Double turbidity;
	private double[] sunDirection;
	private int resolution;
	private BufferedImage posX, negX, posY, negY, posZ, negZ;
	
	private DirectionalLight sunLight;
	private DirectionalLight skyAmbientLight;
	private SceneGraphComponent sunLightNode;
	private SceneGraphComponent skyAmbientNode;
	private SceneGraphComponent lightComponent;
	
	public PerezSky() {
		this(32, null, new double[]{0,1,1});
	}
	
	public PerezSky(int resolution, Double turbidity, double[] sunDirWorld) {
		super("perezSky");
		this.resolution = resolution;
		this.turbidity = turbidity;
		//skyLight.setThreshold(2f/resolution);
		lightComponent = new SceneGraphComponent();
		sunLight = new DirectionalLight();
		// TODO: uncomment when SunSkyLight works	
		sunLight.setAmbientFake(true);
		sunLight.setName("sun light");
		sunLightNode = new SceneGraphComponent("sun");
		sunLightNode.setLight(sunLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 1 }).assignTo(sunLightNode);
		lightComponent.addChild(sunLightNode);

		skyAmbientNode = new SceneGraphComponent("skyAmbient");
		skyAmbientLight = new DirectionalLight();
		skyAmbientLight.setAmbientFake(true);
		skyAmbientLight.setName("sky light");
		skyAmbientNode.setLight(skyAmbientLight);
		MatrixBuilder.euclidean().rotateFromTo(new double[] { 0, 0, 1 },
				new double[] { 0, 1, 0 }).assignTo(skyAmbientNode);
		lightComponent.addChild(skyAmbientNode);
		makeBufferedImages();
		setSunDirection(sunDirWorld);
	}
	
	public Double getTurbidity() {
		return turbidity;
	}

	public void setTurbidity(Double turbidity) {
		this.turbidity = turbidity;
		updateSkyLight();
		updateCubeMap();
	}

	public double[] getSunDirection() {
		return sunDirection.clone();
	}

	public void setSunDirection(double[] sunDirWorld) {
		this.sunDirection = sunDirWorld.clone();
		MatrixBuilder.euclidean().rotateFromTo(
				new double[] { 0, 0, 1 },
				sunDirWorld
		).assignTo(sunLightNode);
		updateSkyLight();
		updateCubeMap();
	}
	
	private void updateSkyLight() {
		ParameterList pl = new ParameterList();
		if (turbidity != null) {
			pl.addFloat("turbidity", turbidity.floatValue());
		}
		if (sunDirection != null) {
			pl.addVectors(
					"sundir",
					InterpolationType.NONE,
					new float[] {(float)sunDirection[0], (float)sunDirection[1], (float)sunDirection[2]}
			);
		}
		skyLight.update(pl);
	}
	
	public ImageData[] getCubeMap() {
		return cubeMap;
	}

	private void updateCubeMap() {
		for (int i=0; i<resolution; i++) {
			for (int j=0; j<resolution; j++) {
				double v = (resolution-1 - 2.*i)/(resolution-.5);
				double u = (resolution-1 - 2.*j)/(resolution-.5);		
				posX.setRGB(i, j, skyLight.getRadiance( 1, u, v));
				negX.setRGB(i, j, skyLight.getRadiance(-1, u, -v));
				posY.setRGB(i, j, skyLight.getRadiance( -v, 1, -u));
				negY.setRGB(i, j, skyLight.getRadiance( -v,-1, u));
				posZ.setRGB(i, j, skyLight.getRadiance( -v, u, 1));
				negZ.setRGB(i, j, skyLight.getRadiance( v, u, -1));
			}
		}
		cubeMap = new ImageData[]{
			new ImageData(posX),
			new ImageData(negX),
			new ImageData(posY),
			new ImageData(negY),
			new ImageData(posZ),
			new ImageData(negZ)
		};
	}

	private void makeBufferedImages() {
		posX = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
		negX = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
		posY = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);posX = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
		negY = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
		posZ = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
		negZ = new BufferedImage(resolution, resolution, BufferedImage.TYPE_INT_RGB);
	}

	public int getResolution() {
		return resolution;
	}

	public void setResolution(int resolution) {
		if (resolution != this.resolution) {
			this.resolution = resolution;
			makeBufferedImages();
			updateCubeMap();
		}
	}

	public SceneGraphComponent getLightComponent() {
		return lightComponent;
	}
}
