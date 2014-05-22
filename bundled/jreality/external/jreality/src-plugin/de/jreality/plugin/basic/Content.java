package de.jreality.plugin.basic;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.tool.Tool;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;

public abstract class Content extends Plugin {

	protected SceneGraphNode
		content = null;
	
	public static enum ChangeEventType {
		ContentChanged,
		ToolAdded,
		ToolRemoved
	}
	
	public static class ContentChangedEvent {
		
		public ContentChangedEvent(ChangeEventType type) {
			this.type = type;
		}
		
		public ChangeEventType
			type = ChangeEventType.ContentChanged;
		public SceneGraphNode
			node = null;
		public Tool
			tool = null;
		
	}
	
	public static interface ContentChangedListener {
	
		public void contentChanged(ContentChangedEvent cce);
		
	}
	
	protected Scene
		scene = null;
	protected List<ContentChangedListener>
		listeners = Collections.synchronizedList(new LinkedList<ContentChangedListener>());
	
	public abstract void setContent(SceneGraphNode content);
	
	protected void setContentNode(SceneGraphNode node) {
		content = node;
	}
	
	
	protected SceneGraphNode getContentNode() {
		return content;
	}
	
	
	protected SceneGraphComponent getContentRoot() {
		return scene.getContentComponent();
	}
	
	protected boolean addContentToolImpl(Tool tool) {
		if (getToolComponent().getTools().contains(tool)) {
			return false;
		} else {
			getToolComponent().addTool(tool);
			ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ToolAdded);
			cce.tool = tool;
			fireContentChanged(cce);
			return true;
		}
	}
	
	protected SceneGraphComponent getToolComponent() {
		return getContentRoot();
	}

	protected boolean removeContentToolImpl(Tool tool) {
		boolean removed = getToolComponent().removeTool(tool);
		if (removed) {
			ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ToolRemoved);
			cce.tool = tool;
			fireContentChanged(cce);
		}
		return removed;
	}
	
	
	/**
	 * Add a content tool. Each Content implementation may reject adding/removing
	 * tools, which is signaled by the return value. The return value gives information
	 * if the tool is part of the Content tools after the method call (not if it was
	 * added due to this call, in contrast to the Collections API).
	 * 
	 * @param tool
	 * @return false if the Content rejects the given tool, true otherwise.
	 */
	public boolean addContentTool(Tool tool) {
		addContentToolImpl(tool);
		return true;
	}
	
	/**
	 * Remove a content tool.
	 * 
	 * @param tool
	 * @return true if the tool was removed, false if it was not set before or if removing is rejected.
	 */
	public boolean removeContentTool(Tool tool) {
		removeContentToolImpl(tool);
		return true;
	}
	
	public void fireContentChanged(ContentChangedEvent cce) {
		synchronized (listeners) {
			for (ContentChangedListener l : listeners) {
				l.contentChanged(cce);
			}	
		}
	}
	
	
	public void fireContentChanged() {
		ContentChangedEvent cce = new ContentChangedEvent(ChangeEventType.ContentChanged);
		synchronized (listeners) {
			for (ContentChangedListener l : listeners) {
				l.contentChanged(cce);
			}
		}
	}
	
	
	public boolean addContentChangedListener(ContentChangedListener l) {
		return listeners.add(l);
	}
	
	public boolean removeContentChangedListener(ContentChangedListener l) {
		return listeners.remove(l);
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		scene = c.getPlugin(Scene.class);
	}
	
	
}
