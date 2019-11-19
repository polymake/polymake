/* Copyright (c) 1997-2019
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

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.channels.SocketChannel;
import java.util.Vector;

/**
 * Launcher for an interactive polymake viewers based on Javaview. The
 * calculations are made in a C++ client, which gets and returns the data using
 * sockets.
 * 
 * @author Thilo Rörig
 */
public class Launcher {
	/** usage message of the Launcher */
	protected final static String USAGE = "usage: java de.tuberlin.polymake.common.Launcher <ps_port>";

	/** charset used for communication */
	public final static String CHARSET = "UTF-8";

	/**
	 * Creates a new Launcher and if given establishes the connection of the
	 * control classes to the corresponding c++ clients via sockets. It also
	 * starts a selector thread which manages the communication between
	 * polymake's clients/server and the java controls.
	 * 
	 * @param classes
	 *            control classes for different polymake visualizations which
	 *            are launched
	 * @param ports
	 *            portnumbers of sockets to c++ clients of polymake
	 * @param polymakeserverPort
	 *            portnumber of polymake server
	 * @exception IOException
	 *                if an error occurs when opening the communication channels
	 * @exception ClassNotFoundException
	 *                if a given control class does not exist
	 * @exception NoSuchMethodException
	 *                if a method of a control class cannot be found
	 * @exception InstantiationException
	 *                if a instance of a control class cannot be created
	 * @exception Exception
	 *                if another error occurs
	 */
	public Launcher(Vector classes, Vector ports, int polymakeserverPort)
			throws IOException, ClassNotFoundException, NoSuchMethodException,
			InstantiationException, Exception {
		if (classes.size() != ports.size()) {
			System.err.println("classes.size != ports.size");
			System.exit(0);
		}

		InetAddress[] localhostAddresses = InetAddress
				.getAllByName("localhost");
		SocketChannel polymakeserverChannel = SocketChannel.open();
		InetSocketAddress psa = null;

		for (int i = 0; i < localhostAddresses.length; ++i) {
			try {
				psa = new InetSocketAddress(localhostAddresses[i],
						polymakeserverPort);
				if (polymakeserverChannel.connect(psa)) {
					if (System.getProperty("polymake.debug") != null) {
						System.err
								.println("de.tuberlin.polymake.common.Launcher: Connection to "
										+ localhostAddresses[i].toString()
										+ ":"
										+ polymakeserverPort
										+ " successful.");
					}
					break;
				}
			} catch (IOException ex) {
				if (i <= localhostAddresses.length - 1) {
					if (System.getProperty("polymake.debug") != null) {
						System.err
								.println("de.tuberlin.polymake.common.Launcher: Connection to "
										+ localhostAddresses[i].toString()
										+ ":"
										+ polymakeserverPort
										+ " refused.");
					}
					polymakeserverChannel = SocketChannel.open();
					continue;
				} else {
					throw new IOException(
							"Connection to polymake server at localhost could not be established.");
				}
			}
		}

		polymakeserverChannel.configureBlocking(false);
//		BufferedReader psReader = new BufferedReader(Channels.newReader(polymakeserverChannel, CHARSET));

		SelectorThread selectorThread = new SelectorThread(polymakeserverChannel);

		/*
		 * Iterator ci = classes.iterator(); Iterator pi = ports.iterator();
		 * ClassLoader classLoader = ClassLoader.getSystemClassLoader();
		 * 
		 * Class bufferedReaderClass = classLoader
		 * .loadClass("java.io.BufferedReader"); Class pipeSinkChannelClass =
		 * classLoader .loadClass("java.nio.channels.Pipe$SinkChannel");
		 * SocketChannel clientChannel = null;
		 * 
		 * PolymakeControl staticGeometryControl = null; while (ci.hasNext()) {
		 * Class controlClass = (Class) ci.next(); int clientPort = ((Integer)
		 * pi.next()).intValue(); Constructor constructor =
		 * controlClass.getConstructor(new Class[] { bufferedReaderClass,
		 * bufferedReaderClass, pipeSinkChannelClass }); if (clientPort != -1) {
		 * Pipe controlPipe = Pipe.open();
		 * controlPipe.source().configureBlocking(false); InetSocketAddress csa =
		 * new InetSocketAddress(psa.getAddress(), clientPort); clientChannel =
		 * SocketChannel.open(); clientChannel.connect(csa);
		 * clientChannel.configureBlocking(false);
		 * 
		 * PolymakeControl pControl = (PolymakeControl) constructor
		 * .newInstance(new Object[] { psReader, new
		 * BufferedReader(Channels.newReader( clientChannel, CHARSET)),
		 * controlPipe.sink() });
		 * 
		 * selectorThread.registerControl(pControl, clientChannel, controlPipe); }
		 * else { if (staticGeometryControl == null) { Pipe controlPipe =
		 * Pipe.open(); controlPipe.source().configureBlocking(false);
		 * 
		 * staticGeometryControl = (PolymakeControl) constructor
		 * .newInstance(new Object[] { psReader, psReader, controlPipe.sink()
		 * }); selectorThread.registerControl(staticGeometryControl,
		 * controlPipe); } else { staticGeometryControl.update(); } }
		 *  }
		 */
		selectorThread.start();
	}

	/**
	 * main() method creating a new Launcher.
	 * 
	 * @param args[]
	 *            a <code>String</code> value containing the control classes
	 *            to be launched and corresponding port numbers if needed
	 */
	public static void main(String args[]) {
		try {
			Vector classes = new Vector();
			Vector ports = new Vector();
			int ps_port = -1;
			int i = 0;
			while (i < args.length) {
				if (args[i].equals("-ps_port")) {
					ps_port = Integer.parseInt(args[i + 1]);
					i += 2;
					break;
				}
				String className = args[i];
				classes.add(Class.forName(className));
				if (i < args.length - 2 && args[i + 1].equals("-client_port")) {
					ports.add(new Integer(args[i + 2]));
					i += 3;
				} else {
					ports.add(new Integer(-1));
					i += 1;
				}
			}
			if (i != args.length || ps_port == -1) {
				System.err.println(USAGE);
				System.exit(0);
			}
			new Launcher(classes, ports, ps_port);
		} catch (ClassNotFoundException e) {
			System.err.println(USAGE + "\n" + e.getMessage() + " not found.");
			System.exit(0);
		} catch (NumberFormatException e) {
			System.err.println(USAGE);
			System.exit(0);
		} catch (Exception e) {
			System.err.println("Launcher: error initializing channels.");
			e.printStackTrace(SelectorThread.newErr);
			System.exit(0);
		}
	}
}
