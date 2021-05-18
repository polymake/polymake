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

package de.tuberlin.polymake.common.javaview;

import java.awt.Color;
import java.awt.Component;
import java.util.StringTokenizer;

import javax.swing.JPanel;

import jv.object.PsViewerIf;
import jv.project.PvDisplayIf;
import jv.project.PvPickListenerIf;
import jv.viewer.PvControlMenu;
import jv.viewer.PvViewer;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;

/**
 * @author Thilo Rörig
 * 
 */
public class Viewer extends de.tuberlin.polymake.common.PolymakeViewer {

	/** One JavaView PvViewer controls all the displays of JavaView */
	public static PvViewer polymakeViewer = new PvViewer();

	/** Display containing the geometry. */
	protected PvDisplayIf disp;

	/**
	 * @param i_geom
	 */
	public Viewer(EmbeddedGeometries i_geom) {

		String jvLicenceFile = System.getProperty("jv.licence");
		if (jvLicenceFile != null) {
			jv.object.PsConfig.setCodeBase(jvLicenceFile);
		}
		polymakeViewer.showPanel(jv.object.PsViewerIf.DISPLAY);

		// Create viewer for viewing 3d geometries, and register applet.
		disp = polymakeViewer.newDisplay(i_geom.getName(), false);

		viewingPanel.add((Component) disp);

		// TODO: What is this good for?
		// disp.setFrame(this);

		StringTokenizer st = new StringTokenizer(System
				.getProperty("polymake.javaview.bgcolor"));
		disp.setBackgroundColor(new Color(Integer.parseInt(st.nextToken()),
				Integer.parseInt(st.nextToken()), Integer.parseInt(st
						.nextToken())));

		for (int i = 0; i < i_geom.getNumberOfGeometries(); ++i) {
			disp.addGeometry(((Geometry) i_geom.getGeometry(i))
					.getGeometry());
		}
		disp.selectGeometry(((Geometry) i_geom.getGeometry(0))
				.getGeometry());
		disp.fit();
		polymakeViewer.selectDisplay(disp);

	}

	/*
	 * @see de.tuberlin.polymake.common.PolymakeViewer#encompass()
	 */
	// @Override
	public void encompass() {
		disp.fit();
	}

	/*
	 * @see de.tuberlin.polymake.common.PolymakeViewer#getMenuBar()
	 */
	// @Override
	public Object getMenuBar() {
		PvControlMenu jvMenuBar = polymakeViewer.newMenuBar(null);
		jvMenuBar.setEnabledMenu(PsViewerIf.MENU_FILE_NEW_PROJECT, false);
		jvMenuBar.setEnabledMenu(PsViewerIf.MENU_FILE_NEW_GEOMETRY, false);
		jvMenuBar.setEnabledMenu(PsViewerIf.MENU_METHOD, true);
		return jvMenuBar;
	}

	/*
	 * @see de.tuberlin.polymake.common.PolymakeViewer#getViewerAppearancePanel()
	 */
	// @Override
	public JPanel getViewerAppearancePanel() {
		return null;
	}

	/**
	 * @param pl
	 */
	public void addPickListener(PvPickListenerIf pl) {
		disp.addPickListener(pl);
	}

}
