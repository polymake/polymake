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

package de.tuberlin.polymake.common;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.Pipe;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;

import de.tuberlin.polymake.common.io.PolymakeServerParser;
import de.tuberlin.polymake.common.io.PolymakeServerParser.PolymakeServerReadException;

/**
 * This class is the heart of the polymake java interface. It implements a
 * Thread listening to multiple channels. A Selector is used to wake the Thread
 * up and process the input of the two channels. The input will be treated
 * according to the run method.
 * 
 * @author Thilo Rörig
 */
public class SelectorThread extends Thread {

    /** usage message of the Launcher */
    protected final static String USAGE = "usage: java de.tuberlin.polymake.common.Launcher <ps_port>";

    /** charset used for communication */
    public final static Charset CHARSET = Charset.forName("UTF-8");

    public final static PrintStream newErr = System.err;

    /** Selector to listen to socket and pipe simultanously. */
    protected Selector selector = null;

    /** Channel to communicate with polymake-server. */
    protected SocketChannel polyChannel = null;

    /** SelectionKey to check read access of socket. */
    protected SelectionKey polyKey = null;

    /** Reader to read from socket. */
    protected BufferedReader polyReader = null;

    /** Encoding used for socket communication. */
    protected Charset charset = null;

    /** mapping clientChannels to PolymakeControls */
    protected HashMap clientControlMap = new HashMap(8);

    protected HashMap pipeControlMap = new HashMap(8);

    protected HashMap pipeClientMap = new HashMap(8);

    /** Buffer to read from pipe. */
    protected ByteBuffer bbuf = ByteBuffer.allocateDirect(8);

    protected SelectionKey staticGeometryKey = null;

    protected PolymakeServerParser psp = null;

    /**
     * Creates a new <code>SelectorThread</code> instance.
     * 
     * @param psChannel
     *            the SocketChannel connecting to the polymake server
     * @param charset
     *            String describing the encoding of the <code>psChannel</code>
     * @exception IOException
     *                if an error occurs
     */
    public SelectorThread(SocketChannel psChannel) throws IOException {
        super("SelectorThread");

        selector = Selector.open();
        psp = new PolymakeServerParser(this);

        if (psChannel != null) {
            polyChannel = psChannel;
            polyReader = new BufferedReader(Channels.newReader(polyChannel,
                                                               CHARSET.name()));
        }
        setPriority(Thread.NORM_PRIORITY);
    }

    /**
     * Describe <code>registerControl</code> method here.
     * 
     * @param pc
     *            a <code>PolymakeControl</code> value
     * @param sc
     *            a <code>SocketChannel</code> value
     * @param p
     *            a <code>Pipe</code> value
     * @return a <code>SelectionKey</code> value
     * @exception IOException
     *                if an error occurs
     */
    public SelectionKey registerControl(PolymakeControl pc, SocketChannel sc, Pipe p) throws IOException {
        SelectionKey pk = p.source().register(selector, SelectionKey.OP_READ);
        SelectionKey sk = sc.register(selector, SelectionKey.OP_READ);
        clientControlMap.put(sk, pc);
        pipeControlMap.put(pk, pc);
        pipeClientMap.put(pk, sk);
        return sk;
    }

    /**
     * Describe <code>registerControl</code> method here.
     * 
     * @param pc
     *            a <code>PolymakeControl</code> value
     * @param p
     *            a <code>Pipe</code> value
     * @exception IOException
     *                if an error occurs
     */
    public void registerControl(PolymakeControl pc, Pipe p) throws IOException {
        SelectionKey pk = p.source().register(selector, SelectionKey.OP_READ);
        if (staticGeometryKey == null) {
            staticGeometryKey = pk;
        }
        pipeControlMap.put(pk, pc);
        pipeClientMap.put(pk, null);
    }

