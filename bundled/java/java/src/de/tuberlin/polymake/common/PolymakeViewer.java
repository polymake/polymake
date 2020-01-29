/* Copyright (c) 1997-2020
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

package de.tuberlin.polymake.common;

import java.awt.GridLayout;

import javax.swing.JComponent;
import javax.swing.JPanel;

/**
 * This class needs to be extended by all viewers which are supposed to be used
 * to display EmbeddedGeometries
 * 
 * @author Thilo Rörig
 * 
 */
public abstract class PolymakeViewer {

	protected JPanel viewingPanel = new JPanel(new GridLayout());

	protected JPanel viewerAppearancePanel = null;
	
	/**
	 * Get the component displaying the geometry.
	 * 
	 * @return the viewing component
	 */
	public JComponent getViewingComponent() {
		return viewingPanel;
	}

	/**
	 * Adapt the size of the displayed geometry to the size of the viewer
	 * component.
	 */
	public abstract void encompass();

	//FIXME: how to unify JMenuBar and MenuBar???
	public abstract Object getMenuBar();

	/**
	 * Get the panel containing additional parameters governing the appearance
	 * of the viewing component.
	 * 
	 * @return panel with viewing parameters
	 */
	public abstract JPanel getViewerAppearancePanel();
}
