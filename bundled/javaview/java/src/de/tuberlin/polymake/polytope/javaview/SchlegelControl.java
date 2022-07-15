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

package de.tuberlin.polymake.polytope.javaview;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.channels.Pipe;

import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.javaview.JvxParser;

/**
 * This class ???
 * 
 * @author Thilo Schr&ouml;der
 */
public class SchlegelControl extends de.tuberlin.polymake.polytope.PolymakeSchlegelControl {

	public SchlegelControl(
			BufferedReader psReader,
			BufferedReader clientReader, 
			Pipe.SinkChannel sink)
			throws IOException {
		super(psReader, clientReader, sink, new JvxParser());
	}

	// @Override
	public PolymakeFrame createFrame(String title) {
		return new SchlegelFrame(geometry, title, parser.getParameters(), parser.getInteractiveParameters(), this);
	}

}
