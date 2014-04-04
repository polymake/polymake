package de.jreality.jogl.shader;

import java.awt.Color;

import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;

/**
 * This class is essentially a cache for the Texture2D used in the constructor.
 * 
 * @author Charles Gunn
 * 
 */
public class JOGLCubeMap implements CubeMap {
	CubeMap proxy;
	protected Color blendColor;
	protected ImageData back, bottom, front, left, right, top;

	public JOGLCubeMap(CubeMap t) {
		super();
		proxy = t;
		update();
	}

	public void update() {
		back = proxy.getBack();
		bottom = proxy.getBottom();
		front = proxy.getFront();
		left = proxy.getLeft();
		right = proxy.getRight();
		top = proxy.getTop();
		blendColor = proxy.getBlendColor();
	}

	public ImageData getBack() {
		return back;
	}

	public ImageData getBottom() {
		return bottom;
	}

	public ImageData getFront() {
		return front;
	}

	public ImageData getLeft() {
		return left;
	}

	public ImageData getRight() {
		return right;
	}

	public ImageData getTop() {
		return top;
	}

	public void setBack(ImageData img) {
		// TODO Auto-generated method stub

	}

	public void setBottom(ImageData img) {
		// TODO Auto-generated method stub

	}

	public void setFront(ImageData img) {
		// TODO Auto-generated method stub

	}

	public void setLeft(ImageData img) {
		// TODO Auto-generated method stub

	}

	public void setRight(ImageData img) {
		// TODO Auto-generated method stub

	}

	public void setTop(ImageData img) {
		// TODO Auto-generated method stub

	}

	public Color getBlendColor() {
		return blendColor;
	}

	public void setBlendColor(Color blendColor) {
		// TODO Auto-generated method stub

	}

}
