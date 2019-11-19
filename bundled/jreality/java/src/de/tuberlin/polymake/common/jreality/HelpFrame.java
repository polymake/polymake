/* Copyright (c) 1997-2019
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

package de.tuberlin.polymake.common.jreality;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
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
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import de.tuberlin.polymake.common.SelectorThread;

public class HelpFrame extends JFrame 
	implements ItemListener,  ActionListener {
    
    private static final long serialVersionUID = -6777423753825260839L;

    /** Area to display help text. */
    protected JTextArea textArea = new JTextArea();
    
    /** Button to close Help. */
    protected JButton hideButton = new JButton("Close");
    
    /** To choose between Javaview and Schlegel interactive help.*/
    //protected Choice whichHelp = new Choice();
    
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
    public HelpFrame(){
		super("Jreality Help");
		setBounds(new Rectangle(30,30,450,500)); 
		addWindowListener( new WindowAdapter() {
			public void windowClosing( WindowEvent e ) {
			    setVisible(false);
			}
		});
		
		setLayout(new BorderLayout());
		textArea.setEditable(false);
		
		textArea.setBackground(Color.WHITE);
	
		titlePanel.setLayout(new BorderLayout());
		titleLabel.setFont(new Font("Arial",Font.BOLD,15));
		titleLabel.setText("JReality Help");
		titlePanel.add(titleLabel,BorderLayout.CENTER);
	
		hideButton.setActionCommand("hide");
		hideButton.addActionListener(this);
		
		hideButton.setAlignmentY(Component.CENTER_ALIGNMENT);
		hideButton.setMinimumSize(new Dimension(20,20));
		hideButton.setPreferredSize(new Dimension(70,20));
	
		panel.setLayout(new BorderLayout());
		panel.add(hideButton, BorderLayout.EAST);
		
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
		if(e.getItem().equals("jReality")) {
			titleLabel.setText("jReality help");
			textArea.setText(text);
		} 
	}

}
