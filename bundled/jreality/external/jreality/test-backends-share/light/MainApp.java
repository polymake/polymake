package light;
import java.awt.Component;
import java.util.List;

import javax.swing.JFrame;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ToolSystemPlugin;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Viewer;
import de.jreality.scene.tool.Tool;


public class MainApp {
	public static void main(String[] args){
		
		IndexedFaceSet ifs = de.jreality.geometry.Primitives.sphere(100);
		JRViewer v = JRViewer.createJRViewerVR(ifs);
		
		v.startupLocal();
		
		Viewer viewer = v.getViewer();
		System.out.println(viewer.toString());
		
		
		JFrame f = new JFrame("test");
		f.getContentPane().add((Component)viewer.getViewingComponent());
		f.setVisible(true);
		
		
		JRViewer v2 = JRViewer.createJRViewerVR(ifs);
		ToolSystemPlugin t = v.getPlugin(ToolSystemPlugin.class);
		List<Tool> tools = v.getController().getPlugin(Scene.class).getContentComponent().getTools();
		List<Tool> tools2 = v2.getController().getPlugin(Scene.class).getContentComponent().getTools();
		for(Tool tool : tools2){
			v2.getController().getPlugin(Scene.class).getContentComponent().removeTool(tool);
		}
		for(Tool tool : tools){
			v2.getController().getPlugin(Scene.class).getContentComponent().addTool(tool);
		}
		
		List<Object> o = v2.getController().getPlugins(Object.class);
		for(Object ob : o){
			System.out.println(ob.toString());
		}
		
		
		v2.startup();
		
		
		
		
	}
}
