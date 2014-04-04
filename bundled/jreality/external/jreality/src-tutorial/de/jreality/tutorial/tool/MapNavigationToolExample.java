package de.jreality.tutorial.tool;

import java.io.IOException;

import de.jreality.geometry.Primitives;
import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.Scene;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.tools.DraggingTool;
import de.jreality.util.Input;

public class MapNavigationToolExample {


	static class CameraDistanceTool extends AbstractTool {
		// mouse wheel up/down
		static final InputSlot up = InputSlot.getDevice("PrimaryUp");
		static final InputSlot down = InputSlot.getDevice("PrimaryDown");
		
		double minDistance = 0.1;
		double step = 0.1;
		
		public CameraDistanceTool() {
			super(); // no activation axes, since this tool is always active
			addCurrentSlot(up, "Move camera away from map");
			addCurrentSlot(down, "Move camera toward map");
		}
		
		@Override
		public void perform(ToolContext tc) {
			SceneGraphComponent avatarCmp = tc.getRootToToolComponent().getLastComponent();
			Matrix m = new Matrix(avatarCmp.getTransformation());
			double zVal = m.getEntry(2, 3); // current distance
			double newZval = zVal;
			if (tc.getSource() == up) {
				newZval += step;
			}
			if (tc.getSource() == down) {
				newZval -= step;
			}
			m.setEntry(2, 3, newZval);
			m.assignTo(avatarCmp);
			
		}
	}
	
	static class MapDraggingTool extends DraggingTool {
		@Override
		public void perform(ToolContext tc) {
			// only allow dragging in x-y plane:
			if (dragInViewDirection) return;
			
			// remember orientation before drag
			FactoredMatrix m1 = new FactoredMatrix(comp.getTransformation());
			super.perform(tc);
			// drag has affected orientation and distance in z-dir
			FactoredMatrix m2 = new FactoredMatrix(comp.getTransformation());
			double[] t = new double[]{m2.getEntry(0, 3), m2.getEntry(1, 3), m1.getEntry(2, 3), 1};
			m1.setColumn(3, t);
			m1.update();
			m1.assignTo(comp);	
		}
	}
	
	private static SceneGraphComponent createMap() {
		SceneGraphComponent map = new SceneGraphComponent("the map");
		Appearance app = new Appearance();
		map.setAppearance(app);
		map.setGeometry(Primitives.texturedQuadrilateral());
		MatrixBuilder.euclidean().scale(20).assignTo(map);
		try {
			TextureUtility.createTexture(app, CommonAttributes.POLYGON_SHADER, ImageData.load(Input.getInput("textures/grid.jpeg")));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return map;
	}

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		Scene s = v.getPlugin(Scene.class);
		s.getContentComponent().addChild(createMap());
		s.getContentComponent().addTool(new MapDraggingTool());
		
		// set up camera position:
		SceneGraphComponent avatarComponent = s.getAvatarComponent();
		MatrixBuilder.euclidean().translate(10, 10, 5).assignTo(avatarComponent);
		avatarComponent.addTool(new CameraDistanceTool());
		
		v.startup();
	}

}
