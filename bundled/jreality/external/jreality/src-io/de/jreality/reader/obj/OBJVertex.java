package de.jreality.reader.obj;

public class OBJVertex {
	private int vertexIndex = 0;
	private int textureIndex = 0;
	private int normalIndex = 0;

	public OBJVertex() {}

	public OBJVertex(int i, int j, int k) {
		vertexIndex = i;
		textureIndex = j;
		normalIndex = k;
	}
	
	public int getVertexIndex() {
		return vertexIndex;
	}
	
	public void setVertexIndex(int vertexIndex) {
		this.vertexIndex = vertexIndex;
	}
	
	public int getTextureIndex() {
		return textureIndex;
	}
	
	public void setTextureIndex(int textureIndex) {
		this.textureIndex = textureIndex;
	}
	
	public int getNormalIndex() {
		return normalIndex;
	}
	
	public void setNormalIndex(int normalIndex) {
		this.normalIndex = normalIndex;
	}
	
	public boolean equalIndices(OBJVertex v1) {
		return vertexIndex == v1.getVertexIndex() && textureIndex == v1.getTextureIndex() && normalIndex == v1.getNormalIndex();
	}
	
	public String toString() {
		if(textureIndex == 0 && normalIndex == 0) {
			return ""+vertexIndex;
		} else
		if(textureIndex != 0 && normalIndex == 0) {
			return vertexIndex+"/"+textureIndex;
		} else 
		if(textureIndex == 0 && normalIndex != 0) {
			return vertexIndex+"//"+normalIndex;
		} 
		return vertexIndex+"/"+textureIndex +"/"+normalIndex;
	}
}

