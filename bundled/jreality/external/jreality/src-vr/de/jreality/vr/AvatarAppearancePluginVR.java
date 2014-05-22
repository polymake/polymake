package de.jreality.vr;

import java.awt.Color;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;

public class AvatarAppearancePluginVR extends AbstractPluginVR {

	public AvatarAppearancePluginVR(String pluginName) {
		super(pluginName);
	}

	@Override
	public void environmentChanged() {
		SceneGraphComponent avatar = getViewerVR().getAvatarNode();
		Appearance app = avatar.getAppearance();
		if (app == null) {
			app = new Appearance();
			avatar.setAppearance(app);
		}
		ImageData[] env = getViewerVR().getEnvironment();
		CubeMap cm;
		if (env != null) {
			cm = TextureUtility.createReflectionMap(
					app,
					CommonAttributes.POINT_SHADER,
					env
			);
			cm.setBlendColor(new Color(1f, 1f, 1f, 1f));
			cm = TextureUtility.createReflectionMap(
					app,
					CommonAttributes.LINE_SHADER,
					env
			);
			cm.setBlendColor(new Color(1f, 1f, 1f, 1f));
			cm = TextureUtility.createReflectionMap(
					app,
					CommonAttributes.POLYGON_SHADER,
					env
			);
			cm.setBlendColor(new Color(1f, 1f, 1f, .3f));
		} else {
			TextureUtility.removeReflectionMap(app, CommonAttributes.POINT_SHADER);
			TextureUtility.removeReflectionMap(app, CommonAttributes.LINE_SHADER);
			TextureUtility.removeReflectionMap(app, CommonAttributes.POLYGON_SHADER);
		}
	}
}
