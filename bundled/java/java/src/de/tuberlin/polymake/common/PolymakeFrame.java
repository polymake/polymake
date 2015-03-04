/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

package de.tuberlin.polymake.common;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.MenuBar;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.IOException;
import java.util.Properties;
import java.util.Vector;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenuBar;
import javax.swing.JPanel;
import javax.swing.JSplitPane;
import javax.swing.UIManager;

import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.ui.ParameterPanel;

/**
 * This class is the super class of all frames used for visualizing polymake content
 * with a Java backend.
 * 
 * @author Thilo Rörig
 *
 */
public abstract class PolymakeFrame extends JFrame implements WindowListener {

    private static final long serialVersionUID = 6476686808978198094L;

    /** Polymake Frame id */
    protected static int id = 1;

    private static Vector<PolymakeFrame> framesById = new Vector<PolymakeFrame>(4, 4);

    /** Frames number */
    protected int number;

    /** Help frame explaining controls of visualization used **/
    public static JFrame helpFrame = null;

    /** Button to open the associated helpFrame */
    protected JButton helpButton;

    /** Contains parameters for display */
    protected Properties parameters;

    /** Contains interactive parameters for display */
    protected Properties iparameters;

    /** Control keeping track of all frames of a type */
    protected PolymakeControl parentControl;

    /** title of the displayed geometry */
    protected String geomTitle;

    /** Panel to the north of the frame */
    protected JPanel northPanel = new JPanel(new BorderLayout());

    protected PolymakeViewer pViewer = null;

    /** The component containing the parameters */
    protected Box southBox = Box.createVerticalBox();

    protected ParameterPanel paramPanel = null;

    /** The status bar */
    protected JLabel statusBar = new JLabel(" ");

    /** The geometry */
    protected EmbeddedGeometries geometry;

    private final int FRAME_WIDTH = 800;

    private final int FRAME_HEIGHT = 600;

    public PolymakeFrame(EmbeddedGeometries geom, String title,
                         Properties params, Properties iparams, PolymakeControl parent) {

        super();
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e1) {
            e1.printStackTrace(SelectorThread.newErr);
        }
        this.number = id++;
        this.parentControl = parent;
        framesById.setSize(id);
        framesById.set(this.number, this);

        this.geometry = (EmbeddedGeometries)geom.clone();

        this.geomTitle = title;
        geomTitle = geomTitle.replaceAll("^.+:","");
        this.setName(geom.getName());
        if (geomTitle.equalsIgnoreCase("unnamed") || geomTitle.equalsIgnoreCase("")) {
            this.setTitle("* polymake *");      
        } else {
            this.setTitle("polymake - " + geomTitle);
        }

        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        this.setBounds(new Rectangle((dim.width - FRAME_WIDTH) / 2,
                                     (dim.height - FRAME_HEIGHT) / 2, FRAME_WIDTH, FRAME_HEIGHT));

        if (params != null) {
            parameters = (Properties) params.clone();
        }

        if (iparams != null) {
            iparameters = (Properties) iparams.clone();
        }

        this.addWindowListener(this);
                
        helpFrame = createHelpFrame();
        if (helpFrame != null) {
            helpButton = new JButton("Help");
            helpButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        if (!helpFrame.isVisible()
                            || helpFrame.getState() == Frame.ICONIFIED)
                            helpFrame.setLocation(getX() + 20, getY() + 20);
                        helpFrame.setState(Frame.NORMAL);
                        helpFrame.setVisible(true);
                    }
                });
            northPanel.add(helpButton, BorderLayout.EAST);

            helpButton.setAlignmentY(Component.CENTER_ALIGNMENT);
            helpButton.setMinimumSize(new Dimension(20, 20));
            helpButton.setPreferredSize(new Dimension(70, 20));
        }
        try {
            parentControl.putMessage("n " + number + "\n", 'P', false);
        } catch (IOException e) {
            e.printStackTrace(SelectorThread.newErr);
        }
    }

    public String getGeomTitle() {
        return geomTitle;
    }

    public static PolymakeFrame getById(int id) {
        try {
            return framesById.get(id);
        } catch (ArrayIndexOutOfBoundsException e) {
            return null;
        }
    }

    public void forget() {
        try {
            framesById.set(number, null);
        } catch (ArrayIndexOutOfBoundsException e) {
            e.printStackTrace(SelectorThread.newErr);
        }
    }

    /**
     * This method needs to be called at the end of the constructor of
     * every subclass of PolymakeFrame, since it puts together the necessary
     * components for the visualization in a uniform way.
     */
    public void setupFrame() {
        if (southBox.getComponentCount() != 0) {
            JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT,
                                                  pViewer.getViewingComponent(), southBox);
            splitPane.setContinuousLayout(true);
            splitPane.setOneTouchExpandable(true);
            splitPane.setResizeWeight(1.);
            this.getContentPane().add(splitPane, BorderLayout.CENTER);
        } else {
            this.getContentPane().add(pViewer.getViewingComponent(), BorderLayout.CENTER);
        }
        JPanel southPanel = new JPanel(new BorderLayout());
        southPanel.add(statusBar,BorderLayout.CENTER);
        if(helpButton != null) southPanel.add(helpButton,BorderLayout.EAST);
        this.getContentPane().add(southPanel, BorderLayout.SOUTH);
//      this.pack();
    }

    /** update the frame with new values for the vertices and parameters */
    public abstract void update(PointSet ps, Properties params);

    /** setStatusBar */
    public void setStatus(String status) {
        statusBar.setText(status);
    }

    public void windowActivated(WindowEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void windowClosing(WindowEvent e) {
        e.getWindow().dispose();
    }

    public void windowClosed(WindowEvent e) {
        try {
            parentControl.removeFrame(geomTitle);
            parentControl.putMessage("c " + number + "\nx\n",'P',false);
            parentControl.removeFrame(getName());
        } catch (IOException e1) {
            e1.printStackTrace(SelectorThread.newErr);
        }
    }

    public void windowDeactivated(WindowEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void windowDeiconified(WindowEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void windowIconified(WindowEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void windowOpened(WindowEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void encompass() {
        pViewer.encompass();
    }

    /**
     * Allows to set the menubar of the PolymakeFrame. The parameter 
     * should be of type MenuBar or JMenuBar.
     * 
     * @param mb
     */
    public void setMenuBar(Object mb) {
        if (mb instanceof MenuBar) {
            setMenuBar((MenuBar) mb);
        } else if (mb instanceof JMenuBar) {
            setJMenuBar((JMenuBar) mb);
        }
    }

    public abstract JFrame createHelpFrame();
}

// Local Variables:
// indent-tabs-mode:nil
// End:
