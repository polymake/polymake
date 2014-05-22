package de.jreality.jogl;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.GeometryEvent;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.TransformationEvent;

/**
 * This is an alternative class for use in conjunction with special sorts of
 * scene graphs with many identical children, as one finds in discrete groups.
 * 
 * To activate use, run java with "-DdiscreteGroup.copycat=true" flag
 * 
 * @author Charles Gunn
 * 
 */
public class MatrixListGoBetween extends GoBetween {

	public MatrixListGoBetween() {
	}

	protected MatrixListGoBetween(SceneGraphComponent sgc, JOGLRenderer jr,
			boolean b) {
		super(sgc, jr, b);
	}

	@Override
	public void appearanceChanged(AppearanceEvent ev) {
		setPeerDisplayListDirty();
		super.appearanceChanged(ev);
	}

	@Override
	public void childAdded(SceneGraphComponentEvent ev) {
		setPeerDisplayListDirty();
		super.childAdded(ev);
	}

	@Override
	public void childRemoved(SceneGraphComponentEvent ev) {
		setPeerDisplayListDirty();
		super.childRemoved(ev);
	}

	@Override
	public void childReplaced(SceneGraphComponentEvent ev) {
		setPeerDisplayListDirty();
		super.childReplaced(ev);
	}

	@Override
	public void geometryChanged(GeometryEvent ev) {
		setPeerDisplayListDirty();
		super.geometryChanged(ev);
	}

	@Override
	public void transformationMatrixChanged(TransformationEvent ev) {
		// setPeerDisplayListDirty();
		super.transformationMatrixChanged(ev);
	}

	@Override
	public void visibilityChanged(SceneGraphComponentEvent ev) {
		setPeerDisplayListDirty();
		super.visibilityChanged(ev);
	}

	protected void setPeerDisplayListDirty() {
		((MatrixListJOGLPeerComponent) peers.get(0)).setDisplayListDirty();
	}

}
