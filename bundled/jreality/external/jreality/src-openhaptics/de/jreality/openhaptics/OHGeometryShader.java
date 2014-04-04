package de.jreality.openhaptics;

import de.jreality.jogl.JOGLPeerComponent;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.jogl.shader.DefaultGeometryShader;
import de.jtem.jopenhaptics.HL;

public class OHGeometryShader extends DefaultGeometryShader {

	boolean needEndShape;
	
	@Override
	public void postRender(JOGLRenderingState jrs) {
		super.postRender(jrs);
		if(needEndShape && OHRenderingState.isHapticRendering(jrs)){
			HL.hlEndShape();
			OHRenderer.checkHLError();
		}
	}
	
	@Override
	public void preRender(JOGLRenderingState jrs, JOGLPeerComponent jpc) {
		if(OHRenderingState.isHapticRendering(jrs) && ((OHPeerComponent) jpc).isHaptic()){
			HL.hlBeginShape(HL.HL_SHAPE_FEEDBACK_BUFFER, ((OHPeerComponent) jpc).getShapeId());
			OHRenderer.checkHLError();
			needEndShape = true;
			((OHPeerComponent) jpc).prepareHapticRendering();
			OHRenderer.checkHLError();
		}
		else{
			needEndShape = false;
		}
		super.preRender(jrs, jpc);
	}
}
