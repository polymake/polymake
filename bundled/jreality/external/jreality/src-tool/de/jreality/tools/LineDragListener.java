package de.jreality.tools;


/** The listener interface for receiving drag events (dragStart, drag, dragEnd),
 * signalling that a line of a lineSet is being dragged (with some unspecified
 * input device). */

public interface LineDragListener extends java.util.EventListener {
	
	/** A drag action with some input device has begun. */
	
	public void lineDragStart(LineDragEvent e);
	
	/** A drag action with some input device has been continued. */
	
	public void lineDragged(LineDragEvent e);
	
	/** A drag action with some input device has finished. */
	
	public void lineDragEnd(LineDragEvent e);
}
