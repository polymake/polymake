package de.jreality.ui.viewerapp;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.SphereUtility;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphPathObserver;
import de.jreality.scene.Sphere;
import de.jreality.scene.Viewer;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;

/**
 * A class which handles rendering the selection coming from a selection manager (see {@link SelectionManager})
 * and to be rendered into an instance of {@link Viewer}.
 * @author gunn
 *
 */
public class SelectionRenderer implements SelectionListener {
	SceneGraphComponent selectionKit, boundKit;
	Appearance boundAppearance;
	SceneGraphPath selection;
	SceneGraphPathObserver observer;
	Viewer viewer;
	boolean visible = false;
	// we need to know which selection manager we listen to, and which viewer
	// we draw into
	public SelectionRenderer(SelectionManager sm, Viewer v)	{
		viewer = v;
		boundKit = SceneGraphUtility.createFullSceneGraphComponent("boundKit");
		boundAppearance = boundKit.getAppearance();
		boundAppearance.setAttribute(CommonAttributes.EDGE_DRAW,true);
		boundAppearance.setAttribute(CommonAttributes.FACE_DRAW,false);
		boundAppearance.setAttribute(CommonAttributes.VERTEX_DRAW,false);
		boundAppearance.setAttribute(CommonAttributes.LIGHTING_ENABLED,false);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE,true);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_FACTOR, 1.0);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE_PATTERN, 0x6666);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_WIDTH,2.0);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBES_DRAW, false);
		boundAppearance.setAttribute(CommonAttributes.LEVEL_OF_DETAIL,0.0);
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.WHITE);

		selectionKit = SceneGraphUtility.createFullSceneGraphComponent("selectionKit");
//		SelectionManagerInterface sm = SelectionManager.selectionManagerForViewer(v);
// for reasons unknown, this doesn't work
//		if (sm.getSelectionPath() != null) setSelectionPath(sm.getSelectionPath());
		sm.addSelectionListener(this);
		selectionKit.addChild(boundKit);
		observer = new SceneGraphPathObserver();
		observer.addTransformationListener( new TransformationListener() {

			public void transformationMatrixChanged(TransformationEvent ev) {
				if (selection == null) return;
				if (boundKit.getTransformation() != null)
					boundKit.getTransformation().setMatrix(selection.getMatrix(null));
			}
			
		});
	}
	
	public void setSelectionPath(SceneGraphPath s)	{
		selection = new SceneGraphPath(s);
		boundKit.getTransformation().setMatrix(selection.getMatrix(null));
		boolean selectionEditable = selection.getLastElement() instanceof SceneGraphComponent &&
			selection.getLastComponent().getTransformation() != null;
		boundAppearance = boundKit.getAppearance();
		boundAppearance.setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.LINE_STIPPLE, !selectionEditable);
		SceneGraphNode sgn =  selection.getLastElement();
		Rectangle3D bbox = null;
		if (sgn instanceof SceneGraphComponent) bbox = BoundingBoxUtility.calculateChildrenBoundingBox((SceneGraphComponent) sgn);
		else if (sgn instanceof PointSet) bbox = BoundingBoxUtility.calculateBoundingBox( (PointSet) sgn);
		else if (sgn instanceof Sphere) bbox = SphereUtility.getSphereBoundingBox();
		else if (sgn instanceof Cylinder) bbox = SphereUtility.getSphereBoundingBox();
		else {
			LoggingSystem.getLogger(this).warning("Unknown selection class: "+sgn.getClass().toString());
			return;
		}
		LoggingSystem.getLogger(this).fine("BBox is "+bbox.toString());
		IndexedFaceSet boxRepresentation = IndexedFaceSetUtility.representAsSceneGraph(null, bbox);
		boundKit.setGeometry(boxRepresentation);						

		observer.setPath(selection);
		addAuxiliaryComponent(selectionKit, viewer);
		viewer.renderAsync();
	}
	
	public boolean isVisible() {
		return visible;
	}

	public void setVisible(boolean visible) {
		this.visible = visible;
		boundKit.setVisible(visible);
		viewer.renderAsync();
	}

	public void dispose()	{
		observer.dispose();
		removeAuxiliaryComponent(selectionKit, viewer);
	}
	
	public static void addAuxiliaryComponent(SceneGraphComponent aux, Viewer v)	{
		if (v.getAuxiliaryRoot() == null)	{
			v.setAuxiliaryRoot(SceneGraphUtility.createFullSceneGraphComponent("AuxiliaryRoot"));
		}
		if (!v.getAuxiliaryRoot().isDirectAncestor(aux)) v.getAuxiliaryRoot().addChild(aux);
	}
	
	public static void removeAuxiliaryComponent(SceneGraphComponent aux, Viewer v)	{
		if (v.getAuxiliaryRoot() == null)	return;
		if (!v.getAuxiliaryRoot().isDirectAncestor(aux) ) return;
		v.getAuxiliaryRoot().removeChild(aux);
	}

	public void selectionChanged(SelectionEvent e) {
		setSelectionPath(e.getSelection().getSGPath());
	}

}
