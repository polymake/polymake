/*
 * Created on Jan 7, 2010
 *
 */
package de.jreality.tutorial.plugin;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Inspector;
import de.jreality.plugin.basic.SimpleAppearancePlugin;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.ui.SimpleAppearanceInspector;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

/**
 * Show how to replace one of the standard content tools (the rotate tool) with a content
 * tool of one's own choice.  In this case I use an instance of {@link ClickWheelCameraZoomTool}
 * but you'll probably want to replace it with another tool activated by left mouse as the
 * rotate tool.  To toggle the tools, depress the '1' key.
 * 
 * @author Charles Gunn
 *
 */public class ContentToolsExample {

	static boolean useMyTool = false;
	public static void main(String[] args)	{
		ContentToolsExample cte = new ContentToolsExample();
		cte.doit();
	}
	
	Content content;
	Tool myTool = new ClickWheelCameraZoomTool(); 
	ContentTools contentTools;
	private void doit() {
		SceneGraphComponent sgc = Primitives.wireframeSphere();
		final JRViewer v = new JRViewer();
		v.setContent(sgc);
		v.addBasicUI();
		v.registerPlugin(new ContentLoader());

		contentTools = new ContentTools();
		v.registerPlugin(contentTools);
		v.getPlugin(Inspector.class).setInitialPosition(
		            ShrinkPanelPlugin.SHRINKER_LEFT);
		v.addContentSupport(ContentType.CenteredAndScaled);
		
		SceneGraphComponent world = new SceneGraphComponent();
		world.setGeometry(Primitives.sharedIcosahedron);
		Appearance app = new Appearance();
		SimpleAppearancePlugin sap = new SimpleAppearancePlugin(app);
		v.registerPlugin(sap);		
		world.setAppearance(app);
		v.startup();
		content = JRViewerUtility.getContentPlugin(v.getController());
		content.setContent(world);
		
		
		final Tool toggleTool = new ToggleTool();
		content.addContentTool(toggleTool);
	}
	
	public class ToggleTool extends AbstractTool {
		int count = 0;
		public ToggleTool() 
		{
			addCurrentSlot(InputSlot.getDevice("key1"));
		}

		@Override
		public void perform(ToolContext tc) {
			count++;
			// it appears that perform() gets called twice when the 1 key is depressed.  Why?
			if ((count % 2) == 0) return;
			useMyTool = !useMyTool;
			System.err.println("In toggle tool "+useMyTool);
		     if (useMyTool)   {
		         contentTools.setRotationEnabled(false);
		         content.addContentTool(myTool);
		     } else {
		         contentTools.setRotationEnabled(true);
		         content.removeContentTool(myTool);
		     }
		}

		@Override
		public String getDescription() {
			return "Test out a toggle tool";
		}
		
	};
}
