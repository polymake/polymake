package de.jreality.tutorial.tool;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;

public class PointerHitToolExample {

	static class MouseOverTool extends AbstractTool {
		
		Color hcl;
		public MouseOverTool(Color highLightColor) {
			super(InputSlot.POINTER_HIT);
			hcl=highLightColor;
		}
		@Override
		public void activate(ToolContext tc) {
			SceneGraphComponent cmp = tc.getRootToLocal().getLastComponent();
			cmp.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, hcl);
		}
		@Override
		public void deactivate(ToolContext tc) {
			SceneGraphComponent cmp = tc.getRootToLocal().getLastComponent();
			cmp.getAppearance().setAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED);
		}
	};
	
	public static void main(String[] args) {
		SceneGraphComponent root = new SceneGraphComponent();

		SceneGraphComponent cmp = new SceneGraphComponent();
		IndexedFaceSet geom = Primitives.icosahedron();
		cmp.setGeometry(geom);
		cmp.setAppearance(new Appearance());
		cmp.addTool(new MouseOverTool(Color.magenta));

		root.addChild(cmp);
		
		cmp = new SceneGraphComponent();
		cmp.setGeometry(geom);
		cmp.setAppearance(new Appearance());
		cmp.addTool(new MouseOverTool(Color.cyan));

		MatrixBuilder.euclidean().translate(0.8,0.001,0.001).assignTo(cmp);
		
		root.addChild(cmp);
		
		JRViewer.display(root);
		
	}
	
}
