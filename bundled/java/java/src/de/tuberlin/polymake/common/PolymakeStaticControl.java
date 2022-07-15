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


package de.tuberlin.polymake.common;

import java.awt.EventQueue;
import java.io.BufferedReader;
import java.io.IOException;
import java.nio.channels.Pipe;

import de.tuberlin.polymake.common.io.GeometryParserIf;

/**
 * @author thilosch
 *
 */
public abstract class PolymakeStaticControl extends PolymakeControl {

	public PolymakeStaticControl(BufferedReader psReader,
			BufferedReader clientReader, Pipe.SinkChannel sink,
			GeometryParserIf gp)
			throws IOException {
		super(psReader, clientReader, sink, gp);
		EventQueue.invokeLater(new Runnable() {
			
			@Override
			public void run() {
				PolymakeFrame geomFrame = createFrame(geometry.getName());
				frameMap.put(geometry.getName(), geomFrame);
//				System.err.println("encompass");
				geomFrame.encompass();
				geomFrame.setVisible(true);
			}
		});
		
	}

	public void update() throws IOException {
		initGeometry();
		PolymakeFrame geomFrame = createFrame(geometry.getName());
		geomFrame.encompass();
		frameMap.put(geometry.getName(), geomFrame);
		geomFrame.setVisible(true);
	}

}
