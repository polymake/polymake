package de.jreality.swing;

import java.awt.BufferCapabilities.FlipContents;
import java.awt.Component;
import java.awt.Dialog;
import java.awt.GraphicsConfiguration;
import java.awt.Rectangle;
import java.awt.image.BufferedImage;
import java.awt.peer.ComponentPeer;
import java.awt.peer.FramePeer;

import sun.awt.CausedFocusEvent.Cause;
import sun.java2d.pipe.Region;

class FakeFramePeer6 extends FakeFramePeer implements FramePeer {

	FakeFramePeer6(JFakeFrame f) {
		super(f);
	}

	public Rectangle getBoundsPrivate() {
		// TODO Auto-generated method stub
		return null;
	}

	public void setAlwaysOnTop(boolean alwaysOnTop) {
		// TODO Auto-generated method stub
		
	}

	public void setModalBlocked(Dialog blocker, boolean blocked) {
		// TODO Auto-generated method stub
		
	}

	public void updateIconImages() {
		// TODO Auto-generated method stub
		
	}

	public void updateMinimumSize() {
		// TODO Auto-generated method stub
		
	}

	public boolean requestFocus(Component lightweightChild, boolean temporary, boolean focusedWindowChangeAllowed, long time, Cause cause) {
		// TODO Auto-generated method stub
		return false;
	}

	public void setOpacity(float arg0) {
		// TODO Auto-generated method stub
		
	}

	public void setOpaque(boolean arg0) {
		// TODO Auto-generated method stub
		
	}

	public void updateWindow(BufferedImage arg0) {
		// TODO Auto-generated method stub
		
	}

	public void applyShape(Region arg0) {
		// TODO Auto-generated method stub
		
	}

	public void flip(int arg0, int arg1, int arg2, int arg3, FlipContents arg4) {
		// TODO Auto-generated method stub
		
	}

	public void repositionSecurityWarning() {
		// TODO Auto-generated method stub
		
	}

	public void updateWindow() {
		// TODO Auto-generated method stub
	}

	public void setZOrder(ComponentPeer above) {
		// TODO Auto-generated method stub
		
	}

	public boolean updateGraphicsData(GraphicsConfiguration gc) {
		// TODO Auto-generated method stub
		return false;
	}

	public void updateAlwaysOnTopState() {
	}
	
	public void emulateActivation(boolean emulate) {
		
	}

}