package de.jreality.plugin.content;

import de.jreality.plugin.basic.Content;
import de.jreality.plugin.basic.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.util.SceneGraphUtility;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.PluginInfo;

public class DirectContent extends Content {
		
	@Override
	public void setContent(SceneGraphNode node) {
		SceneGraphComponent root = getContentRoot();
		boolean fire = getContentNode() != node;
		if (getContentNode() != null) {
			SceneGraphUtility.removeChildNode(root, getContentNode());
		}
		setContentNode(node);
		if (getContentNode() != null) { 
			SceneGraphUtility.addChildNode(root, getContentNode());
		}
		if (fire) {
			ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ContentChanged);
			cce.node = node;
			fireContentChanged(cce);
		}
	}

	
	public void uninstall(Scene scene, Controller c) {
		setContent(null);
	}

	@Override
	public PluginInfo getPluginInfo() {
		PluginInfo info = new PluginInfo("Direct Content", "jReality Group");
		info.isDynamic = false;
		return info;
	}

}
