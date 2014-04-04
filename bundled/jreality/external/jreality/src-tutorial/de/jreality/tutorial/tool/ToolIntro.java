package de.jreality.tutorial.tool;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

public class ToolIntro {

	static class SimpleTool extends AbstractTool {
		private static final InputSlot LEFT_BUTTON = InputSlot.LEFT_BUTTON;
		private static final InputSlot FORWARD_BACKWARD = InputSlot.getDevice("ForwardBackwardAxis");
		private static final InputSlot POINTER = InputSlot.getDevice("PointerTransformation");
		public SimpleTool() {
			super(LEFT_BUTTON);
			addCurrentSlot(FORWARD_BACKWARD, "triggers perform");
		}
		@Override
		public void activate(ToolContext tc) {
			System.out.println("now I am active. Activated by "+tc.getSource());
		}
		@Override
		public void deactivate(ToolContext tc) {
			System.out.println("No longer active. Deactivated by "+tc.getSource());
		}
		@Override
		public void perform(ToolContext tc) {
			AxisState as = tc.getAxisState(FORWARD_BACKWARD);
			if (tc.getSource() == FORWARD_BACKWARD) {
				if (as.isPressed()) addCurrentSlot(POINTER);
				if (as.isReleased()) removeCurrentSlot(POINTER);
				System.out.println(as);
				System.out.println("as.doubleValue()="+as.doubleValue());
				System.out.println("as.intValue()="+as.intValue());
			}
			if (tc.getSource() == POINTER) System.out.println("Pointer moved");
			System.out.println(tc.getRootToLocal());
		}
	}
	
	public static void main(String[] args) {
		SceneGraphComponent cmp = new SceneGraphComponent("tool cmp");
		cmp.setGeometry(Primitives.icosahedron());
		
		cmp.addTool(new SimpleTool());
		
		JRViewer.display(cmp);
	}
}
