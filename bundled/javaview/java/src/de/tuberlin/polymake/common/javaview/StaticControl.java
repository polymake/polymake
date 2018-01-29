/* Copyright (c) 1997-2018
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

package de.tuberlin.polymake.common.javaview;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.channels.Pipe;

/**
 * @author Thilo Rörig
 * 
 */
public class StaticControl extends de.tuberlin.polymake.common.PolymakeStaticControl {
	public StaticControl(BufferedReader psReader,
			BufferedReader clientReader, Pipe.SinkChannel sink)
			throws IOException {
		super(psReader, clientReader, sink, new JvxParser());
	}

	// @Override
	public de.tuberlin.polymake.common.PolymakeFrame createFrame(String title) {
		return new StaticFrame(geometry, geometry.getName(), null,
				null, this);
	}
	
}
