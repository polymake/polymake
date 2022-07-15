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

package de.tuberlin.polymake.common.ui;

import java.awt.AWTEvent;

/**
 * @author Thilo Rörig
 *
 */
public class SliderEvent extends AWTEvent{

	/** autogenerated ID for serialization */
	private static final long serialVersionUID = -2116679418874172447L;
	
	public static final long SLIDER_EVENT = 2*524288L;
	
	/**
	 * @param source
	 * @param id
	 */
	public SliderEvent(Object slider, int id) {
		super(slider, id);
	}
	
}
