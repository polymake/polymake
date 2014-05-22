package de.jreality.plugin.basic;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridLayout;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;

import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.text.BadLocationException;
import javax.swing.text.DefaultStyledDocument;
import javax.swing.text.Position;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyleConstants;

import de.jtem.jrworkspace.plugin.Controller;
import de.jtem.jrworkspace.plugin.sidecontainer.SideContainerPerspective;
import de.jtem.jrworkspace.plugin.sidecontainer.template.ShrinkPanelPlugin;

public class ConsolePlugin extends ShrinkPanelPlugin {

	private DefaultStyledDocument
		doc = new DefaultStyledDocument();
	private JTextPane
		console = new JTextPane(doc);
	private JScrollPane
		scroller = new JScrollPane(console);
	private Font
		textFont = new Font("Arial", Font.PLAIN, 9);
	
	
	public ConsolePlugin() {
		setInitialPosition(SHRINKER_BOTTOM);
		shrinkPanel.setTitle("Console");
		shrinkPanel.setLayout(new GridLayout());
		scroller.setPreferredSize(new Dimension(20, 250));
		shrinkPanel.add(scroller);
		console.setEditable(false);
		console.setFont(textFont);
	}
	
	
	@Override
	public void install(Controller c) throws Exception {
		super.install(c);
		System.setOut(new PrintStream(new ConsolePrintStream(Color.BLACK, System.out), true));
		System.setErr(new PrintStream(new ConsolePrintStream(Color.RED, System.err), true));
	}
	
	
	private class ConsolePrintStream extends ByteArrayOutputStream {

		private Color
			color = Color.BLACK;
		private PrintStream	
			forward = null;
		
		public ConsolePrintStream(Color c, PrintStream forward) {
			this.color = c;
			this.forward = forward;
		}
		
		@Override
		public void flush() throws IOException {
			super.flush();
			Position end = doc.getEndPosition();
			SimpleAttributeSet set = new SimpleAttributeSet();
			set.addAttribute(StyleConstants.Foreground, color);
			String msg = toString();
			try {
				doc.insertString(end.getOffset(), msg, set);
				console.select(doc.getLength(), doc.getLength()); 
			} catch (BadLocationException e) {
				e.printStackTrace();
			}
			if (forward != null) {
				forward.print(msg);
			}
			super.reset();
		}
		
	}
	
	
	@Override
	public Class<? extends SideContainerPerspective> getPerspectivePluginClass() {
		return View.class;
	}

}
