package de.jreality.tutorial.projects.darboux;

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

public class RodView implements ChangeListener {

	IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
	SceneGraphComponent base = new SceneGraphComponent();
	private DarbouxTransform darboux;
	
	public RodView(DarbouxTransform darboux) {
		this.darboux = darboux;
		darboux.addChangeListener(this);
		base.setGeometry(ilsf.getGeometry());
		base.setAppearance(new Appearance());

		DefaultGeometryShader dps = (DefaultGeometryShader) ShaderUtility.createDefaultGeometryShader(base.getAppearance(), false);
		DefaultPointShader ps = (DefaultPointShader) dps.getPointShader();
		DefaultLineShader ls = (DefaultLineShader) dps.getLineShader();
		ps.setPointRadius(0.001);
		ls.setTubeRadius(0.005);
		ls.setDiffuseColor(Color.red);
	}

	public void stateChanged(ChangeEvent e) {
		updateView();
	}

	private void updateView() {
		double[][] curve = darboux.getCurve();
		double[][] darbouxCurve = darboux.getPoints();
		
		int n1 = curve.length;
		int n2 = darbouxCurve.length;
		double[][] points = new double[n1+n2][];
		System.arraycopy(curve, 0, points, 0, n1);
		System.arraycopy(darbouxCurve, 0, points, n1, n2);
		
		int idx[][] = new int[n2][2];
		for (int i=0; i<n2; i++) {
			idx[i][0]=i%n1;
			idx[i][1]=n1+i;
		}
		ilsf.setVertexCount(n1+n2);
		ilsf.setEdgeCount(n2);
		ilsf.setVertexCoordinates(points);
		ilsf.setEdgeIndices(idx);
		ilsf.update();
	}
	
	public SceneGraphComponent getBase() {
		return base;
	}
}
