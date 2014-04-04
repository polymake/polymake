package de.jreality.plugin.experimental;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import de.jreality.plugin.JRViewerUtility;
import de.jreality.plugin.basic.Content;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.Tool;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.annotation.Experimental;

/**
 * A managed content that handles its content with a context map.
 * @author Stefan Sechelmann
 */
@Experimental
public class ManagedContent extends Plugin {

	/**
	 * 
	 * @author Stefan Sechelmann
	 */
	public static interface ContentListener {
		public void contentAdded(Class<?> context, SceneGraphComponent c);
		public void contentRemoved(Class<?> context, SceneGraphComponent c);
		public void contentRemoved(Class<?> context);
		public void contentCleared();
		public void toolAdded(Class<?> context, Tool t);
		public void toolRemoved(Class<?> context, Tool t);
	}
	
	public static class ContentAdapter implements ContentListener {
		public void contentAdded(Class<?> context, SceneGraphComponent c) { }
		public void contentRemoved(Class<?> context, SceneGraphComponent c) { }
		public void contentRemoved(Class<?> context) { }
		public void contentCleared() { }
		public void toolAdded(Class<?> context, Tool t) { }
		public void toolRemoved(Class<?> context, Tool t) { }
	}
	
	private Content
		content = null;
	private SceneGraphComponent
		contentRoot = new SceneGraphComponent("Managed Content Root");
	private Map<Class<?>, SceneGraphComponent>
		contextMap = new HashMap<Class<?>, SceneGraphComponent>();
	private List<ContentListener>
		contentListener = new LinkedList<ContentListener>();
	
	
	public SceneGraphComponent getContextRoot(Class<?> context) {
		if (contextMap.get(context) == null) {
			SceneGraphComponent contextRoot = new SceneGraphComponent();
			contextRoot.setName(context.getSimpleName());
			contentRoot.addChild(contextRoot);
			contextMap.put(context, contextRoot);
		}
		SceneGraphComponent contextRoot = contextMap.get(context);
		return contextRoot;
	}
	
	
	/**
	 * Sets a content for a given context class. This is equivalent to
	 * first invoking removeAll(context) and then addContent(context, c);
	 * @param context
	 * @param c
	 */
	public void setContent(Class<?> context, SceneGraphComponent c) {
		removeAll(context);
		addContent(context, c);
	}
	
	
	/**
	 * Adds a component to the scene graph under the given context.
	 * @param context
	 * @param c
	 */
	public void addContent(Class<?> context, SceneGraphComponent c) {
		SceneGraphComponent contextRoot = getContextRoot(context);
		contextRoot.addChild(c);
		fireContentAdded(context, c);
	}
	
	
	/**
	 * Adds a component to the scene graph if it is not already 
	 * a child of the context root.
	 * @param context
	 * @param c
	 */
	public void addContentUnique(Class<?> context, SceneGraphComponent c) {
		SceneGraphComponent contextRoot = getContextRoot(context);
		if (!contextRoot.getChildComponents().contains(c)) {
			contextRoot.addChild(c);
			fireContentAdded(context, c);
		}
	}
	
	
	/**
	 * Removes a scene component from the scene graph and the given context
	 * @param context
	 * @param c
	 */
	public void removeContent(Class<?> context, SceneGraphComponent c) {
		SceneGraphComponent contextRoot = contextMap.get(context);
		if (contextRoot == null) {
			return;
		}
		if (contextRoot.getChildComponents().contains(c)) {
			contextRoot.removeChild(c);
			fireContentRemoved(context, c);
		}
	}
	
	/**
	 * Removes all content components of a given context 
	 * @param context
	 */
	public void removeAll(Class<?> context) {
		SceneGraphComponent contextRoot = contextMap.get(context);
		if (contextRoot == null) {
			return;
		}
		contextMap.remove(context);
		if (contentRoot.getChildComponents().contains(contextRoot)) {
			contentRoot.removeChild(contextRoot);
			fireContentRemoved(context);
		}
	}
	
	/**
	 * Removes all content components of all contexts
	 */
	public void clearContent() {
		contextMap.clear();
		contentRoot = new SceneGraphComponent();
		fireContentCleared();
	}
	
	
	/**
	 * Adds a tool to the given context root
	 * @param context
	 * @param tool
	 */
	public void addTool(Class<?> context, Tool tool) {
		SceneGraphComponent contextRoot = getContextRoot(context);
		contextRoot.addTool(tool);
		fireToolAdded(context, tool);
	}
	
	
	/**
	 * Adds a tool to the given context root if it is not already 
	 * a child of the context root
	 * @param context
	 * @param tool
	 */
	public void addToolUnique(Class<?> context, Tool tool) {
		SceneGraphComponent contextRoot = getContextRoot(context);
		if (!contextRoot.getTools().contains(tool)) {
			contextRoot.addTool(tool);
			fireToolAdded(context, tool);
		}
	}
	
	/**
	 * Removes a tool from the scene graph at the given context root.
	 * @param context
	 * @param tool
	 */
	public void removeTool(Class<?> context, Tool tool) {
		SceneGraphComponent contextRoot = getContextRoot(context);
		if (contextRoot.getTools().contains(tool)) {
			contextRoot.removeTool(tool);
			fireToolRemoved(context, tool);
		}
	}
	
	
	public void update() {
		content.setContent(contentRoot);
	}
	
	
	/**
	 * Returns all registered contexts
	 * @return a set of context classes
	 */
	protected Set<Class<?>> getContextSet() {
		return contextMap.keySet();
	}
	
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Managed Content", "Stefan Sechelmann");
	}

	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		content = JRViewerUtility.getContentPlugin(c);
	}
	
	public boolean addContentListener(ContentListener l) {
		return contentListener.add(l);
	}
	
	public boolean removeContentListener(ContentListener l) {
		return contentListener.remove(l);
	}
	
	public void removeAllContentListener() {
		contentListener.clear();
	}
	
	protected void fireContentAdded(Class<?> context, SceneGraphComponent c) {
		for (ContentListener l : contentListener) {
			l.contentAdded(context, c);
		}
	}
	
	protected void fireContentRemoved(Class<?> context, SceneGraphComponent c) {
		for (ContentListener l : contentListener) {
			l.contentRemoved(context, c);
		}
	}
	
	protected void fireContentRemoved(Class<?> context) {
		for (ContentListener l : contentListener) {
			l.contentRemoved(context);
		}
	}
	
	protected void fireContentCleared() {
		for (ContentListener l : contentListener) {
			l.contentCleared();
		}
	}
	
	protected void fireToolAdded(Class<?> context, Tool t) {
		for (ContentListener l : contentListener) {
			l.toolAdded(context, t);
		}
	}
	
	protected void fireToolRemoved(Class<?> context, Tool t) {
		for (ContentListener l : contentListener) {
			l.toolRemoved(context, t);
		}
	}
	
}
