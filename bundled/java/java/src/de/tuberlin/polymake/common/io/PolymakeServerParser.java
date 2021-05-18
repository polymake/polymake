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

package de.tuberlin.polymake.common.io;

import java.io.BufferedReader;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.net.InetSocketAddress;
import java.nio.channels.Channels;
import java.nio.channels.Pipe;
import java.nio.channels.SocketChannel;

import de.tuberlin.polymake.common.PolymakeFrame;
import de.tuberlin.polymake.common.PolymakeControl;
import de.tuberlin.polymake.common.SelectorThread;

/**
 * A simple line parser parsing commands coming from the 
 * polymake server.
 * 
 * @author Thilo Rörig
 */
public class PolymakeServerParser {

    SelectorThread parent;

    public PolymakeServerParser(SelectorThread parent) {
        this.parent = parent;
    }

    public void parseLine(java.io.BufferedReader in) throws PolymakeServerReadException, IOException {

        while (true) {
            String line = in.readLine();
            if (line == null) {
                throw new PolymakeServerReadException("Could not read from polymake server channel");
            }
            if (System.getProperty("polymake.debug") != null
                && System.getProperty("polymake.debug").equalsIgnoreCase("max")) {
                SelectorThread.newErr.println(line);
            }
            if (line.length() == 0) { 
                return;
            }
            line.trim();
            switch (line.charAt(0)) {
            case 'n':
                {
                    String[] parts = line.substring(2).split("\\s+");
                    int clientPort = Integer.parseInt(parts[1]);
                    try {
                        createAndRegisterControl(parts[0], clientPort, in);
                        return;
                    } catch (ControlCreationException e) {
                        e.printStackTrace(SelectorThread.newErr);
                    }
                    break;
                }
            case 'e':
                {
                    String[] parts = line.substring(2).split("\\s+", 2);
                    int frameId = Integer.parseInt(parts[0]);
                    PolymakeFrame f = PolymakeFrame.getById(frameId);
                    if (f != null) {
                        f.setStatus(parts[1]);
                    }
                    return;
                }
            default:
            }
        }
    }

    private void createAndRegisterControl(String className, int clientPort, BufferedReader polyReader) throws ControlCreationException {
        ClassLoader classLoader = ClassLoader.getSystemClassLoader();

        try {
            Class bufferedReaderClass = classLoader.loadClass("java.io.BufferedReader");
            Class pipeSinkChannelClass = classLoader.loadClass("java.nio.channels.Pipe$SinkChannel");
            Class controlClass = Class.forName(className);
            Constructor constructor = controlClass.getConstructor(new Class[] {
                                                                      bufferedReaderClass, bufferedReaderClass,
                                                                      pipeSinkChannelClass });
            if (clientPort != -1) {
                Pipe controlPipe = Pipe.open();
                controlPipe.source().configureBlocking(false);
                InetSocketAddress csa = new InetSocketAddress("localhost",
                                                              clientPort);
                SocketChannel clientChannel = SocketChannel.open();
                clientChannel.connect(csa);
                clientChannel.configureBlocking(false);

                PolymakeControl pControl = (PolymakeControl) constructor
                    .newInstance(new Object[] {
                                     polyReader,
                                     new BufferedReader(Channels.newReader(clientChannel, SelectorThread.CHARSET.name())),
                                     controlPipe.sink() });
                parent.registerControl(pControl, clientChannel, controlPipe);
            } else {
                // if (staticGeometryControl == null) {
                Pipe controlPipe = Pipe.open();
                controlPipe.source().configureBlocking(false);

                PolymakeControl staticGeometryControl = (PolymakeControl) constructor
                    .newInstance(new Object[] { polyReader, polyReader,
                                                controlPipe.sink() });
                parent.registerControl(staticGeometryControl, controlPipe);
                // } else {
                // staticGeometryControl.update();
                // }
            }
        } catch (Exception e) {
            e.printStackTrace(SelectorThread.newErr);
            throw new ControlCreationException("Could not create " + className + " connected to port " + clientPort);
        }
    }

    public class PolymakeServerReadException extends Exception {

        private static final long serialVersionUID = -2748406480628591233L;

        public PolymakeServerReadException(String msg) {
            super(msg);
        }
    }

    public class ControlCreationException extends Exception {

        private static final long serialVersionUID = 7056767773781646634L;

        public ControlCreationException(String msg) {
            super(msg);
        }
    }
}

// Local Variables:
// indent-tabs-mode:nil
// End:
