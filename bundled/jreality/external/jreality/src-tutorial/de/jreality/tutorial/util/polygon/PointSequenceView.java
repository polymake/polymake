package de.jreality.tutorial.util.polygon;

import java.awt.Color;

import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.ShaderUtility;

/**
 * A geometry representation for a {@link PointSequence}. Will update to any changes of
 * the sequence.
 * 
 * @author Steffen Weissmann
 */
public class PointSequenceView implements ChangeListener {

	// the component containing the polygon geometry
	private SceneGraphComponent base = new SceneGraphComponent("subdivided poly");
	private IndexedLineSetFactory lsf = new IndexedLineSetFactory();
	
	// the point sequence to render as a polygon
	private PointSequence sequence;
	
	// Shaders to set appearance attributes
	private DefaultPointShader ps;
	private DefaultLineShader ls;
	
	/**
	 * Constructs a geometry for a point sequence.
	 * 
	 * @param sequence the sequence to visualize.
	 */
	public PointSequenceView(PointSequence sequence) {
		this.sequence = sequence;
		sequence.addChangeListener(this);
		base.setGeometry(lsf.getGeometry());
		base.setAppearance(new Appearance());
		
		DefaultGeometryShader dps = (DefaultGeometryShader)
			ShaderUtility.createDefaultGeometryShader(base.getAppearance(), false);
		ps = (DefaultPointShader) dps.getPointShader();
		ls = (DefaultLineShader) dps.getLineShader();
		
		setPointRadius(0.04);
		setLineRadius(0.02);
		setLineColor(Color.orange);
		setPointColor(Color.green);
		
		update();
	}

	public void setPointRadius(double r) {
		ps.setPointRadius(r);
	}

	public void setLineRadius(double r) {
		ls.setTubeRadius(r);
	}
	
	public void setPointColor(Color c) {
		ps.setDiffuseColor(c);
	}
	
	public void setLineColor(Color c) {
		ls.setDiffuseColor(c);
	}
	
	// update the geometry
	private void update() {
		double[][] pts = sequence.getPoints();
		int n = pts.length;
		if (n != lsf.getVertexCount()) {
			int [][] inds = new int[n][2];
			for (int i=0, m=sequence.isClosed() ? n : n-1; i<m; i++) {
				inds[i][0]=i;
				inds[i][1]=(i+1)%n;
			}
			lsf.setVertexCount(n);
			lsf.setEdgeCount(inds.length);
			lsf.setEdgeIndices(inds);
		}
		lsf.setVertexCoordinates(pts);
		lsf.update();
	}
	
	public void stateChanged(ChangeEvent e) {
		update();
	}

	public SceneGraphComponent getBase() {
		return base;
	}
	
}
