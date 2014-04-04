package de.jreality.sunflow;

import org.sunflow.image.Color;
import org.sunflow.system.ImagePanel;

@SuppressWarnings("serial")
public class CancelableImagePanel extends ImagePanel {

	private boolean cancel;
	private boolean done;

	@Override
	public synchronized void imageBegin(int w, int h, int bucketSize) {
		done = false;
		super.imageBegin(w, h, bucketSize);
	}
	
	@Override
	public synchronized void imageUpdate(int x, int y, int w, int h, Color[] data) {
		if (cancel && !done) throw new RuntimeException("cancel");
		super.imageUpdate(x, y, w, h, data);
	}
	
	@Override
	public synchronized void imageFill(int x, int y, int w, int h, Color c) {
		if (cancel && !done) throw new RuntimeException("cancel");
		else super.imageFill(x, y, w, h, c);
	}
	
	@Override
	public synchronized void imageEnd() {
		super.imageEnd();
		done = true;
	}
	
	public synchronized void cancel() {
		cancel=true;
	}

	public boolean isDone() {
		return done;
	}	
}
