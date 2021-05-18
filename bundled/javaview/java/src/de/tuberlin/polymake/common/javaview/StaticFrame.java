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

import java.util.Properties;

import javax.swing.JFrame;

import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.geometry.EmbeddedGeometries;
import de.tuberlin.polymake.common.geometry.PointSet;

public class StaticFrame extends PolymakeFrame {

	protected Viewer jvv = null;
	
	private static final long serialVersionUID = -2140725600323643961L;

	public StaticFrame(
			EmbeddedGeometries geom, 
			String title,
			Properties params, 
			Properties iparams, 
			PolymakeControl parent) 
	{
		super(geom, title, params, iparams, parent);
		jvv = new Viewer(geometry);
		setMenuBar(jvv.getMenuBar());
		pViewer = jvv;
		setupFrame();
	}

	public void update(PointSet ps, Properties params) {
	}

	// @Override
	public JFrame createHelpFrame() {
		//TODO: Create proper help frames!
		return null;
	}
	
	public void encompass() {
		jvv.encompass();
	}

}
