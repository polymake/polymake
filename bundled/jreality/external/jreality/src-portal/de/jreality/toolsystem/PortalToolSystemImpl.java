package de.jreality.toolsystem;

import de.jreality.jogl.JOGLViewer;
import de.jreality.toolsystem.config.ToolSystemConfiguration;

public class PortalToolSystemImpl extends ToolSystem implements PortalToolSystem {

	JOGLViewer joglViewer;
	
	  public PortalToolSystemImpl(JOGLViewer joglViewer, ToolSystemConfiguration config) {
		  super(joglViewer, config, null);
		  this.joglViewer=joglViewer;
	  }

	  /* (non-Javadoc)
	 * @see de.jreality.toolsystem.PortalToolSystem#render()
	 */
	public void render() {
		joglViewer.render();
	  }
	  
	  /* (non-Javadoc)
	 * @see de.jreality.toolsystem.PortalToolSystem#swapBuffers()
	 */
	public void swapBuffers() {
		  joglViewer.swapBuffers();
	  }

	public void setAutoSwapBufferMode(boolean autoSwap) {
		joglViewer.setAutoSwapMode(autoSwap);
	}
	
	

}
