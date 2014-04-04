package de.jreality.jogl;

import static de.jreality.shader.CommonAttributes.CLEAR_COLOR_BUFFER;
import static de.jreality.shader.CommonAttributes.FORCE_RESIDENT_TEXTURES;
import static de.jreality.shader.CommonAttributes.ONE_TEXTURE2D_PER_IMAGE;
import static de.jreality.shader.CommonAttributes.RENDER_S3;
import static de.jreality.shader.CommonAttributes.SKY_BOX;
import static de.jreality.shader.CommonAttributes.USE_OLD_TRANSPARENCY;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.util.LoggingSystem;

public class JOGLTopLevelAppearance {

	Appearance rootAp;
	transient protected AppearanceListener topLevelApListener;
	transient protected boolean renderSpherical = false, frontBanana = true,
			forceResidentTextures = true, oneTexture2DPerImage = false;
	transient protected double globalAntiAliasingFactor = 1.0;
	transient protected CubeMap skyboxCubemap;

	public JOGLTopLevelAppearance(Appearance ap) {
		rootAp = ap;
		topLevelApListener = new AppearanceListener() {

			public void appearanceChanged(AppearanceEvent ev) {
				update();
			}

		};
		rootAp.addAppearanceListener(topLevelApListener);
		update();
	}

	public void dispose() {
		rootAp.removeAppearanceListener(topLevelApListener);
	}

	public void update() {
		// theLog.finer("In extractGlobalParameters");
		Object obj = rootAp.getAttribute(RENDER_S3, Boolean.class); // assume
																	// the best
																	// ...
		if (obj instanceof Boolean)
			frontBanana = renderSpherical = ((Boolean) obj).booleanValue();
		obj = rootAp.getAttribute(FORCE_RESIDENT_TEXTURES, Boolean.class); // assume
																			// the
																			// best
																			// ...
		if (obj instanceof Boolean)
			forceResidentTextures = ((Boolean) obj).booleanValue();
		obj = rootAp.getAttribute(ONE_TEXTURE2D_PER_IMAGE, Boolean.class); // assume
																			// the
																			// best
																			// ...
		if (obj instanceof Boolean)
			oneTexture2DPerImage = ((Boolean) obj).booleanValue();
		LoggingSystem.getLogger(this).fine(
				"one texture per image: " + oneTexture2DPerImage);
		obj = rootAp.getAttribute(CLEAR_COLOR_BUFFER, Boolean.class); // assume
																		// the
																		// best
																		// ...
		// if (obj instanceof Boolean) {
		// renderingState.clearColorBuffer = ((Boolean)obj).booleanValue();
		// //
		// theLog.fine("Setting clear color buffer to "+renderingState.clearColorBuffer);
		// }
		obj = rootAp.getAttribute(USE_OLD_TRANSPARENCY, Boolean.class);
		// a bit ugly: we make this a static variable so shaders can access it
		// easily
		if (obj instanceof Boolean)
			JOGLRenderingState.useOldTransparency = ((Boolean) obj)
					.booleanValue();
		// theLog.fine("forceResTex = "+forceResidentTextures);
		// theLog.info("component display lists = "+renderingState.componentDisplayLists);
		if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class, SKY_BOX,
				rootAp)) {
			skyboxCubemap = (CubeMap) AttributeEntityUtility
					.createAttributeEntity(CubeMap.class, SKY_BOX, rootAp, true);
		} else
			skyboxCubemap = null;
		obj = rootAp.getAttribute(CommonAttributes.ANTI_ALIASING_FACTOR,
				Double.class); // assume the best ...
		if (obj instanceof Double)
			globalAntiAliasingFactor = ((Double) obj).doubleValue();
		obj = rootAp.getAttribute(CommonAttributes.USE_GLSL, Boolean.class); // assume
																				// the
																				// best
																				// ...
		if (obj instanceof Boolean)
			JOGLRenderingState.useGLSL = ((Boolean) obj).booleanValue();

	}

	public boolean isRenderSpherical() {
		return renderSpherical;
	}

	public boolean isFrontBanana() {
		return frontBanana;
	}

	public boolean isForceResidentTextures() {
		return forceResidentTextures;
	}

	public boolean isOneTexture2DPerImage() {
		return oneTexture2DPerImage;
	}

	public CubeMap getSkyboxCubemap() {
		return skyboxCubemap;
	}

	public double getGlobalAntiAliasingFactor() {
		return globalAntiAliasingFactor;
	}

	public void setGlobalAntiAliasingFactor(double globalAntiAliasingFactor) {
		this.globalAntiAliasingFactor = globalAntiAliasingFactor;
	}

}
