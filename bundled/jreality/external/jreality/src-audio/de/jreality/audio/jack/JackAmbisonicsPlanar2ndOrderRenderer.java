 package de.jreality.audio.jack;

import de.jreality.audio.AmbisonicsPlanar2ndOrderSoundEncoder;

/**
 * Jack back-end for Second Order Planar Ambisonics.
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 */
public class JackAmbisonicsPlanar2ndOrderRenderer extends AbstractJackRenderer {

	public JackAmbisonicsPlanar2ndOrderRenderer() {
		nPorts = 5;
		encoder=new AmbisonicsPlanar2ndOrderSoundEncoder() {
			public void finishFrame() {
				int port = JackManager.getPort(key);
				outBufs[port+0].put(bw);
				outBufs[port+1].put(bx);
				outBufs[port+2].put(by);
				outBufs[port+3].put(bu);
				outBufs[port+4].put(bv);
			}
		};
	}
}
