package de.jreality.plugin.audio;

import java.awt.Window;

import javax.swing.SwingUtilities;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.audio.AudioAttributes;
import de.jreality.audio.AudioRenderer;
import de.jreality.audio.Interpolation;
import de.jreality.audio.jack.AbstractJackRenderer;
import de.jreality.audio.jack.JackAmbisonicsPlanar2ndOrderRenderer;
import de.jreality.audio.jack.JackAmbisonicsRenderer;
import de.jreality.audio.jack.JackManager;
import de.jreality.audio.javasound.AbstractJavaSoundRenderer;
import de.jreality.audio.javasound.StereoRenderer;
import de.jreality.audio.javasound.VbapRenderer;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.View;
import de.jreality.plugin.icon.ImageHook;
import de.jreality.scene.SceneGraphPath;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;


public class Audio extends Plugin implements ChangeListener {

	public static enum BackendType {
		noSound,
		javaSound,
		javaSoundVBAP,
		jackAmbisonicsFO,
		jackAmbisonicsPSO;
	}

	public static enum InterpolationType {
		noInterpolation,
		linearInterpolation,
		cosineInterpolation,
		cubicInterpolation;
	}

	private View view = null;
	private AudioPreferences prefs = null;
	private Scene scene = null;
	private AudioRenderer renderer = null;
	private Interpolation.Factory interpolationFactory = AudioAttributes.DEFAULT_INTERPOLATION_FACTORY;

	private SceneGraphPath lastMicrophonePath = null;


	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo();
		info.name = "Audio";
		info.vendorName = "jReality Group"; 
		info.icon = ImageHook.getIcon("audio/sound.png");
		return info;
	}

	@Override
	public void install(Controller c) {
		scene = c.getPlugin(Scene.class);	
		prefs = c.getPlugin(AudioPreferences.class);
		view = c.getPlugin(View.class);
		prefs.addChangeListener(this);
		scene.addChangeListener(this);
		try {
			updateAudioRenderer();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private void updateAudioRenderer() throws Exception {
		if (renderer != null) {
			try {
				renderer.shutdown();
			} catch (Exception e) {
				e.printStackTrace();
			}
			renderer = null;
		}
		switch (prefs.getBackendType()) {
		case noSound:
			break;
		case javaSound:
			renderer = new StereoRenderer();
			break;
		case javaSoundVBAP:
			renderer = new VbapRenderer();
			break;
		case jackAmbisonicsFO:
			renderer = new JackAmbisonicsRenderer();
			break;
		case jackAmbisonicsPSO:
			renderer = new JackAmbisonicsPlanar2ndOrderRenderer();
			break;
		}
		switch (prefs.getInterpolationType()) {
		case noInterpolation:
			interpolationFactory = Interpolation.SampleHold.FACTORY;
			break;
		case linearInterpolation:
			interpolationFactory = Interpolation.Linear.FACTORY;
			break;
		case cosineInterpolation:
			interpolationFactory = Interpolation.Cosine.FACTORY;
			break;
		case cubicInterpolation:
			interpolationFactory = Interpolation.Cubic.FACTORY;
			break;
		}

		if (renderer == null) return;

		renderer.setSceneRoot(scene.getSceneRoot());
		SceneGraphPath micPath = scene.getMicrophonePath();
		renderer.setMicrophonePath(micPath);
		lastMicrophonePath = new SceneGraphPath(micPath);

		renderer.setInterpolationFactory(interpolationFactory);

		if (renderer instanceof AbstractJavaSoundRenderer) {
			AbstractJavaSoundRenderer javaSoundRenderer = (AbstractJavaSoundRenderer) renderer;
			javaSoundRenderer.setFrameSize(prefs.getJavaSoundFrameSize());
		} else if (renderer instanceof AbstractJackRenderer) {
			AbstractJackRenderer ajr = (AbstractJackRenderer) renderer;
			ajr.setLabel(prefs.getJackLabel());
			ajr.setTarget(prefs.getJackTarget());
			JackManager.setRetries(prefs.getJackRetries());
		}

		Thread launcher = new Thread() {
			@Override
			public void run() {
				Window w = SwingUtilities.getWindowAncestor(view.getCenterComponent());
				if (!w.isShowing()) {
					try {
						while (!w.isShowing()) {
							Thread.sleep(500);
						}
						Thread.sleep(1000);
					} catch (InterruptedException e) {}
				}
				try {
					renderer.launch();
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		};
		launcher.start();
	}

	@Override
	public void uninstall(Controller c) throws Exception {
		if (renderer != null) {
			renderer.shutdown();
		}
	}

	public void stateChanged(ChangeEvent e) {
		try {
			if (e.getSource() instanceof AudioPreferences) updateAudioRenderer();
			if (e.getSource() instanceof Scene) {
				SceneGraphPath newMicPath = scene.getMicrophonePath();
				if (lastMicrophonePath != null && lastMicrophonePath.isEqual(newMicPath)) return;
				updateAudioRenderer();
			}
		} catch (Exception ex) {
			ex.printStackTrace();
		}
	}
}
