/* Copyright (c) 1997-2021
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

import java.util.Properties;

import javax.swing.JFrame;
import javax.swing.JButton;
import javax.swing.JPanel;
import java.awt.GridLayout;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.io.IOException;

import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.PointSet;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;

public class StaticFrame extends de.tuberlin.polymake.common.PolymakeFrame {

    private static final long serialVersionUID = -5801265579728259187L;

    public StaticFrame(EmbeddedGeometries geom, String title,
                       Properties params, Properties iparams, de.tuberlin.polymake.common.PolymakeControl parent) {

        super(geom, title, params, iparams, parent);

        pViewer = new Viewer(geom, ((BshParser)parent.getGeometryParser()).getTransformation());

        JButton saveButton = new JButton("Save View");
        saveButton.addActionListener(new ActionListener() {

                public void actionPerformed(ActionEvent e) {
                    try {
                        parentControl.putMessage("n " + number + "\n" + writeTransformation() + "x\n", 'P', false);
                    } catch (IOException e1) {
                        SelectorThread.newErr.println("StaticFrame: error writing to polymake server");
                        e1.printStackTrace(SelectorThread.newErr);
                    }
                }
            });
        JPanel buttonPanel = new JPanel(new GridLayout(1, 2));
        buttonPanel.add(saveButton);
        southBox.add(buttonPanel);

        setupFrame();
    }

    protected String writeTransformation() {
        return SimpleGeometryParser.writeTransformationMatrix(((Viewer)pViewer).getTransformationMatrix()) + "\n";
    }

    // @Override
    public void update(PointSet ps, Properties params) {}

    /* (non-Javadoc)
     * @see de.tuberlin.polymake.common.PolymakeFrame#createHelpFrame()
     */
    // @Override
    public JFrame createHelpFrame() {
        return new HelpFrame();
    }

    public void encompass() {
        pViewer.encompass();
    }
}
