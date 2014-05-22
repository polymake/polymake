package de.jreality.tutorial.util.polygon;

import java.awt.Color;
import java.util.LinkedList;
import java.util.List;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.PointSetFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;

/**
 * A sequence of points that can be dragged. Reports any changes of the point positions to
 * {@link ChangeListener}s that can be attached via {@link #addChangeListener(ChangeListener)}.
 * 
 * The points are stored in an array of double-arrays with length 3.
 * 
 * To use, create an instance and attach it's base ({@link #getBase()} into the scene. Attach
 * a ChangeListener to handle dragging.
 * 
 * @author Steffen Weissmann.
 *
 */
public class DragPointSet implements PointSequence {

	private SceneGraphComponent base = new SceneGraphComponent("DragPointSet");
	private PointSetFactory psf = new PointSetFactory();
	
	/**
	 * A tool that reports on dragging. We will attach a PointDragEventListener to it.
	 */
	private DragEventTool dragTool = new DragEventTool();
	
	double[][] vertices;
	
	private boolean closed=true;
	
	/**
	 * @param initialVertices the initial points.
	 */
	public DragPointSet(double[][] initialVertices) {
		base.setGeometry(psf.getGeometry());
		base.addTool(dragTool);
		
		// Here we attach a listener, then we get notified when the
		// user drags a vertex. Then we will call our pointDragged-Method below.
		dragTool.addPointDragListener(new PointDragListener() {
			public void pointDragEnd(PointDragEvent e) {
			}
			public void pointDragStart(PointDragEvent e) {
			}
			public void pointDragged(PointDragEvent e) {
				DragPointSet.this.pointDragged(e);
			}
		});
		initPoints(initialVertices);
		
		// do some initial appearance settings:
		base.setAppearance(new Appearance());
		DefaultGeometryShader dps = (DefaultGeometryShader)
			ShaderUtility.createDefaultGeometryShader(
					base.getAppearance(), false
			);
		DefaultPointShader ps = (DefaultPointShader) dps.getPointShader();
		ps.setPointRadius(0.05);
		ps.setDiffuseColor(Color.yellow);
	}

	private void initPoints(double[][] initialVertices) {
		vertices = new double[initialVertices.length][];
		for (int i=0; i<initialVertices.length; i++) {
			vertices[i]=initialVertices[i].clone();
		}
		psf.setVertexCount(initialVertices.length);
		psf.setVertexCoordinates(vertices);
		psf.update();
	}
	
	public void pointDragged(PointDragEvent e) {
		vertices[e.getIndex()][0]=e.getX();
		vertices[e.getIndex()][1]=e.getY();
		vertices[e.getIndex()][2]=e.getZ();
		transform(vertices[e.getIndex()]);
		psf.setVertexCoordinates(vertices);
		psf.update();
		fireChange();
	}
	
	
	/**Override this method and do a transformation of the dragged vertex, 
	 * e.g., projection to the sphere.
	 * 
	 * @param vertex
	 */
	public void transform(double[] vertex) {	
	}
	
	/**
	 * Returns the {@link SceneGraphComponent} containing the
	 * point geometry as well as the tool to drag the points.
	 * @return the base component
	 */
	public SceneGraphComponent getBase() {
		return base;
	}
	
	/**
	 * Returns the points as an array of 3-vectors (double arrays with length 3).
	 * @return the points
	 */
	public double[][] getPoints() {
		return vertices;
	}

	/**
	 * true if the point sequence is periodic, false if not. This is for instance
	 * used when rendering the point sequence as a line.
	 */
	public boolean isClosed() {
		return closed;
	}

	/**
	 * set if the point sequence should be interpreted as periodic.
	 * @param closed
	 */
	public void setClosed(boolean closed) {
		if (closed == this.closed) return;
		this.closed = closed;
		fireChange();
	}
	
	/******** listener code *********/
	
	private List<ChangeListener> listeners = new LinkedList<ChangeListener>();

	private void fireChange() {
		final ChangeEvent ev = new ChangeEvent(this);
		synchronized (listeners) {
			for (ChangeListener cl : listeners) cl.stateChanged(ev);
		}
	}

	/**
	 * @param A listener that gets notified whenever the points change.
	 */
	public void addChangeListener(ChangeListener cl) {
		synchronized (listeners) {
			listeners.add(cl);
		}
	}
	
	public void removeChangeListener(ChangeListener cl) {
		synchronized (listeners) {
			listeners.remove(cl);
		}
	}
	

}
