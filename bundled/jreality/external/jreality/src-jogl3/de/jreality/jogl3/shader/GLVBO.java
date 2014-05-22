package de.jreality.jogl3.shader;


public abstract class GLVBO{
	protected int arraySize = 4;
	protected int index;
	protected String name;
	public int getElementSize(){
		return arraySize;
	}
	public String getName() {
		return name;
	}
	public int getID() {
		return index;
	}
	/**
	 * 
	 * @return number of floats/integers in the vbo, i.e. number of bytes divided by 4.
	 */
	public int getLength() {
		return length;
	}
	public abstract int getType();
	protected int length;
}
