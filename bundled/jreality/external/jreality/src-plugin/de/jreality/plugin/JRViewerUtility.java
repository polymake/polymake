package de.jreality.plugin;

import static java.util.Collections.sort;

import java.util.Comparator;
import java.util.List;

import de.jreality.math.Pn;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.content.CenteredAndScaledContent;
import de.jreality.plugin.content.DirectContent;
import de.jreality.plugin.content.TerrainAlignedContent;
import de.jreality.scene.Camera;
import de.jreality.util.EncompassFactory;
import de.jtem.jrworkspace.plugin.Controller;

public class JRViewerUtility {

	public static Class<? extends Content>
		defaultContentClass = DirectContent.class;
	
	/**
	 * Returns a Content instance if there is one registered
	 * @param <T>
	 * @param clazz the class of the plug-in
	 * @return a plug-in instance or null if no such plug-in
	 * was registered
	 */
	public static Content getContentPlugin(Controller c) {
		List<Content> candidates = c.getPlugins(Content.class);
		if (candidates.size() != 0) {
			sort(candidates, new ContentPriorityComparator());
			return c.getPlugin(candidates.get(0).getClass());
		} else {
			return c.getPlugin(defaultContentClass);
		}
	}
	

	/**
	 * A descending priority comparator
	 */
	private static class ContentPriorityComparator implements Comparator<Content> {

		@Override
		public int compare(Content o1, Content o2) {
			return getContentPriority(o1) < getContentPriority(o2) ? 1 : -1;
		}
		
		public double getContentPriority(Content c) {
			if (DirectContent.class.equals(c.getClass())) {
				return 0.0;
			}
			if (CenteredAndScaledContent.class.equals(c.getClass())) {
				return 1.0;
			}
			if (TerrainAlignedContent.class.equals(c.getClass())) {
				return 2.0;
			}
			return 10.0;
		}
	
	}
	
	
	public static void encompass(Scene scene, int metric) {
//		encompass(scene, metric, true);
//	}
//	
//	public static void encompass(Scene scene, int metric, boolean noTerrain) {
		EncompassFactory ef = Scene.encompassFactoryForScene(scene);
		ef.setClippingPlanes(scene.isAutomaticClippingPlanes());
		ef.setStereoParameters(scene.isAutomaticClippingPlanes());
		ef.setMargin(1.75);
		ef.setMetric(metric);
		ef.update();
		if (!scene.isAutomaticClippingPlanes()) {
			Camera camera = ((Camera)scene.getCameraPath().getLastElement());
			camera.setFar(1000);
			camera.setNear(.1);
		}
//		try {
//			CameraUtility.encompass(avatarPath, contentPath, cameraPath, 1.75, metric, doCameraParameters);
//		} catch (Exception e) {}
	}
	
	
	public static void encompassEuclidean(Scene scene) {
		encompass(scene, Pn.EUCLIDEAN);
	}


//	public static void encompassEuclidean(Scene scene, boolean noTerrain) {
//		// TODO Auto-generated method stub
//		encompass(scene, Pn.EUCLIDEAN, noTerrain);
//	}
//	

}
