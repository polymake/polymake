package de.jreality.swing;

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GraphicsConfiguration;
import java.awt.Image;
import java.awt.ImageCapabilities;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.VolatileImage;

class FakeVolatileImage extends VolatileImage {

	private BufferedImage bi;
	
	public FakeVolatileImage(BufferedImage bi) {
		this.bi=bi;
	}

	public void flush() {
		bi.flush();
	}

	public float getAccelerationPriority() {
		return bi.getAccelerationPriority();
	}

	public ImageCapabilities getCapabilities(GraphicsConfiguration gc) {
		return bi.getCapabilities(gc);
	}

	public Graphics getGraphics() {
		return bi.getGraphics();
	}

	public int getHeight(ImageObserver observer) {
		return bi.getHeight(observer);
	}

	public Object getProperty(String name, ImageObserver observer) {
		return bi.getProperty(name, observer);
	}

	public Image getScaledInstance(int width, int height, int hints) {
		return bi.getScaledInstance(width, height, hints);
	}

	public ImageProducer getSource() {
		return bi.getSource();
	}

	public int getWidth(ImageObserver observer) {
		return bi.getWidth(observer);
	}

	public void setAccelerationPriority(float priority) {
		bi.setAccelerationPriority(priority);
	}

	public String toString() {
		return "Fake Volatile Image: "+bi.toString();
	}

	@Override
	public boolean contentsLost() {
		return false;
	}

	@Override
	public Graphics2D createGraphics() {
		return bi.createGraphics();
	}

	@Override
	public ImageCapabilities getCapabilities() {
		return bi.getCapabilities(null);
	}

	@Override
	public int getHeight() {
		return bi.getHeight();
	}

	@Override
	public BufferedImage getSnapshot() {
		return bi;
	}

	@Override
	public int getWidth() {
		return bi.getWidth();
	}

	@Override
	public int validate(GraphicsConfiguration gc) {
		return IMAGE_OK;
	}

}
