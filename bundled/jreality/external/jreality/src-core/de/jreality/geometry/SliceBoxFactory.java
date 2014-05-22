package de.jreality.geometry;

import java.awt.Color;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;

/**
 * This class wraps a {@link SceneGraphComponent} in a representation of its bounding box,
 * containing two parallel local clipping planes
 * separated by a variable amount.  It attaches a tool to the result so that the user can drag on
 * the faces of the box.  The direction of the drag determines the orientation of the clipping planes
 * and the extent of the drag determines its position. The clipping planes will be oriented to be
 * perpendicular to the axial direction of the mouse drag.
 * 
 * Note: using an instance of this class will hinder you from attaching pick-activated tools to the
 * contents of the scene graph component.
 * 
 * A typical usage of this class:
 * <code><b><pre>
		SceneGraphComponent sliceableSGC =  
			SphereUtility.tessellatedCubeSphere(SphereUtility.SPHERE_SUPERFINE);
		SliceBoxFactory sbf = new SliceBoxFactory(sliceableSGC);
		sbf.setSeparation(0.3);
		sbf.update();
		SceneGraphComponent slicedSGC = sbf.getSliceBoxSGC();
 * </pre></b></code>

 * 
 * @author Charles Gunn
 *
 */public class SliceBoxFactory {

	SceneGraphPath clipper = null;
	double separation = .2;
	SceneGraphComponent clipPlane1SGC, clipPlane2SGC, clipIcon1, clipIcon2, worldSGC, sliceBoxSGC;
	IndexedFaceSetFactory clipIconFactory = new IndexedFaceSetFactory();
	
	public SliceBoxFactory(SceneGraphComponent w)	{
		worldSGC = w;
		ClippingPlane clippingPlane1, clippingPlane2;
		clipIcon1 = SceneGraphUtility.createFullSceneGraphComponent("theClipIcon1");
		Transformation clipTform = clipIcon1.getTransformation();
		clipIcon1.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.TUBE_RADIUS, .01);
		clipIcon1.getAppearance().setAttribute("lineShader.polygonShader.diffuseColor", Color.white);
		clipIcon2 = SceneGraphUtility.createFullSceneGraphComponent("theClipIcon2");
		MatrixBuilder.euclidean().translate(0,0,-separation).assignTo(clipIcon2);
		clipIcon1.addChild(clipIcon2);
		clipPlane1SGC = SceneGraphUtility.createFullSceneGraphComponent("theClipPlane2");
		clipPlane1SGC.setTransformation(clipTform);
		clippingPlane1 = new ClippingPlane();
		clippingPlane1.setLocal(true);
		clipPlane1SGC.setGeometry(clippingPlane1);
		
		clipPlane2SGC = SceneGraphUtility.createFullSceneGraphComponent("theClipPlane2");
		clippingPlane2 = new ClippingPlane();
		clippingPlane2.setLocal(true);
		clipPlane2SGC.setGeometry(clippingPlane2);
		clipPlane1SGC.addChild(clipPlane2SGC);
		clipPlane2SGC.addChild(worldSGC);
		
		sliceBoxSGC = SceneGraphUtility.createFullSceneGraphComponent("slice box");
		worldBoundingBox = BoundingBoxUtility.calculateBoundingBox(worldSGC);
		double[] extents = worldBoundingBox.getExtent();
		double[][] black = { { 0, 0, 0, 0 } };
		double[][] blacks = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
				{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
		clipIconFactory.setVertexCount(4);
		clipIconFactory.setFaceCount(1);
		clipIconFactory.setFaceIndices(new int[][]{{0,1,2,3}});
		updateClipIcon(0);
		IndexedFaceSet square = clipIconFactory.getIndexedFaceSet();
		square.setFaceAttributes(Attribute.COLORS,StorageModel.DOUBLE_ARRAY.array(4).createReadOnly(black));
		clipIcon1.setGeometry(square);
		clipIcon2.setGeometry(square);
		IndexedFaceSet cube = Primitives.box(extents[0], extents[1], extents[2],false);
		MatrixBuilder.euclidean().translate(worldBoundingBox.getCenter()).assignTo(sliceBoxSGC);
		cube.setFaceAttributes(Attribute.COLORS,StorageModel.DOUBLE_ARRAY.array(4).createReadOnly(blacks));
		sliceBoxSGC.setGeometry(cube);
		sliceBoxSGC.addTool(new SimpleDragTool());
		sliceBoxSGC.addChild(clipPlane1SGC);
		sliceBoxSGC.addChild(clipIcon1);
		clipper = new SceneGraphPath(sliceBoxSGC,clipPlane1SGC, clipPlane2SGC);
		update();
	}
	
	private void updateClipIcon(int i) {
		int i0 = (i+1)%3, i1 = (i+2)%3;
		double[] ex = worldBoundingBox.getExtent();
		double x = ex[i0]/2, y = ex[i1]/2;
		double[][] vv = { {-x, -y, 0 }, { x, -y, 0 }, { x, y, 0 }, { -x, y, 0 } };
		clipIconFactory.setVertexCoordinates(vv);
		clipIconFactory.setGenerateEdgesFromFaces(true);
		clipIconFactory.update();
	}

	public SceneGraphComponent getSliceBoxSGC() {
		return sliceBoxSGC;
	}
	
	public double getSeparation() {
		return separation;
	}

	public void setSeparation(double separation) {
		this.separation = separation;
	}
	public void update()	{
		MatrixBuilder.euclidean().translate(0,0,-separation).reflect(new double[]{0,0,1,0}).assignTo(clipPlane2SGC);
		MatrixBuilder.euclidean().translate(0,0,-separation).assignTo(clipIcon2);
	}
	  private final InputSlot pointerSlot = InputSlot.getDevice("PointerTransformation");
	  private final InputSlot activeSlot = InputSlot.getDevice("PrimaryAction");
	private Rectangle3D worldBoundingBox;
	  public class SimpleDragTool extends AbstractTool {
		boolean dragging = false, firstTime = false;
		double[] originalCoords;
		double[] originalMat;
		double originald = 0;
		int whichFace;
		int oldIndex0 = -1, oldDirection = -1, index0, index1, index2;
		boolean sameFace = true;
		public SimpleDragTool() {
			super(activeSlot);
			addCurrentSlot(pointerSlot, "triggers drag events");
		}

		public void setSeparation(double d)	{
			separation = d;
			update();
		}
		
		public double getSeparation()	{
			return separation;
		}
		@Override
		public void activate(ToolContext tc) {
			super.activate(tc);
			PickResult currentPick = tc.getCurrentPick();
			if (currentPick != null && currentPick.getPickType() == PickResult.PICK_TYPE_FACE) {
				dragging = true;
				whichFace = currentPick.getIndex();
				index0 = whichFace % 3;
				index1 = (index0+1)%3;
				index2 = (index0+2)%3;
				originalCoords = currentPick.getObjectCoordinates().clone();
				if (index0 == oldIndex0)	{
					originalMat = clipPlane1SGC.getTransformation().getMatrix();
					sameFace = true;
				} else	{
					originalMat = Rn.identityMatrix(4);
					originald = 0.0;
					sameFace = false;
				}
				oldIndex0 = index0;
				System.err.println("activate "+Rn.toString(originalCoords));
				firstTime = true;
			}
		}

		@Override
		public void deactivate(ToolContext tc) {
			super.deactivate(tc);
			dragging = false;
		}

		@Override
		public void perform(ToolContext tc) {
			PickResult currentPick = tc.getCurrentPick();
			if (!dragging || originalCoords == null || currentPick == null) return;
			if (currentPick.getPickType() == PickResult.PICK_TYPE_FACE) {
				double[] newCoords = currentPick.getObjectCoordinates();
				double[] dd = Rn.subtract(null, originalCoords, newCoords);
				int direction = oldDirection;
				if (firstTime)	{  // figure out the direction of dragging
					direction = (Math.abs(dd[index1]) > Math.abs(dd[index2])) ? index1 : index2;
					if (!sameFace || direction != oldDirection)	{
						double[] to = new double[3];
						to[direction] = -1;
						double[] zaxis = {0,0,1};
						MatrixBuilder.euclidean().rotateFromTo(zaxis, to).assignTo(originalMat);
						originald = 0.0;
						oldDirection = direction;						
					}
					Matrix foo = new Matrix(originalMat);
					originald = foo.getEntry(direction, 3);
					firstTime = false;
				}
				double d = -dd[direction];
				double[] newCP = new double[4];
				newCP[direction] = -1;
				newCP[3] = d + originald;
				if (newCP[3] < (-separation-worldBoundingBox.getExtent()[direction]/2)) {
					newCP[3] = -separation-worldBoundingBox.getExtent()[direction]/2;
					d = newCP[3] - originald;
				}
				else if (newCP[3] > (worldBoundingBox.getExtent()[direction]/2)) {
					newCP[3] = worldBoundingBox.getExtent()[direction]/2;					
					d = newCP[3] - originald;
				}
				Matrix foo = new Matrix();
				double[] tlate = new double[3];
				tlate[direction] = d;
				MatrixBuilder.euclidean().translate(tlate).assignTo(foo);
				clipPlane1SGC.getTransformation().setMatrix(
						Rn.times(null, 
								foo.getArray(), originalMat));
				worldSGC.getTransformation().setMatrix(clipper.getInverseMatrix(null));
			}
		}
	  }
}
