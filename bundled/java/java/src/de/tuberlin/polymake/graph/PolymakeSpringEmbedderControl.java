/* Copyright (c) 1997-2023
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

package de.tuberlin.polymake.graph;

import java.awt.EventQueue;
import java.awt.Frame;
import java.io.BufferedReader;
import java.io.IOException;
import java.nio.channels.Pipe;

import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.SelectorThread;
import de.tuberlin.polymake.common.SharedMemoryMatrixException;
import de.tuberlin.polymake.common.io.GeometryParserIf;
import de.tuberlin.polymake.common.io.SimpleGeometryParser;

/**
 * @author thilosch
 * 
 */
public abstract class PolymakeSpringEmbedderControl extends PolymakeControl {

	public PolymakeSpringEmbedderControl(
			BufferedReader psReader,
			BufferedReader clientReader, 
			Pipe.SinkChannel sink,
			GeometryParserIf gp)
			throws IOException {
		super(psReader, clientReader, sink, gp);

		parser = new SimpleGeometryParser(new String[] { "continue",
				"viscosity", "inertion", "repulsion", "orientation", "delay" });

		try {
			putMessage("read " + geometry.getName() + "\n",'C', true);
		} catch (IOException ex) {
			ex.printStackTrace(SelectorThread.newErr);
		}
	}

	public void update() throws IOException {
		try {
			parser.parse(clientReader, geometry.getEmbedding());
			synchronized (clientQueue) {
				if (parser.getError() != null) {
					clientQueue.clear();
				} else {
					//if the clientQueue is empty the client is calculating 
					//data without the need of an answer: TODO
					if(!clientQueue.isEmpty()) {
						clientQueue.popFront();
					}
				}
				//another message is waiting to be sent to the client
				if (!clientQueue.isEmpty()) {
					bbuf.flip();
					pipeSink.write(bbuf);
				}
			}
			
			if (!(frameMap.containsKey(geometry.getEmbedding().getName()))
					|| !(((PolymakeSpringEmbedderFrame) frameMap.get(geometry
							.getEmbedding().getName())).isDisplayable())) {
				EventQueue.invokeLater(new Runnable() {
					
					@Override
					public void run() {
						PolymakeFrame tmpFrame = createFrame(geometry.getEmbedding().getName());
						tmpFrame.update(geometry.getEmbedding(), parser.getParameters());
						frameMap.put(geometry.getName(), tmpFrame);
						tmpFrame.encompass();
						tmpFrame.setVisible(true);
					}
				});
			} else {
				final PolymakeSpringEmbedderFrame tmpFrame = (PolymakeSpringEmbedderFrame) frameMap.get(geometry.getName());
				EventQueue.invokeLater(new Runnable() {

					@Override
					public void run() {
						if (tmpFrame.isVisible()
								|| tmpFrame.getState() == Frame.ICONIFIED) {
							tmpFrame.setState(Frame.NORMAL);
						}
						tmpFrame.setVisible(true);
						if (parser.getWarning() != null) {
							tmpFrame.setStatus(parser.getWarning());
						} else {
							if (parser.getError() != null) {
								tmpFrame.setStatus(parser.getError());
							}
						}
						tmpFrame.update(geometry.getEmbedding(), parser.getParameters());
					}
				});
			}
		} catch (SharedMemoryMatrixException e) {
			e.printStackTrace(SelectorThread.newErr);
		}
	}
}
