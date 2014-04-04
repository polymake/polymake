/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.logging.Level;

import javax.swing.JMenuBar;

import de.jreality.math.Pn;
import de.jreality.reader.ReaderBSH;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;


/**
 *
 * This class opens a ServerSocket and accepts connections.
 * The received input is passed to a ReaderBSH instance.
 * This class implements LoadableScene, so it can be plugged into a viewer.
 *
 * TODO: stop doesn't work yet.
 * 
 * @author weissman
 *
 */
public class BSHServer {
    
    private ReaderBSH readerBSH;
    private int port;
    private volatile boolean running;
    private final Object finishMutex = new Object();

    private Runnable server = new Runnable() {
        public void run() {
            ServerSocket s = null;
            try {
                s = new ServerSocket(port);
                s.setSoTimeout(20);
            } catch (IOException ioe) {
              LoggingSystem.getLogger(BSHServer.this).log(Level.SEVERE, "socket create error", ioe);
              return;
            }
            Socket sock=null;
            while (running) {
                try {
                    try {
                        sock = s.accept();
                    } catch (SocketTimeoutException st) {
                        continue;
                    }
                    InputStream is = sock.getInputStream();
                    PrintStream ps = new PrintStream(sock.getOutputStream());
                    synchronized (readerBSH) {
                        try {
                            readerBSH.appendInput(Input.getInput("socket input stream", is));
                        } catch(Exception e) {
                            e.printStackTrace(ps);
                        } finally
                        {
                          ps.close();
                          sock.close();
                        }
                    }
                } catch (IOException ioe) {
                  LoggingSystem.getLogger(BSHServer.this).log(Level.SEVERE, "socket IO error", ioe);
                }
            }
            try {
                if (sock != null) sock.close();
                if (s != null) s.close();
            } catch (IOException e) {
                LoggingSystem.getLogger(BSHServer.this).log(Level.INFO, "diconnect failed", e);
            }
            synchronized(finishMutex) {
                finishMutex.notifyAll();
            }
        }
    };
    
    /**
     * create a bsh server listening to the given port
     * @param port the port to listen
     * @throws Exception if IO failes or bsh failes
     */
    public BSHServer(int port) throws Exception {
        readerBSH = new ReaderBSH();
        this.port = port;
        start();
    }
    
    private void start() {
      if (running) throw new IllegalStateException("already started");
      running = true;
      new Thread(server).start();
    }
    
    public void finish() {
        synchronized (finishMutex) {
            running = false;
            try {
                finishMutex.wait();
            } catch (InterruptedException e) {
                throw new Error();
            }
        }
    }

    public void setConfiguration(ConfigurationAttributes config) {
    }

    public SceneGraphComponent makeWorld() {
        return readerBSH.getComponent();
    }

    public int getMetric() {
        return Pn.EUCLIDEAN;
    }

    public void customize(JMenuBar menuBar, Viewer viewer) {
    }

    
}
