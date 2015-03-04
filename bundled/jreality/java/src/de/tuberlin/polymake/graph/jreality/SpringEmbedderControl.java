/* Copyright (c) 1997-2015
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

package de.tuberlin.polymake.graph.jreality;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.channels.Pipe;

import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.jreality.BshParser;

/**
 * @author wotzlaw
 * 
 * TODO To change the template for this generated type comment go to Window -
 * Preferences - Java - Code Style - Code Templates
 */
public class SpringEmbedderControl extends de.tuberlin.polymake.graph.PolymakeSpringEmbedderControl {

	public SpringEmbedderControl(BufferedReader psReader,
			BufferedReader clientReader, Pipe.SinkChannel sink)
			throws IOException {
		super(psReader, clientReader, sink, new BshParser());
	}

	// @Override
	public PolymakeFrame createFrame(String title) {
		return new SpringEmbedderFrame(geometry, title, parser.getParameters(), parser.getInteractiveParameters(), this);
	}
}
