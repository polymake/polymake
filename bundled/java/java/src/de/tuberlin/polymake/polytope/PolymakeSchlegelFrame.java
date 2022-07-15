/* Copyright (c) 1997-2022
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

package de.tuberlin.polymake.polytope;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.Properties;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JPanel;

import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;
import de.tuberlin.polymake.common.ui.ParameterPanel;
import de.tuberlin.polymake.common.ui.PolymakeSlider;
import de.tuberlin.polymake.common.ui.SliderEvent;
import de.tuberlin.polymake.common.ui.SliderListener;

/**
 * @author thilosch
 * 
 */
public abstract class PolymakeSchlegelFrame extends PolymakeFrame implements SliderListener {

    private static final long serialVersionUID = -4524691460396933711L;

    protected boolean vertexDragged = false;

    protected Vector facet = null;

    /**
     * @param geom
     * @param title
     * @param params
     * @param iparams
     * @param parent
     */
    public PolymakeSchlegelFrame(EmbeddedGeometries geom, String title, Properties params, Properties iparams,
                                 PolymakeControl parent) {
        super(geom, title, params, iparams, parent);
        geometry = (EmbeddedGeometries) geom.clone();

        paramPanel = new ParameterPanel(params, iparams, this);
    }

    /** update the frame with new values for the vertices and the zoom Factor */
    public void update(PointSet ps, Properties params, boolean clearTags) {
        if (!params.getProperty("zoom").equals("null")) {
            parameters.setProperty("zoom", params.getProperty("zoom"));
            if (vertexDragged) {
                paramPanel.setParameter("zoom", "" + 100 * Double.parseDouble(parameters.getProperty("zoom")));
                vertexDragged = false;
            }
        }
        geometry.update(ps, false);
    }

    public void update(PointSet ps, Properties params) {
        update(ps, params, false);
    }

    /**
     * The name of a listeners allows the display to issue verbal debug
     * messages.
     */
    public String getName() {
        return "SchlegelFramePicker";
    }

    /**
     * This method needs to be called at the end of the contructor of a
     * subclass of the PolymakeSchlegelFrame to setup a uniform
     * GUI for the visualization of interactive Schlegel diagrams.
     */
    protected void setupSchlegelGUI() {

        vertexDragged = true;

        JButton newFacetButton = new JButton("New Projection Facet");
        newFacetButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    try {
                        Vector markedVertices = geometry.getMarkedVertices();
                        if (markedVertices.isEmpty()) {
                            statusBar.setText("no points selected");
                        } else {
                            parentControl.putMessage(SimpleGeometryParser.writeFacet(""+number, markedVertices),
                                                     'P', false);
                            statusBar.setText(" ");
                        }
                    } catch (IOException ex) {
                        SelectorThread.newErr.println("SchlegelFrame: error writing to polymake server");
                        ex.printStackTrace(SelectorThread.newErr);
                    }
                }
            });

        JButton saveViewButton = new JButton("Save View");
        saveViewButton.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    try {
                        parentControl.putMessage("n " + number + "\nS\n" + writeTransformation() + "x\n", 'P', false);
                    } catch (IOException e1) {
                        SelectorThread.newErr.println("SchlegelFrame: error writing to polymake server");
                        e1.printStackTrace(SelectorThread.newErr);
                    }
                }
            });

        JPanel buttonPanel = new JPanel(new GridLayout(1, 2));
        buttonPanel.add(newFacetButton);
        buttonPanel.add(saveViewButton);
        southBox.add(paramPanel);
        southBox.add(buttonPanel);

        setupFrame();
    }

    // to be overridden
    protected String writeTransformation() { return ""; }

    public void sliderValueChanged(SliderEvent event) {
        PolymakeSlider zoomSlider = (PolymakeSlider) event.getSource();
        double sliderZoom = zoomSlider.getDoubleValue() / 100.0;
        double paramZoom = Double.parseDouble(parameters.getProperty("zoom"));
        if (Math.abs(sliderZoom - paramZoom) <= 0.01) {
            return;
        }

        if (sliderZoom == 0)
            sliderZoom = 0.01;
        String msg = "n " + getGeomTitle() + "\n" + "s zoom " + sliderZoom + "\nx\n";
        statusBar.setText(" ");
        try {
            parentControl.putMessage(msg, 'C', true);
        } catch (Exception ex) {
            SelectorThread.newErr.println("JavaviewSchlegelFrame: communication error");
            ex.printStackTrace(SelectorThread.newErr);
        }
    }
}
