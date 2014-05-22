package de.jreality.jogl;

import de.jreality.math.Rn;
import de.jreality.scene.Geometry;

public class MatrixListData extends Geometry {
	public MatrixListData(String name) {
		super(name);
	}

	private double[][] matrixList = { Rn.identityMatrix(4) };
	private boolean[] visibleList;
	private boolean newVisibleList;
	private boolean shadeGeometry = true;
	private transient boolean rendering = false;
	private boolean copycat = true;
	private boolean clipToCamera = true;
	private boolean componentDisplayLists = true;
	private int count;
	private int delay = 500;

	public double[][] getMatrixList() {
		return matrixList;
	}

	public void setMatrixList(double[][] matrixList) {
		this.matrixList = matrixList;
	}

	public boolean[] getVisibleList() {
		return visibleList;
	}

	public void setVisibleList(boolean[] visibleList) {
		this.visibleList = visibleList;
	}

	public boolean isNewVisibleList() {
		return newVisibleList;
	}

	public void setNewVisibleList(boolean newVisibleList) {
		this.newVisibleList = newVisibleList;
	}

	public boolean isShadeGeometry() {
		return shadeGeometry;
	}

	public void setShadeGeometry(boolean shadeGeometry) {
		this.shadeGeometry = shadeGeometry;
	}

	public boolean isRendering() {
		return rendering;
	}

	public void setRendering(boolean rendering) {
		this.rendering = rendering;
	}

	public boolean isCopycat() {
		return copycat;
	}

	public void setCopycat(boolean copycat) {
		this.copycat = copycat;
	}

	public boolean isClipToCamera() {
		return clipToCamera;
	}

	public void setClipToCamera(boolean clipToCamera) {
		this.clipToCamera = clipToCamera;
	}

	public boolean isComponentDisplayLists() {
		return componentDisplayLists;
	}

	public void setComponentDisplayLists(boolean componentDisplayLists) {
		this.componentDisplayLists = componentDisplayLists;
	}

	public int getCount() {
		return count;
	}

	public void setCount(int count) {
		this.count = count;
	}

	public int getDelay() {
		return delay;
	}

	public void setDelay(int delay) {
		this.delay = delay;
	}

}
