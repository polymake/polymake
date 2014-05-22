package de.jreality.tutorial.tool;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.FaceDragEvent;
import de.jreality.tools.FaceDragListener;
import de.jreality.tools.LineDragEvent;
import de.jreality.tools.LineDragListener;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;

public class DragEventTool02 {

	public static void main(String[] args) {
		SceneGraphComponent cmp = new SceneGraphComponent();		
		cmp.setGeometry(Primitives.icosahedron());	
		
		cmp.setGeometry(Primitives.icosahedron());
		Appearance ap = new Appearance();
		cmp.setAppearance(ap);
		setupAppearance(ap);
		
		DragEventTool t = new DragEventTool();
		
		t.addPointDragListener(new PointDragListener() {

			public void pointDragStart(PointDragEvent e) {
				System.out.println("start dragging vertex "+e.getIndex());				
			}

			public void pointDragged(PointDragEvent e) {
				PointSet pointSet = e.getPointSet();
				double[][] points=new double[pointSet.getNumPoints()][];
		        pointSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(points);
		        points[e.getIndex()]=e.getPosition();  
		        pointSet.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(points));			
			}

			public void pointDragEnd(PointDragEvent e) {
			}			
		});
		t.addLineDragListener(new LineDragListener() {
			
			private IndexedLineSet lineSet;
			private double[][] points;
			
			public void lineDragStart(LineDragEvent e) {
				System.out.println("start dragging line "+e.getIndex());
				
				lineSet = e.getIndexedLineSet();
				points=new double[lineSet.getNumPoints()][];
				lineSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(points);
			}

			public void lineDragged(LineDragEvent e) {
				double[][] newPoints=(double[][])points.clone();
				Matrix trafo=new Matrix();
				MatrixBuilder.euclidean().translate(e.getTranslation()).assignTo(trafo);
				int[] lineIndices=e.getLineIndices();
				for(int i=0;i<lineIndices.length;i++){
					newPoints[lineIndices[i]]=trafo.multiplyVector(points[lineIndices[i]]);
				}
				lineSet.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(newPoints));	
			}

			public void lineDragEnd(LineDragEvent e) {
			}			
		});
		t.addFaceDragListener(new FaceDragListener() {
			
			private IndexedFaceSet faceSet;
			private double[][] points;
			
			public void faceDragStart(FaceDragEvent e) {
				faceSet = e.getIndexedFaceSet();
				points=new double[faceSet.getNumPoints()][];
				points = faceSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
			}

			public void faceDragged(FaceDragEvent e) {
				double[][] newPoints=(double[][])points.clone();
				Matrix trafo=new Matrix();
				MatrixBuilder.euclidean().translate(e.getTranslation()).assignTo(trafo);
				int[] faceIndices=e.getFaceIndices();
				for(int i=0;i<faceIndices.length;i++){
					newPoints[faceIndices[i]]=trafo.multiplyVector(points[faceIndices[i]]);
				}
				faceSet.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(newPoints));	
			}

			public void faceDragEnd(FaceDragEvent e) {
			}			
		});
		
		cmp.addTool(t);		

	    JRViewer.display(cmp);
	}
	
	private static void setupAppearance(Appearance ap) {
		DefaultGeometryShader dgs;
		DefaultPolygonShader dps;
		DefaultLineShader dls;
		DefaultPointShader dpts;
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(true);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.blue);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.03);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.05);
	}

}
