package light;

import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

public class GUITool extends AbstractTool {
	public GUITool() {
		super(InputSlot.LEFT_BUTTON);
	}
 
	@Override
	public void activate(ToolContext tc) {
		System.out.println("button clicked! by "+tc.getSource());
	}
 
	@Override
	public void deactivate(ToolContext tc) {
		System.out.println("button released by "+tc.getSource());
	}
 
	@Override
	public void perform(ToolContext tc) {
		System.out.println("Performed by "+tc.getSource());
	}
}
