/*
 * Created on Aug 31, 2010
 *
 */
package de.jreality.tutorial.tool;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.SceneGraphUtility;

public class InputSegmentsExample {

	  private SceneGraphComponent segmentsSGC;

	public InputSegmentsExample() {

	         SceneGraphComponent world = SceneGraphUtility
	               .createFullSceneGraphComponent();
	         // add a transparent background rectangle for picking
	        SceneGraphComponent backgroundSGC = 
	            SceneGraphUtility.createFullSceneGraphComponent("Background");
	        double width = 40, height = 20;
	        double[] corners = {0,0,0,  width,0,0, width,height,0, 0,height,0};
	         Appearance ap = backgroundSGC.getAppearance();
	         ap.setAttribute("transparencyEnabled", true);
	         ap.setAttribute("transparency", 1.0);
	         MatrixBuilder.euclidean().translate(0,0,-.001).assignTo(backgroundSGC);
	         backgroundSGC.setGeometry(Primitives.texturedQuadrilateral(corners));
	         world.addChild(backgroundSGC);
		     SceneGraphComponent frameSGC = 
		            SceneGraphUtility.createFullSceneGraphComponent("Frame");
	         frameSGC.setGeometry(Primitives.texturedQuadrilateral(corners));
	         frameSGC.getAppearance().setAttribute(CommonAttributes.FACE_DRAW, false);
	         frameSGC.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, true);
	         frameSGC.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
		     segmentsSGC = SceneGraphUtility.createFullSceneGraphComponent("Segments");
	         segmentsSGC.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, true);
	         segmentsSGC.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, true);
	         world.addChildren(frameSGC, segmentsSGC);
	         
	         world.addTool(new AbstractTool(InputSlot.RIGHT_BUTTON) {
	        	 	boolean mouseClicked = false;
	        	 	double[][] points = new double[2][4];
	        	 	IndexedLineSetFactory ilsf;
	        	 	SceneGraphComponent currentChild;
	        	 	int[][] edgeInd = {{0,1}};
	            @Override
	            public void activate(ToolContext tc) {
	            		PickResult pr = tc.getCurrentPick();
	            		if (pr == null) return;
		            	super.activate(tc);
	            		addCurrentSlot(InputSlot.POINTER_TRANSFORMATION, "add a new point");
		            	if (!mouseClicked) {
		            		points = new double[2][4];
		            		ilsf = new IndexedLineSetFactory();
		            		ilsf.setVertexCount(2);
		            		ilsf.setVertexCoordinates(points);
		            		ilsf.setEdgeCount(1);
		            		ilsf.setEdgeIndices(edgeInd);
		            		currentChild = new SceneGraphComponent();
		            		segmentsSGC.addChild(currentChild);
	            			points[0] = pr.getObjectCoordinates();
	            			System.err.println("Start Point is: "+Rn.toString(points[0]));
	            			mouseClicked = true;
	            			updatePoint(tc);
	            			currentChild.setGeometry(ilsf.getGeometry());
		            	}
	            }

				@Override
				public void deactivate(ToolContext tc) {
					updatePoint(tc);
					 removeCurrentSlot(InputSlot.POINTER_TRANSFORMATION);
					 mouseClicked = false;
					 super.deactivate(tc);
				}

				@Override
			public void perform(ToolContext tc) {
				System.err.println("Entering perform method");
				updatePoint(tc);
			}

				private void updatePoint(ToolContext tc) {
					if (!mouseClicked) return;
					PickResult pr = tc.getCurrentPick();
					if (pr == null)return;
					points[1] = pr.getObjectCoordinates();
					ilsf.setVertexCoordinates(points);
					System.err.println("End Point is: "+ Rn.toString(points[1]));
					ilsf.update();
				}
	         });
	         JRViewer.display(world);
	      }

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		InputSegmentsExample ape2 = new InputSegmentsExample();
	}

}
