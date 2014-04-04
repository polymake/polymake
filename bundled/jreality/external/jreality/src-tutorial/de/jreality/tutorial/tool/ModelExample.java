package de.jreality.tutorial.tool;

import java.awt.Color;
import java.io.IOException;

import javax.sound.sampled.UnsupportedAudioFileException;

import de.jreality.audio.javasound.CachedAudioInputStreamSource;
import de.jreality.geometry.IndexedLineSetUtility;
import de.jreality.geometry.PointSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.shader.CommonAttributes;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;
import de.jreality.tools.RotateTool;
import de.jreality.util.Input;



public class ModelExample implements PointDragListener {
	
	private static final Color DEFAULT_COLOR = Color.green;
	private static final Color HIGHLIGHT_COLOR = Color.red;
	
	PointSetFactory controlPoints = new PointSetFactory();
	
	CachedAudioInputStreamSource audioSource = new CachedAudioInputStreamSource("bells", Input.getInput("sound/churchbell_loop.wav"), true);
	
	SceneGraphComponent baseCmp = new SceneGraphComponent();
	SceneGraphComponent controlCmp = new SceneGraphComponent();
	SceneGraphComponent splineCmp = new SceneGraphComponent();
	SceneGraphComponent audioCmp = new SceneGraphComponent();
	
	int n = 5;
	double[][] vertices = Primitives.regularPolygonVertices(n, 0);
	Color[] vertexColors = new Color[n];

	public ModelExample() throws UnsupportedAudioFileException, IOException {
		for (int i=0; i<n; i++) vertexColors[i] = DEFAULT_COLOR;
		controlPoints.setVertexCount(n);
		updateContent();

		DragEventTool tool = new DragEventTool();
		tool.addPointDragListener(this);
		
		Appearance app = new Appearance();
		app.setAttribute(CommonAttributes.POINT_RADIUS, 0.05);
		
		baseCmp.addChildren(controlCmp, splineCmp, audioCmp);
		baseCmp.addTool(new RotateTool());
		audioCmp.setAudioSource(audioSource);
		controlCmp.setAppearance(app);
		controlCmp.setGeometry(controlPoints.getGeometry());
		controlCmp.addTool(tool);
	}
	
	public void pointDragStart(PointDragEvent e) {
		int idx = e.getIndex();
		vertexColors[idx] = HIGHLIGHT_COLOR;
		updateContent();
		setSoundPosition(vertices[idx]);
		audioSource.start();
	}
	
	public void pointDragged(PointDragEvent e) {
		double[] pos = new double[]{e.getX(), e.getY(), e.getZ()};
		vertices[e.getIndex()] = pos;
		setSoundPosition(pos);
		updateContent();
	}
	
	public void pointDragEnd(PointDragEvent e) {
		audioSource.stop();
		vertexColors[e.getIndex()] = DEFAULT_COLOR;
		updateContent();
	}

	private void setSoundPosition(double[] audioPos) {
		MatrixBuilder.euclidean().translate(audioPos).assignTo(audioCmp);
	}
	
	private void updateContent() {
		controlPoints.setVertexCoordinates(vertices);
		controlPoints.setVertexColors(vertexColors);
		controlPoints.update();
		
		double[][] spline = computeSpline(vertices);
		IndexedLineSet curve = IndexedLineSetUtility.createCurveFromPoints(spline, true);
		splineCmp.setGeometry(curve);
	}

	private SceneGraphNode getComponent() {
		return baseCmp;
	}

	private double[][] computeSpline(double[][] vertices) {
		double[][] cur = vertices;
		for (int i=0; i<3; i++) {
			double[][] sub = new double[2*cur.length][];
			int n = cur.length;
			for (int j=0; j<n; j++) {
				sub[2*j] = cur[j];
				sub[2*j+1] = subdivide(
								cur[(j-1+n)%n],
								cur[j],
								cur[(j+1)%n],
								cur[(j+2)%n]
							 );
			}
			cur = sub;
		}
		return cur;
	}
	
	private static double[] subdivide(double[] v1, double[] v2, double[] v3, double[] v4) {
		double[] ret = new double[3];
    	for (int j=0; j<3; j++) ret[j] = (9.0*(v2[j]+v3[j])-v1[j]-v4[j])/16.0;
    	return ret;
	}
	
	public static void main(String[] args) throws UnsupportedAudioFileException, IOException {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addAudioSupport();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		ModelExample example = new ModelExample();
		v.setContent(example.getComponent());
		v.startup();
	}
}