package de.jreality.jogl3;

import de.jreality.scene.Transformation;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;

public class JOGLTransformationEntity extends SceneGraphNodeEntity implements TransformationListener {

	final double [] matrix = new double[16];
	
	protected JOGLTransformationEntity(Transformation node) {
		super(node);
		node.getMatrix(matrix);
	}

	public void transformationMatrixChanged(TransformationEvent ev) {
		ev.getMatrix(matrix);
	}

}
