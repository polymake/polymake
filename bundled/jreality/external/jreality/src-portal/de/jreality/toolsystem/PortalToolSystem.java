package de.jreality.toolsystem;


public interface PortalToolSystem extends ToolEventReceiver {

	public void render();

	public void setAutoSwapBufferMode(boolean autoSwap);
	
	public abstract void swapBuffers();

}