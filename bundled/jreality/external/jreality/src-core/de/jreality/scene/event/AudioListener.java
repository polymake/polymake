package de.jreality.scene.event;

import java.util.EventListener;

public interface AudioListener extends EventListener {
	public void audioChanged(AudioEvent ev);
}
