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

package de.tuberlin.polymake.graph.javaview;

import java.util.Properties;

import javax.swing.JFrame;

import jv.object.PsDebug;
import jv.project.PgGeometryIf;
import jv.project.PvPickEvent;
import jv.project.PvPickListenerIf;
import jv.vecmath.PdVector;
import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.javaview.Viewer;
import de.tuberlin.polymake.common.ui.SliderListener;

public class SpringEmbedderFrame extends de.tuberlin.polymake.graph.PolymakeSpringEmbedderFrame
		implements PvPickListenerIf, SliderListener {

	/**
	 * 
	 */
	private static final long serialVersionUID = -7774465990362965484L;

	/**
	 * Create a frame displaying the given geometry with given zoom parameter.
	 * The output of any event is written into a pipe.
	 * 
	 * @param i_geom
	 *            geometry to be displayed
	 * @param params
	 *            non Javaview jvx parameters
	 * @param sink
	 */
	public SpringEmbedderFrame(EmbeddedGeometries geom, String title,
			Properties params, Properties iparams, PolymakeControl parent) {

		super(geom, title, params, iparams, parent);

		Viewer jvv = new Viewer(geometry);
		jvv.addPickListener(this);
		setMenuBar(jvv.getMenuBar());
		pViewer = jvv;

		setupSpringEmbedderGUI();
	}

	// ---------------- Begin of PvPickListenerIf ------------------------

	/**
	 * The name of a listeners allows the display to issue verbal debug
	 * messages.
	 */
	public String getName() {
		return "SpringEmbedderFramePicker";
	}

	/** Currently not supported by display yet. */
	public void selectGeometry(PgGeometryIf geom) {
	}

	/**
	 * Get a location in the display with 2d display and 3d world coordinates.
	 * Method is called when display is in mode PvDisplayIf.MODE_DISPLAY_PICK.
	 * 
	 * @see jv.project.PvPickListenerIf
	 * @param pos
	 *            Pick event issued by the display
	 */
	public void pickDisplay(PvPickEvent pos) {
		PsDebug.message("pickDisplay entered.");
	}

	/**
	 * Drag a location in the display with 2d display and 3d world coordinates.
	 * Method is called when display is in mode PvDisplayIf.MODE_DISPLAY_PICK.
	 * 
	 * @see jv.project.PvPickListenerIf
	 * @param pos
	 *            Pick event issued by the display
	 */
	public void dragDisplay(PvPickEvent pos) {
		PsDebug.message("pickDisplay entered.");
	}

	/**
	 * Pick an arbitrary point on a geometry, point may lie inside an element.
	 * Method is called when display is in mode PvDisplayIf.MODE_INITIAL_PICK or
	 * if temporarily the i-key is pressed, and any pixel in the display is
	 * picked.
	 * 
	 * @see jv.project.PvPickListenerIf
	 * @param pos
	 *            Pick event issued by the display
	 */
	public void pickInitial(PvPickEvent pos) {
		// PsDebug.message("pickInitial entered.");
	}

	/**
	 * Drag an arbitrary point along a geometry, point may lie inside an
	 * element. Method is called when display is in mode
	 * PvDisplayIf.MODE_INITIAL_PICK or if temporarily the i-key is pressed, and
	 * any pixel in the display is dragged.
	 * 
	 * @see jv.project.PvPickListenerIf
	 * @param pos
	 *            Pick event issued by the display
	 */
	public void dragInitial(PvPickEvent pos) {
		// PsDebug.message("dragInitial entered = "+pos.getLocation());
	}

	/**
	 * Get a picked vertex of a geometry. Method is called when display is in
	 * mode PvDisplayIf.MODE_PICK or if temporarily the p-key is pressed, and a
	 * vertex picked.
	 * 
	 * @see jv.project.PvPickListenerIf
	 * @param geom
	 *            Picked geometry on which vertex lies
	 * @param index
	 *            Index of vertex in vertex array of geometry
	 * @param vertex
	 *            3d coordinates of vertex position
	 */
	public void pickVertex(PgGeometryIf geom, int index, PdVector vertex) {
	}

	/**
	 * <p>
	 * Method is called when display is in mode PvDisplayIf.MODE_PICK or if
	 * temporarily the p-key is pressed, and a vertex dragged.
	 * </p>
	 * <p>
	 * Drag a picked vertex of a geometry and put its coordinates into the
	 * MsgQueue. If the queue is empty sent PolymakeFrame.ANSWER to sink.
	 * </p>
	 * 
	 * @param geom
	 *            Picked geometry on which vertex lies
	 * @param index
	 *            Index of vertex in vertex array of geometry
	 * @param vertex
	 *            3d coordinates of vertex position
	 */
	public void dragVertex(PgGeometryIf a_geom, int index, PdVector vertex) {
		double[] coords = new double[3];
		System.arraycopy(vertex.m_data, 0, coords, 0, 3);
		geometry.moveVertex(a_geom.getName(), index, coords);
	}

	/**
	 * Mark a set of vertices of a geometry within a given bounding box. Method
	 * is called when display is in mode PvDisplayIf.MODE_MARK or if temporarily
	 * the m-key is pressed, and a rectangle is drawn.
	 * 
	 * @param markBox
	 *            contains four coplanar points on the bounding prism, and
	 *            direction of prism.
	 */
	public void markVertices(PvPickEvent markBox) {
	}

	/**
	 * Unmark a set of vertices of a geometry within a given bounding box.
	 * Method is called when display is in mode PvDisplayIf.MODE_UNMARK or if
	 * temporarily the u-key is pressed, and a rectangle is drawn.
	 * 
	 * @param markBox
	 *            contains four coplanar points on the bounding prism, and
	 *            direction of prism.
	 */
	public void unmarkVertices(PvPickEvent markBox) {
		// PsDebug.message("unmarkVertices entered. Unmarked Vertex: " +
		// pos.getVertexInd());
	}

	// ---------------- End of PvPickListenerIf ------------------------

	/*
	 * (non-Javadoc)
	 * 
	 * @see de.tuberlin.polymake.common.PolymakeFrame#createHelpFrame()
	 */
	// @Override
	public JFrame createHelpFrame() {
		return new SpringEmbedderHelpFrame();
	}

}
