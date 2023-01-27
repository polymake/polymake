/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.polytope.javaview;

import java.awt.BorderLayout;
import java.awt.Font;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import jv.object.PsConfig;
import de.tuberlin.polymake.common.SelectorThread;

/**
 * This class is used to display the help of the interactive
 * Schlegel diagram application.
 *
 * @author Thilo Schr&ouml;der
 */
public class SchlegelHelpFrame extends JFrame 
	implements ItemListener,  ActionListener {
    
    /**
	 * 
	 */
	private static final long serialVersionUID = -2569178159772020577L;

	/** Area to display help text. */
    protected JTextArea textArea = new JTextArea();
    
    /** Button to close Help. */
    protected JButton hideButton = new JButton("Close");
    
    /** To choose between Javaview and Schlegel interactive help.*/
    protected JComboBox whichHelp = new JComboBox();
    
    /** Bottom Panel to display buttons. */
    protected JPanel panel = new JPanel();
    
    /** String holding the help text. */
    protected String text = new String();
    
    /** Panel holding the title of the help text. */
    protected JPanel titlePanel = new JPanel();
    
    /** Title of help. */
    protected JLabel titleLabel = new JLabel();

    /**
	 * Create a new frame to display the contents of
	 * the Help.txt file.
	 */
	public SchlegelHelpFrame() {
		super("SchegelInteractive and Javaview Help");
		setBounds(new Rectangle(30,30,450,500)); 
		addWindowListener( new WindowAdapter() {
			public void windowClosing( WindowEvent e ) {
			    setVisible(false);
			}
		});
		setLayout(new BorderLayout());
		textArea.setEditable(false);
	
		titlePanel.setLayout(new BorderLayout());
		titleLabel.setFont(new Font("Arial",Font.BOLD,15));
		titleLabel.setText("SchlegelInteractive Help");
		titlePanel.add(titleLabel,BorderLayout.CENTER);
	
		hideButton.setActionCommand("hide");
		hideButton.addActionListener(this);
		
		whichHelp.addItem("Schlegel interactive");
		whichHelp.addItem("Javaview");
		whichHelp.addItemListener(this);
	
		panel.setLayout(new BorderLayout());
		panel.add(hideButton, BorderLayout.EAST);
		panel.add(whichHelp,BorderLayout.WEST);
		try{
		    BufferedInputStream in = new BufferedInputStream(getClass().getResource("Help.txt").openStream());
		    BufferedReader reader = new BufferedReader(new InputStreamReader(in));
		    String line = new String();
		    while(( line = reader.readLine()) != null) text += line + "\n";
		} catch( IOException e ) {SelectorThread.newErr.println(e.getMessage());}
		textArea.setText(text);
	
		add(titlePanel, BorderLayout.NORTH);
		add(new JScrollPane(textArea),BorderLayout.CENTER);
		add(panel,BorderLayout.SOUTH);
    }
    
    /** Process Button events. */
    public void actionPerformed(ActionEvent e) {
    	if(e.getActionCommand().equals("hide")) {
    		this.setVisible(false);
    	}
    }
    
    /** Process the changes of the Choice menu. */
	public void itemStateChanged( ItemEvent e ) {
		if(e.getItem().equals("Schlegel interactive")) {
			titleLabel.setText("SchlegelInteractive Help");
			textArea.setText(text);
		} else if (e.getItem().equals("Javaview")) {
			titleLabel.setText("Javaview Help (Keyboard Shortcuts)");
			textArea.setText(PsConfig.getMessage("28008"));//false,28008,"explanation"));
		}
	}

}
