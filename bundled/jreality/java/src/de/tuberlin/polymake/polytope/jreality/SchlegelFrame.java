/* Copyright (c) 1997-2014
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

package de.tuberlin.polymake.polytope.jreality;

import java.util.Properties;

import javax.swing.JFrame;

import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;
import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;
import de.tuberlin.polymake.common.jreality.HelpFrame;
import de.tuberlin.polymake.common.jreality.Viewer;

public class SchlegelFrame extends
		de.tuberlin.polymake.polytope.PolymakeSchlegelFrame {

    private static final long serialVersionUID = 7319533398442778849L;

    public SchlegelFrame(EmbeddedGeometries geom, String title,
                         Properties params, Properties iparams, PolymakeControl parent) {

        super(geom, title, params, iparams, parent);
        Viewer jrv = new Viewer(geometry, ((de.tuberlin.polymake.common.jreality.BshParser)parent.getGeometryParser()).getTransformation());
        southBox.add(jrv.getViewerAppearancePanel());
        jrv.addPointDragListener(new PDListenerToClient());

        pViewer = jrv;

        setupSchlegelGUI();
    }

	private class PDListenerToClient implements PointDragListener {

		public void pointDragStart(PointDragEvent e) {
		}

		public void pointDragged(PointDragEvent e) {
			double[] coords = new double[3];
			System.arraycopy(e.getPosition(), 0, coords, 0, 3);
			geometry.moveVertex(e.getPointSet().getName(), e.getIndex(),coords);
			try {
				int vertexIndex = geometry.getEmbeddedVertexIndex(e
						.getPointSet().getName(), e.getIndex());
				parentControl.putMessage(SimpleGeometryParser.write(
						getGeomTitle(), vertexIndex), 'C', true);
			} catch (Exception ex) {
				ex.printStackTrace(SelectorThread.newErr);
			}
			statusBar.setText(" ");
			vertexDragged = true;
		}

		public void pointDragEnd(PointDragEvent e) {
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see de.tuberlin.polymake.common.PolymakeFrame#createHelpFrame()
	 */
	// @Override
	public JFrame createHelpFrame() {
		return new HelpFrame();
	}

    // @Override
    protected String writeTransformation() {
        return SimpleGeometryParser.writeTransformationMatrix(((Viewer)pViewer).getTransformationMatrix()) + "\n";
    }
}
