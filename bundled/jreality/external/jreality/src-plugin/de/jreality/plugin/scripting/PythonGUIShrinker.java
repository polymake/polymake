package de.jreality.plugin.scripting;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;

import de.jtem.jrworkspace.plugin.sidecontainer.widget.ShrinkPanel;

public class PythonGUIShrinker extends ShrinkPanel implements ActionListener, PythonGUIListener {

	private static final long 
		serialVersionUID = 1L;
	private PythonScriptTool
		tool = null;
	private JButton
		runButton = new JButton("Run Script");

	public PythonGUIShrinker(PythonScriptTool tool) {
		super("Python Script GUI");
		this.tool = tool;
		updateGUI();
		runButton.addActionListener(this);
	}
	
	public void updateGUI() {
		removeAll();
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.weightx = 1.0;
		c.weighty = 0.0;
		c.fill = GridBagConstraints.BOTH;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.insets = new Insets(2, 2, 2, 2);
		for (PythonGUI<?> gui : tool.getGuiList()) {
			gui.removeGUIListener(this);
			gui.addGUIListener(this);
			add(gui.getFrontendGUI(), c);
		}
		add(runButton, c);
		setTitle(tool.getName());
		setIcon(tool.getIcon());
		runButton.setIcon(tool.getIcon());
	}
	
	
	@Override
	public void actionPerformed(ActionEvent e) {
		tool.execute();
	}
	@Override
	public void valueChanged(PythonGUI<?> gui) {
		if (gui.isInstant()) {
			tool.execute();
		}
	}
	
}
