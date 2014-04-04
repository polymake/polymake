package de.jreality.jogl3.geom;

import javax.media.opengl.GL3;

import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.event.GeometryListener;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;

public abstract class JOGLGeometryEntity extends SceneGraphNodeEntity implements GeometryListener {

	protected JOGLGeometryEntity(SceneGraphNode node) {
		super(node);
	}
	public boolean dataUpToDate = false;
	
	//for example holds an array of faces, each face is an array of vertices, and each vertices has an array of components,
	//e.g. u and v coordinates.
	protected boolean isDoubleArrayArrayArray(StorageModel sm){
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.inlined(2))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.inlined(3))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.inlined(4))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.array())
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.array(2))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.array(3))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY.array(4))
			return true;
		return false;
	}
	
	protected boolean isDoubleArrayArray(StorageModel sm){
		if(sm==StorageModel.DOUBLE_ARRAY_ARRAY)
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY.array())
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY.array(2))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY.array(3))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY.array(4))
			return true;
		if(sm == StorageModel.DOUBLE_ARRAY.inlined(4))
			return true;
		if(sm==StorageModel.DOUBLE_ARRAY.inlined(1))
			return true;
		if(sm==StorageModel.DOUBLE2_INLINED)
			return true;
		if(sm==StorageModel.DOUBLE3_INLINED)
			return true;
		return false;
	}
	protected boolean isDoubleArray(StorageModel sm){
		if(sm==StorageModel.DOUBLE_ARRAY)
			return true;
		return false;
	}
	
	protected boolean isIntArray(StorageModel sm){
		if(sm==StorageModel.INT_ARRAY_ARRAY)
			return true;
		if(sm==StorageModel.INT_ARRAY.array(2))
			return true;
		if(sm==StorageModel.INT_ARRAY.array(3))
			return true;
		return false;
	}
	protected boolean isStringArray(StorageModel sm){
		if(sm==StorageModel.STRING_ARRAY)
			return true;
		return false;
	}
	/**
	 * 
	 * @param gl
	 * @return whether the length has changed or not
	 */
	public abstract boolean updateData(GL3 gl);

}
