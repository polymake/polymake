package de.jreality.tutorial.geom;

import java.awt.Color;
import java.io.IOException;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.SphereUtility;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.TwoSidePolygonShader;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.tools.RotateTool;
import de.jreality.util.SceneGraphUtility;


public class ClippingPlaneExample{
		static double[][] square = {{0,-1,0},{1,-1,0},{1,1,0},{0,1,0}};
	
	  public static void main(String[] args) throws IOException {
		SceneGraphComponent root = new SceneGraphComponent("world");
		
		final SceneGraphComponent clipIcon = SceneGraphUtility.createFullSceneGraphComponent("theClipIcon");
		double[][] vv = {{-1,-1,0},{-1,1,0},{1,1,0},{1,-1,0}};
		IndexedFaceSet square = IndexedFaceSetUtility.constructPolygon(vv);
		// set color to be completely transparent
		square.setFaceAttributes(Attribute.COLORS, 
				StorageModel.DOUBLE_ARRAY.array(4).createReadOnly(new double[][]{{0,0,1,0}}));
	
		clipIcon.setGeometry(square);
		clipIcon.getTransformation().setMatrix(P3.makeTranslationMatrix(null, new double[]{0d,0d,.5d}, Pn.EUCLIDEAN));
		clipIcon.addTool(new RotateTool());
		// The clip plane itself is a child of the clip icon, so when I move the icon the plane moves
		SceneGraphComponent clipPlane =  SceneGraphUtility.createFullSceneGraphComponent("theClipPlane");
		// the icon for the clipping plane shouldn't get clipped away; move it slightly 
		MatrixBuilder.euclidean().translate(0,0,.01).assignTo(clipPlane);

		ClippingPlane cp =  new ClippingPlane();
		clipPlane.setGeometry(cp);
		// add a rotate tool to the clip icon
		final SceneGraphComponent sgc = SceneGraphUtility.createFullSceneGraphComponent("sphere");
		clipIcon.addChild(clipPlane);
		
		sgc.setPickable( false);
		sgc.addChild(SphereUtility.tessellatedCubeSphere(SphereUtility.SPHERE_SUPERFINE));
		sgc.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER,TwoSidePolygonShader.class);
		sgc.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+".front."+CommonAttributes.DIFFUSE_COLOR, new Color(0,204,204));
		sgc.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+".back."+CommonAttributes.DIFFUSE_COLOR, new Color(204,204,0));
		sgc.getAppearance().setAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, java.awt.Color.WHITE);
		root.addChild(sgc);
		root.addChild(clipIcon);
		root.addTool(new ClickWheelCameraZoomTool());
	    JRViewer.display(root);
	}
 }
