package de.jreality.tools;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.JButton;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.toolsystem.ToolSystem;

/**
 * The de.jreality.tools.Button is similar to the de.jreality.tools.Timer
 * should be used instead of swing buttons if its action causes changes in the
 * scene graph (e.g. a change of transformation). To avoid expectable dead
 * locks this button fires its actionEvents on the ToolSystem-Event-Queue. NOTE:
 * Unless the class variable is not attached no JRButton won't do anything.
 * 
 * @author knoeppel
 * 
 */

@SuppressWarnings("serial")
public class Button extends JButton {

	// class variable
	private static JRActionEventHandler jractionhandler;

	public Button(AbstractAction action) {
		super(action);
		if (jractionhandler == null) // first installation of a jrbutton
			jractionhandler = new JRActionEventHandler();
	}

	public static JRActionEventHandler getJRActionEventHandler() {
		return jractionhandler;
	}

	/**
	 * Internal class to fire the actionEvents on the ToolSystem-Event-Queue.
	 * 
	 * @author knoeppel
	 * 
	 */
	class JRActionEventHandler implements AnimatorTask {

		private boolean attached = false;
		private List<ActionEvent> eventList = new Vector<ActionEvent>();

		public JRActionEventHandler() {
		}

		public boolean run(double time, double dt) {
			while (!eventList.isEmpty())
				dequeue();
			return true;
		}

		private void enqueue(ActionEvent e) {
			eventList.add(e);
		}

		private void dequeue() {
			ActionEvent e = eventList.get(0);
			eventList.remove(0);
			
			if (!(e.getSource() instanceof Button))
				throw new IllegalStateException("Event comes not from a JRButton!");
			
			((Button) e.getSource()).perform(e);
		}

	}

	public static void attach(ToolSystem ts) {
		attach(AnimatorTool.getInstance(ts));
	}

	public static void attach(ToolContext tc) {
		attach(AnimatorTool.getInstance(tc));
	}

	public static void attach(AnimatorTool at) {
		if (jractionhandler.attached)
			return;
		at.schedule(JRActionEventHandler.class, jractionhandler);
		jractionhandler.attached = true;
	}

	public static void attach(SceneGraphComponent cmp) {
		if (jractionhandler.attached)
			return;
		cmp.addTool(new AbstractTool() {
			{
				addCurrentSlot(InputSlot.getDevice("SystemTime"));
			}

			@Override
			public void perform(ToolContext tc) {
				attach(tc);
				tc.getRootToToolComponent().getLastComponent().removeTool(this);
			}
		});
	}
	
	protected void perform(ActionEvent event){
		 Object[] listeners = listenerList.getListenerList();
	        // Process the listeners last to first, notifying
	        // those that are interested in this event
	        for (int i = listeners.length-2; i>=0; i-=2) {
	            if (listeners[i]==ActionListener.class) {
	                ((ActionListener)listeners[i+1]).actionPerformed(event);
	            }          
	        }
	}

	@Override
	protected void fireActionPerformed(ActionEvent event) {
		if (!jractionhandler.attached)
			throw new IllegalStateException("button not attached!");
		ActionEvent e = new ActionEvent(this, event.getID(), event
				.getActionCommand(), event.getWhen(), event.getModifiers());
		getJRActionEventHandler().enqueue(e);
	}

}
