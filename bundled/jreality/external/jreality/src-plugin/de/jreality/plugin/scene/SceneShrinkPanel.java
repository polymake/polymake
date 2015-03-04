package de.jreality.plugin.scene;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import de.jreality.plugin.basic.Scene;
import de.jreality.plugin.basic.ViewShrinkPanelPlugin;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.ActionTool;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkSlot;

/**
 * A {@link ViewShrinkPanelPlugin} that flops into the Scene when double-clicking on the terrain.
 * Use the setTriggerComponent to attach it to another part of the scene.
 * 
 * @author Steffen Weissmann
 *
 */
public abstract class SceneShrinkPanel extends ViewShrinkPanelPlugin {
	
	private ActionTool 
		actionTool=new ActionTool("PanelActivation");
	
	private boolean windowInScene=false;

	private SceneShrinkSlot sceneSlot;
	private ShrinkSlot
		lastSlot = null;
	
	private ShrinkPanel internalShrinkPanel;
	
	private SceneGraphComponent currentTrigger;

	
	public SceneShrinkPanel() {
		actionTool.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				toggle();
			}
		});
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		sceneSlot = c.getPlugin(SceneShrinkSlot.class);
		
		sceneSlot.registerAccessory(this);
		
		internalShrinkPanel = new ShrinkPanel(shrinkPanel.getName());
		internalShrinkPanel.setIcon(shrinkPanel.getIcon());
		internalShrinkPanel.setFloatable(false);
		
//		sceneSlot.getShrinkSlot().addShrinkPanel(internalShrinkPanel);
//		internalShrinkPanel.setVisible(false);
		
		if (currentTrigger==null) 
			setTriggerComponent(c.getPlugin(Scene.class).getBackdropComponent());

	}

	@Override
	public void uninstall(Controller c) throws Exception {
		super.uninstall(c);
		installTrigger(null);
//		sceneSlot.unregisterAccessory(this);
	}

	private void installTrigger(SceneGraphComponent trigger) {
		if (trigger == currentTrigger) return;
		if (currentTrigger != null) currentTrigger.removeTool(actionTool);
		currentTrigger = trigger;
		if (trigger != null) trigger.addTool(actionTool);
	}

	public void setTriggerComponent(SceneGraphComponent cmp) {
		installTrigger(cmp);
	}

	protected void toggle() {
		if (windowInScene) moveOutOfScene();
		else moveIntoScene();
	}
	
	void moveIntoScene() {
		if (windowInScene) return;
		windowInScene = true;
		// move content to scene slot
//		JPanel content = shrinkPanel.getContentPanel();
//		internalShrinkPanel.setShrinked(shrinkPanel.isShrinked());
//		internalShrinkPanel.setContentPanel(content);
//		// hide outer panel
//		shrinkPanel.setVisible(false);
//		// show internal panel
//		internalShrinkPanel.setVisible(true);
		lastSlot = shrinkPanel.getParentSlot();
		if (lastSlot != null) lastSlot.removeShrinkPanel(shrinkPanel);
		sceneSlot.getShrinkSlot().addShrinkPanel(shrinkPanel);
		shrinkPanel.setFloatable(false);
		// force display of scene slot
		sceneSlot.setVisible(true);
//		System.out.println(getClass().getSimpleName()+".moveIntoScene()");
	}
	
	void moveOutOfScene() {
		if (!windowInScene) return;
		windowInScene = false;
		// move content to non-scene slot	
//		JPanel content = internalShrinkPanel.getContentPanel();
//		shrinkPanel.setShrinked(internalShrinkPanel.isShrinked());
//		shrinkPanel.setContentPanel(content);
//		// hide inner panel
//		internalShrinkPanel.setVisible(false);
//		// show external panel
//		shrinkPanel.setVisible(true);
		sceneSlot.getShrinkSlot().removeShrinkPanel(shrinkPanel);
		if (lastSlot != null) lastSlot.addShrinkPanel(shrinkPanel);
		shrinkPanel.setFloatable(true);
		// close internal frame if empty
		sceneSlot.closeFrameIfEmpty();
//		System.out.println(getClass().getSimpleName()+".moveOutOfScene()");
	}
	
	void sceneFrameClosed() {
		moveOutOfScene();
	}
	
}
