package de.jreality.swing;

import java.awt.AWTEvent;
import java.awt.Component;
import java.awt.EventQueue;
import java.awt.Point;
import java.awt.event.MouseEvent;

import de.jreality.scene.tool.InputSlot;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.FaceDragEvent;
import de.jreality.tools.FaceDragListener;

public class PlanarMouseEventTool extends DragEventTool implements FaceDragListener {

	private Point oldPoint;
	private Component comp;

	private boolean dispatchLater = false;
	long lastActivationTime;
	private Point newPoint;

	public PlanarMouseEventTool(InputSlot drag, int buttonID, Component c, boolean dispatchLater) {
		super(drag);
		currentButton=buttonID;
		this.comp = c;
		this.dispatchLater = dispatchLater;
		addCurrentSlot(InputSlot.getDevice("PointerTransformation"), "moves the mouse pointer");
		super.addFaceDragListener(this);
	}

	public PlanarMouseEventTool(InputSlot drag, int buttonID, Component c) {
		this(drag, buttonID, c, true);
	}

	public void faceDragEnd(FaceDragEvent e) {
		dispatchMouseEvent(newPoint, MouseEvent.MOUSE_RELEASED, currentButton);
		if(oldPoint.distance(newPoint) < 15) {
			dispatchMouseEvent(newPoint, MouseEvent.MOUSE_CLICKED, currentButton);
		}
	}

	public void faceDragStart(FaceDragEvent e) {
		long t = System.currentTimeMillis();
		if (t-lastActivationTime<=doubleClickDelay) {
			doubleClick=true;
			lastActivationTime=0;
		} else {
			lastActivationTime = t;
			doubleClick=false;
		}
		newPoint = generatePoint(e);
		oldPoint = newPoint;
		dispatchMouseEvent(newPoint, MouseEvent.MOUSE_PRESSED, currentButton);
	}

	public void faceDragged(FaceDragEvent e) {
		newPoint = generatePoint(e);
		dispatchMouseEvent(newPoint, MouseEvent.MOUSE_DRAGGED, currentButton);

	}

	int currentButton=0;
	boolean doubleClick;
	int doubleClickDelay=400;

	private Point generatePoint(FaceDragEvent e) {
		double[] pos = e.getPosition();
		return new Point((int)pos[0], (int)pos[1]);
	}    

	/**
	 * 
	 * @param newPoint
	 * @param type
	 * @param button 0, 1 or 2
	 */
	void dispatchMouseEvent(Point newPoint, int type, int button) {
//		int xAbs = newPoint.x+comp.getLocation().x;
//		int yAbs = newPoint.y+comp.getLocation().y;
		final MouseEvent newEvent = new MouseEvent(comp,
				(int) type, System.currentTimeMillis(), /*InputEvent.BUTTON1_DOWN_MASK*/ 1 << (10+button), newPoint.x,
				newPoint.y, doubleClick ? 2 : 1, false, MouseEvent.BUTTON1+button);  // TODO: is this what we want?
		dispatchEvent(newEvent);
	}

	/**
	 * this method is safe as it executes in the DispatchThread.
	 * @param e
	 */
	public void dispatchEvent(final AWTEvent e) {
		if (dispatchLater) {
			EventQueue.invokeLater(new Runnable() {
				public void run() {
					comp.dispatchEvent(e);
				}
			});
		} else {
			comp.dispatchEvent(e);
		}
	}
}
