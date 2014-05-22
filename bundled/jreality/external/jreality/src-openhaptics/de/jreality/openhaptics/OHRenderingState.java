package de.jreality.openhaptics;

import de.jreality.jogl.JOGLRenderingState;

public class OHRenderingState extends JOGLRenderingState {

	public OHRenderingState(OHRenderer jr) {
		super(jr);
	}

	public boolean isHapticRendering = false;
	public static boolean isHapticRendering(JOGLRenderingState jrs){ 
		return ((OHRenderingState)jrs).isHapticRendering; 
	};
}