    /**
     * Describe <code>run</code> method here.
     * 
     */
    public void run() {
        try {
            polyKey = polyChannel.register(selector, SelectionKey.OP_READ);
            while (true) {
                if (System.getProperty("polymake.debug") != null) {
                    SelectorThread.newErr.println("java: selecting");
                }
                selector.select();
                Set selectedKeys = selector.selectedKeys();
                Iterator it = selectedKeys.iterator();
                while (it.hasNext()) {
                    SelectionKey actKey = (SelectionKey) it.next();
                    it.remove();
                    if (pipeControlMap.containsKey(actKey)) {
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("controlPipe...");
                        }

                        bbuf.clear();
                        ((Pipe.SourceChannel) actKey.channel()).read(bbuf);
                        bbuf.flip();
                        char ch = bbuf.getChar();
                        PolymakeControl pc = (PolymakeControl) pipeControlMap.get(actKey);
                        SelectionKey clientKey = (SelectionKey) pipeClientMap.get(actKey);
                        if (!pc.isAlive()) {
                            pipeControlMap.remove(actKey);
                            pipeClientMap.remove(actKey);
                            if (clientKey != null
                                && clientControlMap.containsKey(clientKey)) {
                                clientControlMap.remove(clientKey);
                                clientKey.channel().close();
                            }
                        }
                        switch (ch) {
                        case 'P': {
                            String msg = pc.getMessage();
                            polyChannel.write(CHARSET.encode(msg));
                            if (System.getProperty("polymake.debug") != null) {
                                SelectorThread.newErr.println("jv->pm:\n" + msg + "sent.");
                            }
                            break;
                        }
                        case 'C': {
                            if (clientKey != null) {
                                SocketChannel clientChannel = (SocketChannel) clientKey.channel();
                                String msg = pc.getMessage();
                                clientChannel.write(CHARSET.encode(msg));
                                if (System.getProperty("polymake.debug") != null) {
                                    SelectorThread.newErr.println("jv->client:\n" + msg + "sent.");
                                }
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                        }
                        if (!pc.getAnswer()) {
                            pc.popMessage();
                        }
                        if (!pc.isAlive() && pipeClientMap.size() == 0) {
                            System.exit(0);
                        }
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("controlPipe...done");
                        }
                    } else if (clientControlMap.containsKey(actKey)) {
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("clientChannel...");
                        }
                        ((PolymakeControl) clientControlMap.get(actKey)).update();
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("clientChannel...done");
                        }
                    } else if (polyKey.equals(actKey)) {
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("polyChannel...");
                        }
                        if (actKey.isReadable()) {
                            try {
                                psp.parseLine(polyReader);
                            } catch (PolymakeServerReadException e) {
                                // Connection to polymakeserver lost -> exit!
                                System.exit(0);
                            }
                        }
                        if (System.getProperty("polymake.debug") != null) {
                            System.err.println("polyChannel...done");
                        }
                    }
                }
                Thread.yield();
            }
        } catch (ClosedChannelException cce) {
            SelectorThread.newErr.println("Polymake exited while java running -- terminating java");
            System.exit(0);
        } catch (IOException e) {
            SelectorThread.newErr.println("SelectorThread: caught IOException in IOThread");
            e.printStackTrace(SelectorThread.newErr);
        }
    }

    /**
     * Close the channels and exit application, if ALL_ClOSED was received.
     * 
     * @throws IOException
     *             if any problems closing the channels
     */
    protected void cleanUp() throws IOException {
        if (System.getProperty("polymake.debug") != null) {
            SelectorThread.newErr.print("java exiting...");
        }
        if (polyChannel != null) {
            polyChannel.close();
        }
        if (System.getProperty("polymake.debug") != null) {
            SelectorThread.newErr.println("done!");
        }
        System.exit(0);
    }

    public static void main(String[] argv) {
        if (System.getProperty("polymake.debug") == null) {
            try {
                System.setOut(new PrintStream(new File("/dev/null")));
                System.setErr(new PrintStream(new File("/dev/null")));
            } catch (FileNotFoundException e1) {
                e1.printStackTrace(newErr);
            }
        }
        try {
            int ps_port = Integer.parseInt(argv[0]);
            InetAddress[] localhostAddresses = InetAddress.getAllByName("localhost");
            SocketChannel polymakeserverChannel = SocketChannel.open();
            InetSocketAddress psa = null;
            for (int i = 0; i < localhostAddresses.length; ++i) {
                try {
                    psa = new InetSocketAddress(localhostAddresses[i], ps_port);
                    polymakeserverChannel.configureBlocking(true);
                    if (polymakeserverChannel.connect(psa)) {
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr
                                .println("de.tuberlin.polymake.common.Launcher: Connection to "
                                         + localhostAddresses[i].toString()
                                         + ":" + ps_port + " successful.");
                        }
                        break;
                    }
                } catch (IOException ex) {
                    if (i <= localhostAddresses.length - 1) {
                        if (System.getProperty("polymake.debug") != null) {
                            SelectorThread.newErr.println("de.tuberlin.polymake.common.Launcher: Connection to "
                                                          + localhostAddresses[i].toString()
                                                          + ":" + ps_port + " refused.");
                        }
                        polymakeserverChannel = SocketChannel.open();
                        continue;
                    } else {
                        throw new IOException("Connection to polymake server at localhost could not be established.");
                    }
                }
            }
            polymakeserverChannel.configureBlocking(false);
            SelectorThread selectorThread = new SelectorThread(polymakeserverChannel);
            for (int i = 1; i < argv.length; ++i) {
                if(argv[i].equals("-nl")) {
                    System.loadLibrary(argv[++i]);
                }
            }
            selectorThread.start();
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace(SelectorThread.newErr);
            System.exit(0);
        } catch (NumberFormatException e) {
            SelectorThread.newErr.println(USAGE);
            System.exit(0);
        } catch (Exception e) {
            SelectorThread.newErr
                .println("SelectorThread: error initializing channels.");
            e.printStackTrace(SelectorThread.newErr);
            System.exit(0);
        }
    }
}

// Local Variables:
// indent-tabs-mode:nil
// End:
