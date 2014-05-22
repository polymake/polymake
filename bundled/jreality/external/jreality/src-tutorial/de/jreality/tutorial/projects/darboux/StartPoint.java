package de.jreality.tutorial.projects.darboux;

import java.util.LinkedList;
import java.util.List;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.PointSetFactory;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;
import de.jreality.tutorial.util.polygon.PointSequence;

public class StartPoint implements ChangeListener {

	private PointSequence curve;
	double[] foot;
	double[] pt = new double[3];
	
	SceneGraphComponent root = new SceneGraphComponent();
	PointSetFactory psf = new PointSetFactory();
	
	public StartPoint(PointSequence curve) {
		psf.setVertexCount(1);
		root.setGeometry(psf.getGeometry());
		root.setAppearance(new Appearance());
		DefaultPointShader dps = (DefaultPointShader) ShaderUtility.createDefaultGeometryShader(root.getAppearance(), false).getPointShader();
		dps.setPointRadius(0.05);
		DragEventTool t = new DragEventTool();
		t.addPointDragListener(new PointDragListener() {
			public void pointDragEnd(PointDragEvent e) {
			}
			public void pointDragStart(PointDragEvent e) {
			}
			public void pointDragged(PointDragEvent e) {
				StartPoint.this.pointDragged(e);
			}
		});
		root.addTool(t);
		this.curve=curve;
		curve.addChangeListener(this);
		foot=curve.getPoints()[0].clone();
		update();
	}

	private void update() {
		psf.setVertexCoordinates(pt);
		psf.update();
		fireChange();
	}

	public void stateChanged(ChangeEvent e) {
		double[] newFoot = curve.getPoints()[0];
		double[] diff = Rn.subtract(null, newFoot, foot);
		Rn.add(pt, diff, pt);
		System.arraycopy(newFoot, 0, foot, 0, 3);
		update();
	}

	public void pointDragged(PointDragEvent e) {
		pt[0]=e.getX();
		pt[1]=e.getY();
		pt[2]=e.getZ();
		update();
	}
	
	private List<ChangeListener> listeners = new LinkedList<ChangeListener>();
	
	private void fireChange() {
		final ChangeEvent ev = new ChangeEvent(this);
		synchronized (listeners ) {
			for (ChangeListener cl : listeners) cl.stateChanged(ev);
		}
	}

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

	public SceneGraphComponent getRoot() {
		return root;
	}

	public double[] getPoint() {
		return pt.clone();
	}

}
