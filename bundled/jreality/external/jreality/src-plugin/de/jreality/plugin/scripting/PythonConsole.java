package de.jreality.plugin.scripting;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.net.URI;

import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.text.Keymap;

import org.python.core.PySystemState;
import org.python.util.PythonInterpreter;

import de.jreality.plugin.JRViewer;
import de.jreality.plugin.basic.View;
import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class PythonConsole extends ShrinkPanelPlugin implements FocusListener {

	private Controller
		controller = null;
	private PythonInterpreter 
		interpreter = null;
	private JScrollPane
		contentPanel = new JScrollPane();
	private JTextPane
		textPane = JRViewer.scriptingTextPane;
	private boolean
		consoleBooted = false;
	
	public PythonConsole() {
        setInitialPosition(ShrinkPanelPlugin.SHRINKER_BOTTOM);
	}
	
	private void createLayout() {
		Dimension d = new Dimension(400, 200);
        contentPanel.setPreferredSize(d);
        contentPanel.setMinimumSize(d);
        shrinkPanel.setLayout(new GridLayout());
        shrinkPanel.add(contentPanel);
        shrinkPanel.setShrinked(true);
        contentPanel.setViewportView(textPane);
        textPane.setText("Python Console, click to boot up....");
        textPane.addFocusListener(this);
        textPane.setCaretColor(Color.BLACK);
	}
	
	public static class MyJTextPane extends JTextPane {
		
		private static final long serialVersionUID = 1L;

		@Override
		public void setKeymap(Keymap map) {
			if (map != null && !map.getName().contains("Substance")) {
				super.setKeymap(map);
			}
		}
		
	}

	protected void bootConsole() {
		if (consoleBooted) return;
		interpreter = getInterpreter();
		interpreter.set("c", controller);
		interpreter.set("textpane", textPane);
		interpreter.exec("vars = {'C' : c}");
		interpreter.exec("from console import Console");
		interpreter.exec("Console(vars, textpane)");
		consoleBooted = true;
	}
	
	
	@Override
	public void focusGained(FocusEvent e) {
		bootConsole();
	}
	@Override
	public void focusLost(FocusEvent e) {
	}
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		this.controller = c;
		createLayout();
	}
	
	public PythonInterpreter getInterpreter() {
		if (interpreter == null) {
			PySystemState.initialize();
			interpreter = new PythonInterpreter();
		}
		return interpreter;
	}
	
	public static void main(String[] args) {
		try {
			PythonInterpreter interpreter = new PythonInterpreter();
			interpreter.set("__name__", "__main__");
			URI consoleURI = URI.create("jar:file:lib/jython/console.jar!/console.py");
			interpreter.execfile(consoleURI.toURL().openStream());
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}
	
}
