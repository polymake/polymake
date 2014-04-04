package de.jreality.tools;


/** The listener interface for receiving drag events (dragStart, drag, dragEnd),
 * signalling that a face of a faceSet is being dragged (with some unspecified
 * input device). */

public interface FaceDragListener extends java.util.EventListener {
	
	/** A drag action with some input device has begun. */
	
	public void faceDragStart(FaceDragEvent e);
	
	/** A drag action with some input device has been continued. */
	
	public void faceDragged(FaceDragEvent e);
	
	/** A drag action with some input device has finished. */
	
	public void faceDragEnd(FaceDragEvent e);
}
