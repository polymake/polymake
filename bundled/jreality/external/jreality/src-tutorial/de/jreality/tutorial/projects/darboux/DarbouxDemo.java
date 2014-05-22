package de.jreality.tutorial.projects.darboux;

import java.awt.Color;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JSlider;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tutorial.util.polygon.DragPointSet;
import de.jreality.tutorial.util.polygon.PointSequenceView;
import de.jreality.tutorial.util.polygon.SubdividedPolygon;
import de.jreality.ui.widgets.TextSlider;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class DarbouxDemo {

	SceneGraphComponent root = new SceneGraphComponent();
	SubdividedPolygon subPoly;
	
	static double initialR = 0.5;
	static int initialN = 0;
	
	public void setR(double r) {
		dt_plus.setR(r);
		dt_minus.setR(-r);
	}
	
	public void setN(int n) {
		subPoly.setSubdivisionLevel(n);
	}
	
	DarbouxTransform dt_plus, dt_minus;
	private boolean closed=true;
	
	
	public DarbouxDemo() {
		DragPointSet dps = new DragPointSet(circle(5, 1));
		
		dps.setClosed(false);
		root.addChild(dps.getBase());
		subPoly = new SubdividedPolygon(dps);
	
		double[] startPoint = new double[3];
		dt_plus = new DarbouxTransform(dps, startPoint);
		dt_minus = new DarbouxTransform(dps, startPoint);
		
		final StartPoint sp = new StartPoint(dps);
		sp.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				dt_plus.setStartPoint(sp.getPoint());
				dt_minus.setStartPoint(sp.getPoint());
			}
		});
		root.addChild(sp.getRoot());
		
		PointSequenceView curveView = new PointSequenceView(subPoly);
		curveView.setPointRadius(0.03);
		curveView.setLineRadius(0.02);
		curveView.setPointColor(Color.red);
		curveView.setLineColor(Color.blue);
		root.addChild(curveView.getBase());
		
		SceneGraphComponent darboux = new SceneGraphComponent();
		root.addChild(darboux);
		
		RodView rv = new RodView(dt_plus);
		root.addChild(rv.getBase());
		
		PointSequenceView psv1 = new PointSequenceView(dt_plus);
		psv1.setPointRadius(0.04);
		psv1.setLineRadius(0.02);
		psv1.setLineColor(Color.orange);
		psv1.setPointColor(Color.green);
		root.addChild(psv1.getBase());
		
		RodView rv_minus = new RodView(dt_minus);
		root.addChild(rv_minus.getBase());
		
		PointSequenceView psv2 = new PointSequenceView(dt_minus);
		psv2.setPointRadius(0.04);
		psv2.setLineRadius(0.02);
		psv2.setLineColor(Color.orange);
		psv2.setPointColor(Color.green);
		root.addChild(psv2.getBase());
		
		setR(initialR);
	}
	
	static double[][] circle(int n, double r) {
		double[][] verts = new double[n][3];
		double dphi = 2.0*Math.PI/n;
		for (int i=0; i<n; i++) {
			verts[i][0]=r*Math.cos(i*dphi);
			verts[i][1]=r*Math.sin(i*dphi);
		}
		return verts;
	}
	
	private SceneGraphComponent getRoot() {
		return root;
	}

	public static void main(String[] args) {
		final DarbouxDemo demo = new DarbouxDemo();
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.registerPlugin(new ContentTools());
		v.addContentSupport(ContentType.TerrainAligned);
		v.addVRSupport();
		
		//final JSlider slider = new JSliderVR(0, 1000, 100);
		final TextSlider.DoubleLog slider = new TextSlider.DoubleLog("r", JSlider.HORIZONTAL, 0.1, 100, 1.0);
		slider.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				demo.setR(slider.getValue());
			}
		});
		
		final JSpinner subdivSpinner = new JSpinner(new SpinnerNumberModel(1, 0, 10, 1));
		subdivSpinner.addChangeListener(new ChangeListener() {
			public void stateChanged(ChangeEvent e) {
				demo.setN((Integer)subdivSpinner.getValue());
			}
		});
		
		ViewShrinkPanelPlugin plugin = new ViewShrinkPanelPlugin() {
			@Override
			public PluginInfo getPluginInfo() {
				return new PluginInfo("r Slider");
			}
		};
		plugin.getShrinkPanel().setLayout(new GridBagLayout());
		plugin.getShrinkPanel().add(slider);
		plugin.getShrinkPanel().add(subdivSpinner);
		v.registerPlugin(plugin);
		
		v.setContent(demo.getRoot());
		
		v.startup();
	}
}

