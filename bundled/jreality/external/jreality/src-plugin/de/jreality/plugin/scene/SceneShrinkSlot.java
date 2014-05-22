package de.jreality.plugin.scene;

import java.awt.GridLayout;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.HashSet;
import java.util.Set;

import de.jreality.plugin.basic.RunningEnvironment;
import de.jreality.swing.JFakeFrameWithGeometry;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.Plugin;
import de.jtem.jrworkspace.plugin.PluginInfo;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;
import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkSlotVertical;

public class SceneShrinkSlot extends Plugin {

	RunningEnvironment env;
	
	Set<ShrinkPanel> panels = new HashSet<ShrinkPanel>();
	
	@SuppressWarnings("serial")
	ShrinkSlotVertical slot = new ShrinkSlotVertical(250) {
		@Override
		protected void addShrinkPanelAt(ShrinkPanel p, int pos) {
			super.addShrinkPanelAt(p, pos);
			panels.add(p);
		}
		@Override
		public void removeShrinkPanel(ShrinkPanel panel) {
			super.removeShrinkPanel(panel);
			panels.remove(panel);
		}
	};
	
	private JFakeFrameWithGeometry slotFrame;

	private Set<SceneShrinkPanel> accessories = new HashSet<SceneShrinkPanel>();
	
	@Override
	public PluginInfo getPluginInfo() {
		return new PluginInfo("Embedded Shrink Slot", "jReality Group");
	}
	
	@Override
	public void install(Controller c) throws Exception {
		WindowManager wm = c.getPlugin(WindowManager.class);
		slotFrame = wm.createFrame("Controls");
		slotFrame.setLayout(new GridLayout());
		slotFrame.add(slot);
		if(env != RunningEnvironment.DESKTOP) //if portal
			slotFrame.setBounds(slotFrame.getDesktopWidth()/2, slotFrame.getDesktopHeight()/2, 350, 350);
		else slotFrame.setBounds(slotFrame.getDesktopWidth()/2, slotFrame.getDesktopHeight()/2, 250, 350);
		
		slotFrame.addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				for (SceneShrinkPanel ca : accessories) ca.sceneFrameClosed();
			}
		});
	}
	
	ShrinkSlotVertical getShrinkSlot() {
		return slot;
	}
	
	public void setVisible(boolean v) {
		slotFrame.setVisible(v);
	}

	public boolean isVisible() {
		return slotFrame.isVisible();
	}

	public void closeFrameIfEmpty() {
		if (!isVisible()) return;
		boolean vis = false;
		for (ShrinkPanel p : panels) {
			if (p.isVisible()) vis = true;
		}
		if (!vis) {
			setVisible(false);
		}
	}

	void registerAccessory(SceneShrinkPanel contentAccessory) {
		accessories.add(contentAccessory);
	}

	void unregisterAccessory(SceneShrinkPanel contentAccessory) {
		accessories.remove(contentAccessory);
	}

}
